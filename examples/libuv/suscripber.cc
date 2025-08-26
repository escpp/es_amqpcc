/**
 * @file libuv_subscriber_example.cc
 * @brief 使用libuv和es_amqpcc::Subscriber的示例程序
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "libuv/subscriber.h"
#include "json.h"

using LibuvSubscriber = es_amqpcc::LibuvSubscriber;

const std::string exchange_name = "test-exchange";
const std::string routing_key = "test.routing.key";
const std::string queue_name = "test-queue";

int main()
{
    std::cout << "Starting libuv subscriber example..." << std::endl;

    try {
        // 创建Subscriber实例
        LibuvSubscriber subscriber(exchange_name, routing_key, queue_name);
        
        // 设置连接成功回调
        subscriber.on_connect_success([]() {
            std::cout << "Connected to RabbitMQ server successfully!" << std::endl;
        });
        
        // 设置连接失败回调
        subscriber.on_connect_error([](const std::string &error) {
            std::cout << "Connection failed: " << error << std::endl;
        });
        
        // 设置消息接收回调
        subscriber.on_received([&subscriber](const AMQP::Message &message, uint64_t delivery_tag, bool redelivered) {
            std::cout << "📨 消息接收:" << std::endl;
            std::cout << "  - 内容: " << (char*)message.body() << std::endl;
            std::cout << "  - 长度: " << message.bodySize() << " bytes" << std::endl;
            std::cout << "  - 投递标签: " << delivery_tag << std::endl;
            std::cout << "  - 重投递: " << (redelivered ? "是" : "否") << std::endl;
            
            // 示例：根据消息内容决定如何处理
            std::string content((char*)message.body(), message.bodySize());
            
            es_amqpcc::json j;
            try{
                j = es_amqpcc::json::parse(content);
            } catch (const std::exception &e) {
                std::cerr << "Exception: " << e.what() << std::endl;
                std::cerr << "内容: " << content << std::endl;
                subscriber.reject(delivery_tag);
                return;
            }

            if (j["cmd"] == "ack") {
                std::cout << "✅ 处理成功，确认消息" << std::endl;
                std::cout << j.dump(4) << std::endl;
                subscriber.ack(delivery_tag);
            } else if (j["cmd"] == "retry") {
                std::cout << "🔄 需要重试，重入队消息" << std::endl;
                std::cout << j.dump(4) << std::endl;
                subscriber.requeue(delivery_tag);
            } else {
                std::cout << "❌ 处理失败，拒绝消息" << std::endl;
                std::cout << j.dump(4) << std::endl;
                subscriber.reject(delivery_tag);
            }
        });
        
        // 设置消费成功回调
        subscriber.on_consume_success([](const std::string &consumer_tag) {
            std::cout << "✅ 消费开始成功，消费者标签: " << consumer_tag << std::endl;
        });
        
        // 设置消费错误回调
        subscriber.on_consume_error([](const std::string &error) {
            std::cout << "❌ 消费开始失败: " << error << std::endl;
        });
        
        // 连接到RabbitMQ服务器
        std::cout << "Connecting to RabbitMQ server..." << std::endl;
        if (subscriber.connect("amqp://rabbitmq:rabbitmq@localhost/")) {
            // 运行事件循环（这会阻塞直到连接关闭）
        } else {
            std::cout << "Failed to initialize connection" << std::endl;
            return 1;
        }

        subscriber.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Subscriber example completed" << std::endl;
    return 0;
}
