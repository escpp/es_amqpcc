# AMQP-CPP 使用说明文档

## 概述

AMQP-CPP 是一个用于 C++ 的异步 AMQP 客户端库，支持 RabbitMQ 和其他 AMQP 0.9.1 兼容的消息代理。该库提供了高性能的消息处理能力，支持多种事件循环集成。

## 核心组件

### 1. Address 类

用于解析和管理 AMQP 连接地址。

#### 构造函数
```cpp
// 从字符串解析地址
AMQP::Address address("amqp://guest:guest@localhost:5672/vhost");

// 从已知属性构造
AMQP::Address address("localhost", 5672, AMQP::Login("user", "pass"), "vhost");
```

#### 主要方法
- `secure()`: 检查是否使用安全连接 (amqps://)
- `login()`: 获取登录信息
- `hostname()`: 获取主机名
- `port()`: 获取端口号
- `vhost()`: 获取虚拟主机
- `options()`: 获取额外选项

### 2. Connection 类

管理 AMQP 连接的核心类。

#### 构造函数
```cpp
// 使用连接处理器和地址
AMQP::Connection connection(handler, address);

// 使用连接处理器、登录信息和虚拟主机
AMQP::Connection connection(handler, login, vhost);
```

#### 主要方法
- `parse()`: 解析从服务器接收的数据
- `heartbeat()`: 发送心跳包
- `ready()`: 检查连接是否就绪
- `usable()`: 检查连接是否可用
- `close()`: 关闭连接
- `channels()`: 获取活动通道数量

### 3. Channel 类

提供 AMQP 通道操作的主要接口，包含所有 AMQP 操作的方法。

#### 构造函数
```cpp
// 通过连接创建通道
AMQP::Channel channel(&connection);

// 移动构造函数（允许通道对象的移动）
AMQP::Channel channel2 = std::move(channel);
```

#### 通道管理方法
- `ready()`: 检查通道是否就绪
- `usable()`: 检查通道是否可用
- `close()`: 关闭通道
- `id()`: 获取通道ID
- `pause()`: 暂停消息传递
- `resume()`: 恢复消息传递

#### 队列操作

##### declareQueue - 声明队列（多个重载版本）
```cpp
// 基本用法 - 指定队列名和标志
channel.declareQueue("my-queue", AMQP::durable)
    .onSuccess([](const std::string &name, uint32_t msgCount, uint32_t consumerCount) {
        std::cout << "队列声明成功: " << name << std::endl;
    });

// 带额外参数 - 支持消息TTL、死信交换器等
AMQP::Table arguments;
arguments["x-message-ttl"] = 60000; // 消息存活60秒
arguments["x-dead-letter-exchange"] = "dlx"; // 死信交换器
channel.declareQueue("delayed-queue", AMQP::durable, arguments);

// 自动生成队列名 - 服务器分配名称
channel.declareQueue(AMQP::exclusive)
    .onSuccess([](const std::string &name, uint32_t, uint32_t) {
        std::cout << "临时队列: " << name << std::endl;
    });

// 仅检查队列是否存在（passive模式）
channel.declareQueue("existing-queue", AMQP::passive)
    .onSuccess([](const std::string &name, uint32_t, uint32_t) {
        std::cout << "队列存在: " << name << std::endl;
    })
    .onError([](const char *message) {
        std::cerr << "队列不存在: " << message << std::endl;
    });
```

##### 队列标志说明

| 标志 | 描述 | 使用场景 |
|------|------|----------|
| `AMQP::durable` | 持久化队列，服务器重启后仍然存在 | 重要业务数据存储 |
| `AMQP::autodelete` | 当所有消费者断开时自动删除队列 | 临时工作队列 |
| `AMQP::passive` | 只检查队列是否存在，不创建队列 | 队列状态验证 |
| `AMQP::exclusive` | 独占队列，仅对声明连接可见 | 私有临时队列 |

##### bindQueue - 绑定队列到交换器
```cpp
// 基本绑定
channel.bindQueue("orders-exchange", "orders-queue", "order.created")
    .onSuccess([]() {
        std::cout << "队列绑定成功" << std::endl;
    });

// 带额外参数的绑定
AMQP::Table bindArgs;
bindArgs["x-match"] = "any"; // 匹配任意头
bindArgs["priority"] = "high";
channel.bindQueue("headers-exchange", "high-priority-queue", "", bindArgs);
```

##### unbindQueue - 解绑队列
```cpp
channel.unbindQueue("orders-exchange", "orders-queue", "order.created")
    .onSuccess([]() {
        std::cout << "队列解绑成功" << std::endl;
    });
```

##### removeQueue - 删除队列
```cpp
// 无条件删除队列
channel.removeQueue("my-queue")
    .onSuccess([](uint32_t messageCount) {
        std::cout << "队列删除，包含 " << messageCount << " 条消息" << std::endl;
    });

// 条件删除：只有当队列未使用且为空时才删除
channel.removeQueue("my-queue", AMQP::ifunused + AMQP::ifempty)
    .onSuccess([](uint32_t messageCount) {
        std::cout << "队列删除成功" << std::endl;
    })
    .onError([](const char *message) {
        std::cerr << "队列删除失败: " << message << std::endl;
    });
```

##### purgeQueue - 清空队列
```cpp
channel.purgeQueue("my-queue")
    .onSuccess([](uint32_t messageCount) {
        std::cout << "队列清空，移除 " << messageCount << " 条消息" << std::endl;
    });
```

#### 交换器操作

##### declareExchange - 声明交换器（多个重载版本）
```cpp
// 声明直连交换器
channel.declareExchange("my-exchange", AMQP::direct, AMQP::durable)
    .onSuccess([]() {
        std::cout << "交换器声明成功" << std::endl;
    });

// 声明带参数的交换器
AMQP::Table exchangeArgs;
exchangeArgs["alternate-exchange"] = "my-ae"; // 备用交换器
channel.declareExchange("main-exchange", AMQP::direct, AMQP::durable, exchangeArgs);

// 自动命名交换器
channel.declareExchange(AMQP::fanout, AMQP::durable)
    .onSuccess([]() {
        std::cout << "交换器声明成功" << std::endl;
    });

// 交换器类型说明
// - AMQP::direct: 直连交换器 - 精确匹配路由键
// - AMQP::fanout: 广播交换器 - 忽略路由键，发送到所有绑定队列
// - AMQP::topic: 主题交换器 - 模式匹配路由键（支持通配符）
// - AMQP::headers: 头交换器 - 基于消息头键值对匹配
```

##### removeExchange - 删除交换器
```cpp
// 无条件删除
channel.removeExchange("my-exchange")
    .onSuccess([]() {
        std::cout << "交换器删除成功" << std::endl;
    });

// 条件删除：只有当交换器未使用时才删除
channel.removeExchange("my-exchange", AMQP::ifunused)
    .onSuccess([]() {
        std::cout << "交换器删除成功" << std::endl;
    })
    .onError([](const char *message) {
        std::cerr << "交换器删除失败: " << message << std::endl;
    });
```

##### bindExchange - 绑定交换器（交换器到交换器）
```cpp
// 绑定两个交换器
channel.bindExchange("source-exchange", "target-exchange", "routing-key")
    .onSuccess([]() {
        std::cout << "交换器绑定成功" << std::endl;
    });

// 带参数的绑定
AMQP::Table bindArgs;
bindArgs["x-match"] = "all";
channel.bindExchange("source-exchange", "target-exchange", "routing-key", bindArgs);
```

##### unbindExchange - 解绑交换器
```cpp
channel.unbindExchange("target-exchange", "source-exchange", "routing-key")
    .onSuccess([]() {
        std::cout << "交换器解绑成功" << std::endl;
    });
```

#### 消息发布

##### publish - 发布消息（多个重载版本）
```cpp
// 发布字符串消息（最简单形式）
channel.publish("exchange", "routing.key", "Hello World!");

// 发布带标志的消息
channel.publish("exchange", "routing.key", "Hello World!", AMQP::mandatory);

// 发布信封消息（完整控制）
AMQP::Envelope envelope("Message content", 15);
envelope.setDeliveryMode(2); // 持久化消息
envelope.setPriority(5);     // 消息优先级
envelope.setExpiration("60000"); // 过期时间60秒
channel.publish("exchange", "routing.key", envelope, AMQP::mandatory);

// 发布原始数据
const char *data = "Raw binary data";
channel.publish("exchange", "routing.key", data, strlen(data));

// 发布标志说明
// - AMQP::mandatory: 如果消息无法路由到任何队列，返回给发送者
// - AMQP::immediate: 如果消息无法立即投递给消费者，返回给发送者（RabbitMQ 3.0+已弃用）

// 处理被返回的消息（使用mandatory标志时）
channel.recall()
    .onReturned([](const AMQP::Message &message, uint16_t code, const char *text) {
        std::cout << "消息被返回: " << text << " Code: " << code << std::endl;
    });
```

#### 消息消费

##### consume - 开始消费（多个重载版本）
```cpp
// 基本消费
channel.consume("my-queue")
    .onSuccess([](const std::string &tag) {
        std::cout << "开始消费，标签: " << tag << std::endl;
    })
    .onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "收到消息: " << std::string(message.body(), message.bodySize()) << std::endl;
        // 确认消息
        channel.ack(deliveryTag);
    })
    .onCancelled([](const std::string &tag) {
        std::cout << "消费被取消，标签: " << tag << std::endl;
    });

// 指定消费者标签和标志
channel.consume("my-queue", "my-consumer", AMQP::exclusive)
    .onSuccess([](const std::string &tag) {
        std::cout << "开始独占消费: " << tag << std::endl;
    });

// 带额外参数的消费
AMQP::Table consumeArgs;
consumeArgs["x-priority"] = 10; // 消费者优先级
channel.consume("my-queue", "my-consumer", 0, consumeArgs);

// 消费标志说明
// - AMQP::noack: 自动确认消息，不需要手动ack
// - AMQP::exclusive: 独占消费，其他消费者不能访问该队列
// - AMQP::nolocal: 不接收本连接发布的消息
```

##### get - 获取单条消息
```cpp
channel.get("my-queue")
    .onSuccess([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "获取到消息: " << std::string(message.body(), message.bodySize()) << std::endl;
        channel.ack(deliveryTag);
    })
    .onEmpty([]() {
        std::cout << "队列为空" << std::endl;
    });

// 使用noack标志自动确认
channel.get("my-queue", AMQP::noack)
    .onSuccess([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "获取到消息（自动确认）" << std::endl;
    });
```

##### cancel - 取消消费
```cpp
channel.cancel("my-consumer-tag")
    .onSuccess([](const std::string &tag) {
        std::cout << "消费取消成功: " << tag << std::endl;
    });
```

#### 消息确认

##### ack - 确认消息
```cpp
// 确认单条消息
channel.ack(deliveryTag);

// 确认多条消息（累积确认）
channel.ack(deliveryTag, AMQP::multiple);
// 这会确认deliveryTag及之前的所有未确认消息
```

##### reject - 拒绝消息
```cpp
// 拒绝并重新入队（让其他消费者处理）
channel.reject(deliveryTag, AMQP::requeue);

// 拒绝并丢弃（进入死信队列或直接删除）
channel.reject(deliveryTag);

// 拒绝多条消息
channel.reject(deliveryTag, AMQP::multiple + AMQP::requeue);
```

##### recover - 恢复未确认的消息
```cpp
// 请求重新投递所有未确认的消息
channel.recover(AMQP::requeue)
    .onSuccess([]() {
        std::cout << "消息恢复成功" << std::endl;
    });
// 标志说明：
// - AMQP::requeue: true-重新入队（可能被其他消费者获取），false-重新投递给原消费者
```

#### 事务管理

##### confirmSelect - 开启确认模式
```cpp
channel.confirmSelect()
    .onSuccess([]() {
        std::cout << "确认模式已开启" << std::endl;
    });
```

##### 事务操作
```cpp
// 开始事务
channel.startTransaction();

// 提交事务  
channel.commitTransaction()
    .onSuccess([]() {
        std::cout << "事务提交成功" << std::endl;
    });

// 回滚事务
channel.rollbackTransaction();
```

#### 服务质量控制

##### setQos - 设置服务质量
```cpp
// 设置预取计数
channel.setQos(10) // 最多10条未确认消息
    .onSuccess([]() {
        std::cout << "QoS设置成功" << std::endl;
    });
```

### 4. Deferred 对象

所有异步操作都返回 Deferred 对象，支持链式回调。

#### 回调方法
```cpp
channel.declareQueue("test")
    .onSuccess([](const std::string &name, uint32_t msgCount, uint32_t consumerCount) {
        // 操作成功回调
    })
    .onError([](const char *message) {
        // 操作失败回调
    })
    .onFinalize([]() {
        // 最终回调（无论成功失败都会执行）
    });
```

## 事件循环集成

### LibUV 集成
```cpp
#include <amqpcpp/libuv.h>

class MyHandler : public AMQP::LibUvHandler {
public:
    MyHandler(uv_loop_t *loop) : AMQP::LibUvHandler(loop) {}
    
    void onError(AMQP::TcpConnection *connection, const char *message) override {
        std::cout << "错误: " << message << std::endl;
    }
    
    void onConnected(AMQP::TcpConnection *connection) override {
        std::cout << "连接成功" << std::endl;
    }
};

// 使用
uv_loop_t *loop = uv_default_loop();
MyHandler handler(loop);
AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://localhost"));
```

### LibEV 集成
```cpp
#include <amqpcpp/libev.h>

class MyHandler : public AMQP::LibEvHandler {
public:
    MyHandler(struct ev_loop *loop) : AMQP::LibEvHandler(loop) {}
    // ... 实现事件处理方法
};
```

### Boost.Asio 集成
```cpp
#include <amqpcpp/libboostasio.h>

AMQP::LibBoostAsioHandler handler(io_service);
AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://localhost"));
```

## 消息处理

### Message 类
```cpp
// 在接收回调中访问消息属性
.onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    // 消息体
    std::string body(message.body(), message.bodySize());
    
    // 消息属性
    std::cout << "交换器: " << message.exchange() << std::endl;
    std::cout << "路由键: " << message.routingkey() << std::endl;
    std::cout << "重投递: " << (redelivered ? "是" : "否") << std::endl;
});
```

### Envelope 类
用于构建要发送的消息信封。

```cpp
AMQP::Envelope envelope("消息内容");
envelope.setDeliveryMode(2); // 持久化模式
envelope.setPriority(5);     // 优先级
envelope.setExpiration("60000"); // 过期时间(毫秒)

channel.publish("exchange", "routing.key", envelope);
```

## 错误处理

### 连接错误处理
```cpp
class MyHandler : public AMQP::ConnectionHandler {
    void onError(AMQP::TcpConnection *connection, const char *message) override {
        std::cerr << "连接错误: " << message << std::endl;
    }
    
    void onReady(AMQP::TcpConnection *connection) override {
        std::cout << "连接就绪" << std::endl;
    }
    
    void onClosed(AMQP::TcpConnection *connection) override {
        std::cout << "连接关闭" << std::endl;
    }
};
```

### 操作错误处理
```cpp
channel.declareQueue("test")
    .onError([](const char *message) {
        std::cerr << "队列声明失败: " << message << std::endl;
    });
```

## 最佳实践

### 1. 连接管理
```cpp
// 使用连接池管理连接
// 设置合理的心跳间隔
// 实现连接重连机制
```

### 2. 通道使用
```cpp
// 为不同的业务使用不同的通道
// 及时关闭不再使用的通道
// 合理设置QoS预取计数
```

### 3. 消息处理
```cpp
// 使用确认模式确保消息可靠性
// 实现死信队列处理无法投递的消息
// 设置消息过期时间避免消息堆积
```

### 4. 性能优化
```cpp
// 批量发布消息
// 使用持久化连接
// 合理设置缓冲区大小
```

## 示例代码

### 基于 LibUV 的示例
```cpp
#include <uv.h>
#include <amqpcpp.h>
#include <amqpcpp/libuv.h>

/**
 * 自定义事件处理器
 */
class MyHandler : public AMQP::LibUvHandler
{
private:
    /**
     * 连接错误回调
     */
    virtual void onError(AMQP::TcpConnection *connection, const char *message) override
    {
        std::cout << "连接错误: " << message << std::endl;
    }

    /**
     * 连接成功回调
     */
    virtual void onConnected(AMQP::TcpConnection *connection) override 
    {
        std::cout << "连接成功" << std::endl;
    }
    
public:
    /**
     * 构造函数
     */
    MyHandler(uv_loop_t *loop) : AMQP::LibUvHandler(loop) {}
};

int main()
{
    // 获取事件循环
    auto *loop = uv_default_loop();
    
    // 创建事件处理器
    MyHandler handler(loop);
    
    // 建立连接
    AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://guest:guest@localhost/"));
    
    // 创建通道
    AMQP::TcpChannel channel(&connection);
    
    // 声明临时队列（独占模式）
    channel.declareQueue(AMQP::exclusive)
        .onSuccess([&connection](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
            std::cout << "声明队列成功: " << name << std::endl;
        });
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

### 基于 LibEvent 的示例
```cpp
#include <event2/event.h>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>

int main()
{
    // 创建事件基础
    auto evbase = event_base_new();

    // 创建事件处理器
    AMQP::LibEventHandler handler(evbase);

    // 建立连接
    AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://localhost/"));

    // 创建通道
    AMQP::TcpChannel channel(&connection);

    // 声明临时队列
    channel.declareQueue(AMQP::exclusive)
        .onSuccess([&connection](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
            std::cout << "声明队列: " << name << std::endl;
            connection.close(); // 完成后关闭连接
        });

    // 运行事件循环
    event_base_dispatch(evbase);
    event_base_free(evbase);

    return 0;
}
```

### 完整生产者-消费者示例
```cpp
#include <uv.h>
#include <amqpcpp.h>
#include <amqpcpp/libuv.h>
#include <thread>
#include <chrono>

class MyHandler : public AMQP::LibUvHandler {
public:
    MyHandler(uv_loop_t *loop) : AMQP::LibUvHandler(loop) {}
    
    void onError(AMQP::TcpConnection *connection, const char *message) override {
        std::cout << "错误: " << message << std::endl;
    }
    
    void onConnected(AMQP::TcpConnection *connection) override {
        std::cout << "连接成功" << std::endl;
    }
};

// 生产者函数
void producer() {
    uv_loop_t *loop = uv_default_loop();
    MyHandler handler(loop);
    AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://localhost/"));
    AMQP::TcpChannel channel(&connection);
    
    // 声明持久化队列
    channel.declareQueue("task-queue", AMQP::durable)
        .onSuccess([&channel](const std::string &name, uint32_t, uint32_t) {
            // 发布10条消息
            for (int i = 0; i < 10; i++) {
                std::string message = "任务 " + std::to_string(i);
                AMQP::Envelope envelope(message.data(), message.size());
                envelope.setDeliveryMode(2); // 持久化消息
                channel.publish("", name, envelope);
                std::cout << "发送: " << message << std::endl;
            }
        });
    
    uv_run(loop, UV_RUN_DEFAULT);
}

// 消费者函数
void consumer() {
    uv_loop_t *loop = uv_default_loop();
    MyHandler handler(loop);
    AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://localhost/"));
    AMQP::TcpChannel channel(&connection);
    
    // 设置QoS，每次只处理一条消息
    channel.setQos(1);
    
    // 开始消费
    channel.consume("task-queue")
        .onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
            std::string body(message.body(), message.bodySize());
            std::cout << "接收: " << body << " (重投递: " << redelivered << ")" << std::endl;
            
            // 模拟处理时间
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 确认消息处理完成
            channel.ack(deliveryTag);
            std::cout << "确认: " << body << std::endl;
        });
    
    uv_run(loop, UV_RUN_DEFAULT);
}

int main() {
    // 启动消费者线程
    std::thread consumerThread(consumer);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 启动生产者
    producer();
    
    consumerThread.join();
    return 0;
}
```

### 使用交换器和路由的示例
```cpp
#include <uv.h>
#include <amqpcpp.h>
#include <amqpcpp/libuv.h>

class MyHandler : public AMQP::LibUvHandler {
public:
    MyHandler(uv_loop_t *loop) : AMQP::LibUvHandler(loop) {}
    
    void onError(AMQP::TcpConnection *connection, const char *message) override {
        std::cout << "错误: " << message << std::endl;
    }
};

int main() {
    uv_loop_t *loop = uv_default_loop();
    MyHandler handler(loop);
    AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://localhost/"));
    AMQP::TcpChannel channel(&connection);
    
    // 1. 声明直连交换器
    channel.declareExchange("orders", AMQP::direct, AMQP::durable)
        .onSuccess([]() { std::cout << "交换器声明成功" << std::endl; });
    
    // 2. 声明多个队列
    channel.declareQueue("order-processing", AMQP::durable);
    channel.declareQueue("inventory-update", AMQP::durable);
    channel.declareQueue("payment-processing", AMQP::durable);
    
    // 3. 绑定队列到交换器
    channel.bindQueue("orders", "order-processing", "order.created");
    channel.bindQueue("orders", "inventory-update", "order.created"); 
    channel.bindQueue("orders", "payment-processing", "order.created");
    
    // 4. 发布消息到交换器
    channel.publish("orders", "order.created", "新订单数据");
    
    // 5. 开始消费各个队列
    channel.consume("order-processing")
        .onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool) {
            std::cout << "订单处理: " << std::string(message.body(), message.bodySize()) << std::endl;
            channel.ack(deliveryTag);
        });
    
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## AMQP 消息流详解

### 生产者 → 交换器 → 队列 → 消费者 消息流

#### 1. 生产者与交换器、路由键的关系

**应用场景举例：电商订单系统**

```cpp
// 生产者发布不同类型的订单消息
void publishOrder(AMQP::TcpChannel &channel, const std::string &orderType, const std::string &orderData) {
    // 使用不同的路由键发布到同一个交换器
    if (orderType == "normal") {
        channel.publish("orders-exchange", "order.created.normal", orderData);
    } else if (orderType == "vip") {
        channel.publish("orders-exchange", "order.created.vip", orderData);
    } else if (orderType == "urgent") {
        channel.publish("orders-exchange", "order.created.urgent", orderData);
    }
}

// 设置交换器和队列绑定
void setupOrderSystem(AMQP::TcpChannel &channel) {
    // 声明主题交换器
    channel.declareExchange("orders-exchange", AMQP::topic, AMQP::durable);
    
    // 声明不同优先级的队列
    channel.declareQueue("normal-orders", AMQP::durable);
    channel.declareQueue("vip-orders", AMQP::durable); 
    channel.declareQueue("urgent-orders", AMQP::durable);
    
    // 使用路由键模式绑定队列到交换器
    channel.bindQueue("orders-exchange", "normal-orders", "order.created.normal");
    channel.bindQueue("orders-exchange", "vip-orders", "order.created.vip");
    channel.bindQueue("orders-exchange", "urgent-orders", "order.created.urgent");
    
    // 也可以使用通配符绑定
    channel.bindQueue("orders-exchange", "all-orders", "order.created.*");
}
```

#### 2. 消费者与交换器、队列、路由键的关系

```cpp
// 不同优先级的消费者
void setupConsumers(AMQP::TcpChannel &channel) {
    // 普通订单消费者
    channel.consume("normal-orders", "normal-consumer")
        .onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
            processNormalOrder(message, deliveryTag);
        });
    
    // VIP订单消费者（更高优先级）
    channel.consume("vip-orders", "vip-consumer") 
        .onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
            processVipOrder(message, deliveryTag);
        });
    
    // 紧急订单消费者（最高优先级）
    channel.consume("urgent-orders", "urgent-consumer")
        .onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
            processUrgentOrder(message, deliveryTag);
        });
}
```

#### 3. 消息流转全过程

1. **生产者发布消息** → 交换器 + 路由键
2. **交换器路由消息** → 根据路由键匹配绑定规则
3. **消息进入队列** → 按照绑定规则分发到相应队列
4. **消费者获取消息** → 从队列中拉取或推送消息
5. **消息处理完成** → 确认消费或重新入队

#### 4. 消息确认和重排队机制

##### 什么时候确认消费（ACK）
```cpp
void processOrder(const AMQP::Message &message, uint64_t deliveryTag) {
    try {
        // 业务逻辑处理
        bool success = businessLogic.process(message);
        
        if (success) {
            // 处理成功，确认消息
            channel.ack(deliveryTag);
            std::cout << "订单处理成功，消息已确认" << std::endl;
        } else {
            // 处理失败，拒绝消息并重新入队
            channel.reject(deliveryTag, AMQP::requeue);
            std::cout << "订单处理失败，消息重新入队" << std::endl;
        }
    } catch (const std::exception &e) {
        // 发生异常，拒绝消息并丢弃（进入死信队列）
        channel.reject(deliveryTag);
        std::cerr << "处理异常，消息丢弃: " << e.what() << std::endl;
    }
}
```

##### 什么时候消费失败
- **业务逻辑失败**：订单数据验证失败、库存不足、支付失败等
- **系统异常**：数据库连接失败、外部服务不可用、程序bug等
- **超时处理**：消息处理时间过长，需要重新分配

##### 什么时候需要重新排队
```cpp
void processMessage(const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
    // 检查重投递次数
    if (redelivered) {
        // 获取重投递计数（需要从消息头获取）
        int redeliveryCount = getRedeliveryCount(message);
        
        if (redeliveryCount >= 3) {
            // 超过最大重试次数，丢弃消息
            channel.reject(deliveryTag);
            std::cout << "超过最大重试次数，消息丢弃" << std::endl;
            return;
        }
    }
    
    // 临时性错误，重新入队重试
    if (isTemporaryError()) {
        channel.reject(deliveryTag, AMQP::requeue);
        std::cout << "临时错误，消息重新入队" << std::endl;
    } 
    // 永久性错误，丢弃消息
    else if (isPermanentError()) {
        channel.reject(deliveryTag);
        std::cout << "永久错误，消息丢弃" << std::endl;
    }
    // 处理成功，确认消息
    else {
        channel.ack(deliveryTag);
        std::cout << "处理成功，消息确认" << std::endl;
    }
}
```

##### 为什么需要重新排队
1. **临时性故障**：网络抖动、服务短暂不可用、数据库连接超时
2. **资源竞争**：多个消费者同时处理同一资源时的锁冲突
3. **依赖服务**：依赖的外部服务暂时不可用
4. **负载均衡**：将消息重新分配给其他消费者处理

#### 5. 完整的消息生命周期管理

```cpp
// 设置死信交换器处理无法处理的消息
void setupDlx(AMQP::TcpChannel &channel) {
    // 声明死信交换器
    channel.declareExchange("dlx", AMQP::direct, AMQP::durable);
    channel.declareQueue("dead-letters", AMQP::durable);
    channel.bindQueue("dlx", "dead-letters", "");
    
    // 设置队列的死信参数
    AMQP::Table queueArgs;
    queueArgs["x-dead-letter-exchange"] = "dlx";
    queueArgs["x-dead-letter-routing-key"] = "dead-letters";
    queueArgs["x-message-ttl"] = 600000; // 10分钟过期
    
    channel.declareQueue("orders-with-dlx", AMQP::durable, queueArgs);
}

// 处理死信队列中的消息
void processDeadLetters(AMQP::TcpChannel &channel) {
    channel.consume("dead-letters")
        .onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool) {
            // 记录无法处理的消息，人工干预或批量处理
            logDeadLetter(message);
            channel.ack(deliveryTag);
        });
}
```

## 常见问题

### 1. 连接失败
- 检查RabbitMQ服务器是否运行
- 验证用户名密码是否正确
- 确认虚拟主机是否存在

### 2. 消息丢失
- 使用持久化队列和消息
- 开启确认模式
- 实现适当的错误处理

### 3. 性能问题
- 调整预取计数
- 使用批量操作
- 优化网络配置

## 参考资料

- [AMQP 0.9.1 协议规范](https://www.rabbitmq.com/resources/specs/amqp0-9-1.pdf)
- [RabbitMQ 文档](https://www.rabbitmq.com/documentation.html)
- [AMQP-CPP GitHub 仓库](https://github.com/CopernicaMarketingSoftware/AMQP-CPP)

---

这份文档提供了 AMQP-CPP 库的全面使用指南，涵盖了核心类、方法、参数说明以及最佳实践。根据实际需求，您可以参考相应的章节来使用这个强大的 AMQP 客户端库。
