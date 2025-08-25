#include <iostream>
#include <string>
#include <random>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <event2/event.h>

/**
 * 消息处理状态
 */
enum class MessageStatus {
    SUCCESS,    // 处理成功
    RETRY,      // 需要重试
    DISCARD     // 丢弃消息
};

/**
 * 自定义连接处理器
 */
class MyHandler : public AMQP::LibEventHandler
{
public:
    /**
     * 构造函数
     */
    MyHandler(struct event_base* evbase) : AMQP::LibEventHandler(evbase) {}

    /**
     * 连接错误回调
     */
    virtual void onError(AMQP::TcpConnection* connection, const char* message) override
    {
        std::cerr << "连接错误: " << message << std::endl;
    }

    /**
     * 连接成功回调
     */
    virtual void onConnected(AMQP::TcpConnection* connection) override
    {
        std::cout << "成功连接到 RabbitMQ 服务器" << std::endl;
    }

    /**
     * 连接关闭回调
     */
    virtual void onClosed(AMQP::TcpConnection* connection) override
    {
        std::cout << "连接已关闭" << std::endl;
    }
};

/**
 * 模拟消息处理业务逻辑
 * @return 处理状态
 */
MessageStatus processMessage(const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
{
    std::string body(message.body(), message.bodySize());
    std::cout << "收到消息 [" << deliveryTag << "]: " << body;
    
    if (redelivered) {
        std::cout << " (重投递)";
    }
    std::cout << std::endl;

    // 获取重试次数（从消息头）
    int retryCount = 0;
    AMQP::Table headers = message.headers();
    if (headers.contains("x-retry-count")) {
        retryCount = static_cast<int>(headers.get("x-retry-count"));
        std::cout << "重试次数: " << retryCount << std::endl;
    }

    // 模拟不同的处理结果（随机生成用于演示）
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    int result = dis(gen);

    if (result <= 60) {
        // 60% 概率成功
        std::cout << "消息处理成功" << std::endl;
        return MessageStatus::SUCCESS;
    } else if (result <= 85) {
        // 25% 概率需要重试
        std::cout << "消息处理失败，需要重试" << std::endl;
        return MessageStatus::RETRY;
    } else {
        // 15% 概率丢弃
        std::cout << "消息处理严重错误，丢弃消息" << std::endl;
        return MessageStatus::DISCARD;
    }
}

int main()
{
    // 创建 libevent 事件基础
    struct event_base* evbase = event_base_new();
    if (!evbase) {
        std::cerr << "无法创建事件基础" << std::endl;
        return 1;
    }

    // 创建连接处理器
    MyHandler handler(evbase);

    // 创建 AMQP 连接地址
    AMQP::Address address("amqp://guest:guest@localhost:5672/");

    // 建立 TCP 连接
    AMQP::TcpConnection connection(&handler, address);

    // 创建通道
    AMQP::TcpChannel channel(&connection);

    // 设置服务质量（每次只处理一条消息）
    channel.setQos(1)
        .onSuccess([]() {
            std::cout << "QoS 设置成功" << std::endl;
        })
        .onError([](const char* message) {
            std::cerr << "QoS 设置失败: " << message << std::endl;
        });

    // 声明主交换器
    channel.declareExchange("test-exchange", AMQP::direct, AMQP::durable)
        .onSuccess([]() {
            std::cout << "主交换器声明成功" << std::endl;
        })
        .onError([](const char* message) {
            std::cerr << "主交换器声明失败: " << message << std::endl;
        });

    // 声明主队列
    channel.declareQueue("test-queue", AMQP::durable)
        .onSuccess([](const std::string& name, uint32_t, uint32_t) {
            std::cout << "主队列声明成功: " << name << std::endl;
        })
        .onError([](const char* message) {
            std::cerr << "主队列声明失败: " << message << std::endl;
        });

    // 绑定主队列到主交换器
    channel.bindQueue("test-exchange", "test-queue", "test.key")
        .onSuccess([]() {
            std::cout << "队列绑定成功" << std::endl;
        })
        .onError([](const char* message) {
            std::cerr << "队列绑定失败: " << message << std::endl;
        });

    std::cout << "开始消费消息..." << std::endl;

    // 开始消费消息
    channel.consume("test-queue")
        .onSuccess([](const std::string& consumerTag) {
            std::cout << "开始消费，消费者标签: " << consumerTag << std::endl;
        })
        .onReceived([&channel](const AMQP::Message& message, 
                              uint64_t deliveryTag, 
                              bool redelivered) {
            
            // 处理消息
            MessageStatus status = processMessage(message, deliveryTag, redelivered);

            // 根据处理结果采取不同行动
            switch (status) {
                case MessageStatus::SUCCESS:
                    // 处理成功，确认消息
                    channel.ack(deliveryTag);
                    std::cout << "消息确认完成" << std::endl;
                    break;

                case MessageStatus::RETRY:
                    // 处理失败，重新入队重试
                    channel.reject(deliveryTag, AMQP::requeue);
                    std::cout << "消息重新入队" << std::endl;
                    break;

                case MessageStatus::DISCARD:
                    // 严重错误，丢弃消息
                    channel.reject(deliveryTag);
                    std::cout << "消息已丢弃" << std::endl;
                    break;
            }

            std::cout << "----------------------------------------" << std::endl;
        })
        .onError([](const char* message) {
            std::cerr << "消费错误: " << message << std::endl;
        });

    // 运行事件循环
    std::cout << "消费者已启动，等待消息..." << std::endl;
    event_base_dispatch(evbase);

    // 清理资源
    connection.close();
    event_base_free(evbase);

    std::cout << "消费者程序执行完成" << std::endl;
    return 0;
}
