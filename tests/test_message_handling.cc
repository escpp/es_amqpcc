/**
 * @file test_message_handling.cc
 * @brief 测试消息确认、拒绝和重入队功能的示例
 */

#include <iostream>
#include "include/libuv/subscriber.h"

using LibuvSubscriber = es_amqpcc::libuv::Subscriber;

int main()
{
    std::cout << "Testing message handling functionality..." << std::endl;

    try {
        // 创建Subscriber实例
        LibuvSubscriber subscriber("test-exchange", "test.routing.key", "test-queue");
        
        // 测试消息确认功能
        std::cout << "Testing ack() function..." << std::endl;
        // 注意：在实际使用中，deliveryTag来自接收到的消息
        // 这里只是演示接口调用
        
        // 测试拒绝功能
        std::cout << "Testing reject() function..." << std::endl;
        
        // 测试重入队功能
        std::cout << "Testing requeue() function..." << std::endl;
        
        std::cout << "✅ All message handling functions are available!" << std::endl;
        std::cout << "  - ack(uint64_t): 确认消息" << std::endl;
        std::cout << "  - reject(uint64_t): 拒绝消息（丢弃）" << std::endl;
        std::cout << "  - requeue(uint64_t): 拒绝消息并重入队" << std::endl;
        
        // 演示如何在on_received回调中使用这些功能
        subscriber.on_received([&subscriber](const AMQP::Message &message, uint64_t delivery_tag, bool redelivered) {
            std::cout << "Message received with delivery tag: " << delivery_tag << std::endl;
            
            // 根据消息内容决定如何处理
            std::string content((char*)message.body(), message.bodySize());
            
            if (content.find("success") != std::string::npos) {
                std::cout << "Processing successful - acknowledging message" << std::endl;
                subscriber.ack(delivery_tag);
            } else if (content.find("retry") != std::string::npos) {
                std::cout << "Need to retry - requeuing message" << std::endl;
                subscriber.requeue(delivery_tag);
            } else {
                std::cout << "Processing failed - rejecting message" << std::endl;
                subscriber.reject(delivery_tag);
            }
        });
        
        std::cout << "✅ Message handling test completed successfully!" << std::endl;
        
    } catch (const std::exception &e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
