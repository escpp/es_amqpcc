/**
 * @file libuv_exchange_example.cc
 * @brief 使用libuv和es_amqpcc::exchange的示例程序（新接口）
 */

#include <iostream>
#include "libuv/publisher.h"
#include "json.h"

// 定义字符串字面量用于模板实例化
const std::string exchange_name = "test-exchange";
const std::string routing_key = "test.routing.key";

int main()
{
    std::cout << "Starting libuv exchange example (new interface)..." << std::endl;

    try {
        // 创建exchange实例
        es_amqpcc::LibuvPublisher test_exchange(exchange_name, routing_key);
        
        // 设置连接成功回调
        test_exchange.on_connect_success([]() {
            std::cout << "Connected to RabbitMQ server successfully!" << std::endl;
        });
        
        // 设置连接错误回调
        test_exchange.on_connect_error([](const std::string &error) {
            std::cout << "Connection Error: " << error << std::endl;
        });

        test_exchange.on_declare_success([]() {
            std::cout << "Exchange declared successfully!" << std::endl;
        });

        test_exchange.on_declare_error([](const std::string &error) {
            std::cout << "Exchange declare Error: " << error << std::endl;
        });
        
        // 设置连接关闭回调
        test_exchange.on_closed([]() {
            std::cout << "Connection closed!" << std::endl;
        });
        
        // 设置消息发布成功回调
        test_exchange.on_publish_success([]() {
            std::cout << "Message published successfully!" << std::endl;
        });
        
        // 设置发布错误回调
        test_exchange.on_publish_error([](const std::string &error) {
            std::cout << "Publish Error: " << error << std::endl;
        });
        
        
        // 连接到RabbitMQ服务器
        if (test_exchange.connect("amqp://rabbitmq:rabbitmq@localhost/")) {
            std::cout << "Connected successfully!" << std::endl;
            test_exchange.run();
            
            for (int i = 0; i < 10; ++i) {
                es_amqpcc::json j;
                j["seq"] = i;
                j["ii"] = double(i*i);
                j["cmd"] = "ack";

                // 发布JSON消息
                if (test_exchange.publish(j)) {
                    std::cout << "JSON message published successfully!" << std::endl;
                    std::cout << "Total messages published: " << test_exchange.size() << std::endl;
                } else {
                    std::cout << "Failed to publish JSON message" << std::endl;
                }
                getchar();
            }
        } else {
            std::cout << "Failed to connect to RabbitMQ server!" << std::endl;
        }
        
        // 运行事件循环（这会阻塞直到连接关闭）
        std::cout << "Running event loop..." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Example completed" << std::endl;
    getchar();
    return 0;
}
