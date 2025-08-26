#pragma once
#include <string>
#include <functional>
#include <memory>
#include <uv.h>
#include <amqpcpp.h>
#include <amqpcpp/libuv.h>
#include <amqpcpp/linux_tcp/tcpchannel.h>

namespace es_amqpcc
{
    /**
     * @brief AMQP订阅者类，用于消费消息（libuv驱动）
     */
    class LibuvSubscriber
    {
    private:
        uv_loop_t *_loop;                                 ///< libuv事件循环
        std::unique_ptr<AMQP::LibUvHandler> _handler;     ///< libuv处理器
        std::unique_ptr<AMQP::TcpConnection> _connection; ///< AMQP连接
        std::unique_ptr<AMQP::TcpChannel> _channel;       ///< AMQP通道
        std::string _exchange_name;                       ///< 交换器名称
        std::string _routing_key;                         ///< 路由键
        std::string _queue_name;                          ///< 队列名称
        std::string _consumer_tag;                        ///< 消费者标签
        size_t _message_count;                            ///< 接收的消息计数

        std::function<void()> _on_connect_success;                               ///< 连接成功回调
        std::function<void(const std::string &)> _on_connect_error;              ///< 连接失败回调
        std::function<void(const AMQP::Message &, uint64_t, bool)> _on_received; ///< 消息接收回调
        std::function<void(const std::string &)> _on_consume_start_success;      ///< 消费成功回调
        std::function<void(const std::string &)> _on_consume_start_error;        ///< 消费错误回调

        /**
         * @brief 自定义libuv处理器
         */
        class SubscriberHandler : public AMQP::LibUvHandler
        {
        private:
            LibuvSubscriber *_parent = nullptr; ///< 父subscriber对象

        public:
            /**
             * @brief 构造函数
             * @param loop libuv事件循环
             * @param parent 父subscriber对象
             */
            SubscriberHandler(uv_loop_t *loop, LibuvSubscriber *parent)
                : AMQP::LibUvHandler(loop), _parent(parent) {}

            /**
             * @brief 连接错误回调
             */
            virtual void onError(AMQP::TcpConnection *connection, const char *message) override
            {
                if (_parent && _parent->_on_connect_error)
                {
                    _parent->_on_connect_error(message);
                }
            }

            /**
             * @brief 连接成功回调
             */
            virtual void onConnected(AMQP::TcpConnection *connection) override
            {
                if (_parent && _parent->_on_connect_success)
                {
                    _parent->_on_connect_success();
                }
            }
        };

    public:
        /**
         * @brief 构造函数
         *
         * @param exchange_name 交换器名称
         * @param routing_key 路由键
         * @param queue_name 队列名称
         */
        LibuvSubscriber(const std::string &exchange_name,
                        const std::string &routing_key,
                        const std::string &queue_name)
            : _loop(uv_default_loop()),
              _exchange_name(exchange_name),
              _routing_key(routing_key),
              _queue_name(queue_name),
              _message_count(0)
        {
        }

        /**
         * @brief 连接到AMQP服务器
         *
         * @param amqp_url AMQP连接URL（格式：amqp://rabbitmq:rabbitmq@localhost/）
         * @return true 连接初始化成功
         * @return false 连接初始化失败
         */
        bool connect(const std::string &amqp_url)
        {
            try
            {
                // 创建libuv处理器
                _handler.reset(new SubscriberHandler(_loop, this));

                // 创建连接
                _connection.reset(new AMQP::TcpConnection(_handler.get(), AMQP::Address(amqp_url)));

                // 创建通道
                _channel.reset(new AMQP::TcpChannel(_connection.get()));

                // 声明交换器（如果不存在则创建）
                _channel->declareExchange(_exchange_name, AMQP::direct)
                    .onSuccess([&](){
                        // 交换器声明成功
                        std::cout << "Exchange declared: " << _exchange_name << std::endl;
                    })
                    .onError([](const char *message){
                        std::cout << "Exchange declaration failed: " << message << std::endl;
                    })
                    ;

                // 声明队列
                _channel->declareQueue(_queue_name, AMQP::durable)
                    .onSuccess([](const std::string &name, uint32_t messageCount, uint32_t consumerCount){
                        // 队列声明成功 
                        std::cout << "Queue declared: " << name << std::endl;
                    })
                    .onError([](const char *message){
                        std::cout << "Queue declaration failed: " << message << std::endl;
                    })
                    ;

                // 绑定队列到交换器
                _channel->bindQueue(_exchange_name, _queue_name, _routing_key)
                .onSuccess([&](){
                    // 队列绑定成功
                    std::cout << "Queue bound: "
                        << "_exchange_name=" << _exchange_name
                        << ", _queue_name=" << _queue_name
                        << ", _routing_key" << _routing_key  << std::endl;
                })
                .onError([](const char *message){
                    std::cout << "Queue binding failed: " << message << std::endl;
                })
                ;

                _consumer_tag = _channel->consume(_queue_name)
                                    // 开始消费消息
                                    .onReceived([this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
                                                {
                            _message_count++;
                            if (_on_received) {
                                _on_received(message, deliveryTag, redelivered);
                            } })

                                    // 消费开始成功
                                    .onSuccess([this](const std::string &consumerTag)
                                               {
                            if (_on_consume_start_success) {
                                _on_consume_start_success(consumerTag);
                            } })

                                    // 消费开始失败
                                    .onError([this](const char *message)
                                             {
                            if (_on_consume_start_error) {
                                _on_consume_start_error(message);
                            } });

                return true;
            }
            catch (const std::exception &e)
            {
                if (_on_connect_error)
                {
                    _on_connect_error(std::string("Connection failed: ") + e.what());
                }
                return false;
            }
        }

        /**
         * @brief 设置连接成功回调
         * @param callback 回调函数
         */
        void on_connect_success(std::function<void()> callback)
        {
            _on_connect_success = callback;
        }

        /**
         * @brief 设置连接失败回调
         * @param callback 回调函数，接收错误消息
         */
        void on_connect_error(std::function<void(const std::string &)> callback)
        {
            _on_connect_error = callback;
        }

        /**
         * @brief 设置消息接收回调
         * @param callback 回调函数，接收消息内容、投递标签、重投递标志和Subscriber引用
         */
        void on_received(std::function<void(const AMQP::Message &, uint64_t, bool)> callback)
        {
            _on_received = callback;
        }

        /**
         * @brief 设置消费成功回调
         * @param callback 回调函数，接收消费者标签
         */
        void on_consume_success(std::function<void(const std::string &)> callback)
        {
            _on_consume_start_success = callback;
        }

        /**
         * @brief 设置消费错误回调
         * @param callback 回调函数，接收错误消息
         */
        void on_consume_error(std::function<void(const std::string &)> callback)
        {
            _on_consume_start_error = callback;
        }

        /**
         * @brief 确认消息
         * @param deliveryTag 投递标签
         * @return true 确认成功，false 确认失败
         */
        bool ack(uint64_t deliveryTag) const
        {
            if (_channel)
            {
                return _channel->ack(deliveryTag);
            }
            return false;
        }

        /**
         * @brief 拒绝消息（丢弃）
         * @param deliveryTag 投递标签
         * @return true 拒绝成功，false 拒绝失败
         */
        bool reject(uint64_t deliveryTag) const
        {
            if (_channel)
            {
                return _channel->reject(deliveryTag, 0);
            }
            return false;
        }

        /**
         * @brief 拒绝消息并重入队
         * @param deliveryTag 投递标签
         * @return true 操作成功，false 操作失败
         */
        bool requeue(uint64_t deliveryTag) const
        {
            if (_channel)
            {
                return _channel->reject(deliveryTag, AMQP::requeue);
            }
            return false;
        }

        /**
         * @brief 获取接收到的消息数量
         *
         * @return size_t 消息数量
         */
        size_t get_message_number() const
        {
            return _message_count;
        }

        /**
         * @brief 运行事件循环
         */
        bool run()
        {
            if (nullptr == _loop)
            {
                return false;
            }

            if (_loop)
            {
                uv_run(_loop, UV_RUN_DEFAULT);
                return true;
            }

            return false;
        }

        /**
         * @brief 获取交换器名称
         *
         * @return const std::string& 交换器名称
         */
        const std::string &exchange_name() const
        {
            return _exchange_name;
        }

        /**
         * @brief 获取路由键
         *
         * @return const std::string& 路由键
         */
        const std::string &routing_key() const
        {
            return _routing_key;
        }

        /**
         * @brief 获取队列名称
         *
         * @return const std::string& 队列名称
         */
        const std::string &queue_name() const
        {
            return _queue_name;
        }

        // 禁用拷贝构造和赋值
        LibuvSubscriber(const LibuvSubscriber &) = delete;
        LibuvSubscriber &operator=(const LibuvSubscriber &) = delete;
    };

} // namespace es_amqpcc
