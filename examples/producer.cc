#include <iostream>
#include <string>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <event2/event.h>
#include <unistd.h>

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

    // 开启确认模式
    channel.confirmSelect()
        .onSuccess([]() {
            std::cout << "确认模式已开启" << std::endl;
        })
        .onError([](const char* message) {
            std::cerr << "开启确认模式失败: " << message << std::endl;
        });

    // 声明交换器 (直连类型，持久化)
    channel.declareExchange("test-exchange", AMQP::direct, AMQP::durable)
        .onSuccess([]() {
            std::cout << "交换器 'test-exchange' 声明成功" << std::endl;
        })
        .onError([](const char* message) {
            std::cerr << "交换器声明失败: " << message << std::endl;
        });

    // 发布多条测试消息
    std::cout << "开始发布消息..." << std::endl;

    for (int i = 1; i <= 10; ++i) {
        std::string message = "测试消息 " + std::to_string(i);
        std::string routingKey = "test.key";

        // 创建消息信封
        AMQP::Envelope envelope(message.data(), message.size());
        envelope.setDeliveryMode(2); // 持久化消息
        envelope.setPriority(1);     // 消息优先级
        envelope.setContentType("text/plain");

        // 发布消息
        channel.publish("test-exchange", routingKey, envelope, AMQP::mandatory);

        std::cout << "已发布消息 [" << i << "]: " << message << std::endl;
        
        // 短暂延迟，模拟真实场景
        struct timeval tv = {0, 100000}; // 100ms
        event_base_loopexit(evbase, &tv);
        event_base_loop(evbase, EVLOOP_NONBLOCK);
    }

    std::cout << "消息发布完成，等待确认..." << std::endl;

    // 运行事件循环
    event_base_dispatch(evbase);

    // 等待一段时间让所有确认到达
    std::cout << "等待最终确认..." << std::endl;
    ::sleep(2);

    // 关闭连接
    connection.close();

    // 清理资源
    event_base_free(evbase);

    std::cout << "生产者程序执行完成" << std::endl;
    return 0;
}
