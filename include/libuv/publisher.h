#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <functional>
#include <memory>
#include <uv.h>
#include <amqpcpp.h>
#include <amqpcpp/libuv.h>
#include <amqpcpp/linux_tcp/tcpchannel.h>
#include "json.h"

namespace es_amqpcc
{
    /**
     * @brief AMQP交换器类（libuv驱动）
     */
    class LibuvPublisher
    {
    private:
        bool _is_running = false;
        uv_loop_t *_loop = nullptr;                       ///< libuv事件循环
        std::unique_ptr<AMQP::LibUvHandler> _handler;     ///< libuv处理器
        std::unique_ptr<AMQP::TcpConnection> _connection; ///< AMQP连接
        std::unique_ptr<AMQP::TcpChannel> _channel;       ///< AMQP通道
        std::string _exchange_name;                       ///< 交换器名称
        std::string _routing_key;                         ///< 路由键
        size_t _message_count = 0;                        ///< 发布的消息计数
        bool _connected = false;                          ///< 连接状态

        std::function<void()> _on_connect_success;                  ///< 连接成功回调
        std::function<void()> _on_close_success;                    ///< 连接关闭回调
        std::function<void()> _on_publish_success;                  ///< 消息发布成功回调
        std::function<void()> _on_declare_success;                  ///< 交换器声明成功回调
        std::function<void(const std::string &)> _on_connect_error; ///< 连接错误回调
        std::function<void(const std::string &)> _on_publish_error; ///< 发布错误回调
        std::function<void(const std::string &)> _on_declare_error; ///< 交换器声明错误回调

        /**
         * @brief 自定义libuv处理器
         */
        class ExchangeHandler : public AMQP::LibUvHandler
        {
        private:
            LibuvPublisher *_parent = nullptr; ///< 父exchange对象

        public:
            /**
             * @brief 构造函数
             * @param loop libuv事件循环
             * @param parent 父exchange对象
             */
            ExchangeHandler(uv_loop_t *loop, LibuvPublisher *parent)
                : AMQP::LibUvHandler(loop), _parent(parent) {}

            /**
             * @brief 连接错误回调
             */
            virtual void onError(AMQP::TcpConnection *connection, const char *message) override
            {
                std::cout << "Connection error: " << message << std::endl;
                if (_parent)
                {
                    if (_parent->_on_connect_error)
                    {
                        _parent->_on_connect_error(message);
                    }
                }
            }

            /**
             * @brief 连接成功回调
             */
            virtual void onConnected(AMQP::TcpConnection *connection) override
            {
                std::cout << "Connection error: connection=" << connection << std::endl;
                if (_parent)
                {
                    _parent->_connected = true;
                    std::cout << "Connected to AMQP server. _parent->_connected=" << _parent->_connected << std::endl;
                    if (_parent->_on_connect_success)
                    {
                        _parent->_on_connect_success();
                    }
                }
            }

            /**
             * @brief 连接关闭回调
             */
            virtual void onClosed(AMQP::TcpConnection *connection) override
            {
                std::cout << "Connection closed" << std::endl;
                if (_parent)
                {
                    _parent->_connected = false;
                    if (_parent->_on_close_success)
                    {
                        _parent->_on_close_success();
                    }
                }
            }
        };

    public:
        /**
         * @brief 构造函数
         *
         * @param exchange_name 交换器名称
         * @param routing_key 路由键
         */
        explicit LibuvPublisher(const std::string &exchange_name, const std::string &routing_key)
            : _loop(uv_default_loop()),
              _exchange_name(exchange_name),
              _routing_key(routing_key),
              _message_count(0),
              _connected(false)
        {
        }

        /**
         * @brief 连接到AMQP服务器并声明交换器
         *
         * @param amqp_url AMQP连接URL（格式：amqp://user:pwd@host/）
         * @param timeout_ms 连接超时时间（毫秒），默认30秒
         * @return true 连接成功并声明交换器
         * @return false 连接失败或声明交换器失败
         */
        bool connect(const std::string &amqp_url)
        {
            if (_connected)
            {
                std::cout << "Already connected" << std::endl;
                return true; // 已经连接
            }

            try
            {
                // 创建libuv处理器
                _handler = std::make_unique<ExchangeHandler>(_loop, this);

                // 创建连接
                _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), AMQP::Address(amqp_url));

                // 创建通道
                _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());

                // _channel->removeExchange(_exchange_name)
                //     .onSuccess([this]()
                //                {
                //             std::cout << "Exchange '" << _exchange_name << "' removed successfully" << std::endl;
                //              })
                //     .onError([this](const char *message) {
                //             std::cout << "Exchange '" << _exchange_name << "' removal failed: " << message << std::endl;
                //     })
                //     .onFinalize([](){ 
                //         std::cout << "Exchange removal finalized" << std::endl;
                //     })
                //     ;
                // std::cout << "Exchange removed" << std::endl;

                // 声明交换器
                _channel->declareExchange(_exchange_name, AMQP::direct)
                    .onSuccess([this]()
                               {
                            std::cout << "Exchange '" << _exchange_name << "' declared successfully" << std::endl;
                            if (_on_declare_success) {
                                _on_declare_success();
                            } })
                    .onError([this](const char *message)
                             {
                            if (_on_declare_error) {
                                _on_declare_error(std::string("Exchange declaration failed: ") + message);
                            } })
                            ;

                _connected = true;
                return true;
            }
            catch (const std::exception &e)
            {
                if (_on_connect_error)
                {
                    _on_connect_error(std::string("Connect failed: ") + e.what());
                }
                return false;
            }
        }

        /**
         * @brief 关闭连接
         *
         * @return true 关闭成功
         * @return false 关闭失败
         */
        bool close(void)
        {
            if (!_connected)
            {
                return true; // 已经关闭
            }

            try
            {
                _channel.reset();
                _connection.reset();
                _handler.reset();
                _connected = false;
                return true;
            }
            catch (const std::exception &e)
            {
                if (_on_connect_error)
                {
                    _on_connect_error(std::string("Close failed: ") + e.what());
                }
                return false;
            }
        }

        /**
         * @brief 发布消息到交换器
         *
         * @param msg 要发布的消息内容
         * @return true 发布成功
         * @return false 发布失败
         */
        bool publish(const std::string &msg)
        {
            if (!_channel || !_connected)
            {
                std::cout << "_channel=" << _channel.get() << ", _connected=" << _connected << std::endl;
                if (_on_publish_error)
                {
                    _on_publish_error("Publish failed: channel is not open");
                }
                return false;
            }

            try
            {
                // 直接发布消息（不使用信封）
                bool ret = _channel->publish(_exchange_name, _routing_key, msg);
                if (false == ret)
                {
                    if (_on_publish_error)
                    {
                        _on_publish_error("Publish failed: channel is not open");
                    }
                    return false;
                }
                _message_count++;

                // 触发发布成功回调
                if (_on_publish_success)
                {
                    _on_publish_success();
                }

                return true;
            }
            catch (const std::exception &e)
            {
                // 发布失败
                if (_on_publish_error)
                {
                    _on_publish_error(std::string("Publish failed: ") + e.what());
                }
                return false;
            }
        }

        /**
         * @brief 发布JSON消息到交换器
         *
         * @param msg 要发布的JSON消息
         * @return true 发布成功
         * @return false 发布失败
         */
        bool publish(const es_amqpcc::json &msg)
        {
            return publish(msg.dump());
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
         * @brief 设置连接关闭回调
         * @param callback 回调函数
         */
        void on_closed(std::function<void()> callback)
        {
            _on_close_success = callback;
        }

        /**
         * @brief 设置消息发布成功回调
         * @param callback 回调函数
         */
        void on_publish_success(std::function<void()> callback)
        {
            _on_publish_success = callback;
        }

        /**
         * @brief 设置连接错误回调
         * @param callback 回调函数
         */
        void on_connect_error(std::function<void(const std::string &)> callback)
        {
            _on_connect_error = callback;
        }

        /**
         * @brief 设置发布错误回调
         * @param callback 回调函数
         */
        void on_publish_error(std::function<void(const std::string &)> callback)
        {
            _on_publish_error = callback;
        }

        /**
         * @brief 设置交换器声明成功回调
         * @param callback 回调函数
         */
        void on_declare_success(std::function<void()> callback)
        {
            _on_declare_success = callback;
        }

        /**
         * @brief 设置交换器声明错误回调
         * @param callback 回调函数
         */
        void on_declare_error(std::function<void(const std::string &)> callback)
        {
            _on_declare_error = callback;
        }

        /**
         * @brief 运行事件循环
         * @return true 事件循环成功运行，false 运行失败
         */
        bool run(void)
        {
            if (nullptr == _loop || _is_running) {
                return false;
            }
       
            _is_running = true;
            std::thread([&](){
                bool ret = uv_run(_loop, UV_RUN_DEFAULT);
                std::cout << "uv_run return ret=" << ret << std::endl;
            }).detach();

            return true;
        }

        /**
         * @brief 获取已发布的消息数量
         *
         * @return size_t 消息数量
         */
        size_t size() const
        {
            return _message_count;
        }

        /**
         * @brief 获取交换器名称
         *
         * @return const std::string& 交换器名称
         */
        const std::string &name() const
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

        // 禁用拷贝构造和赋值
        LibuvPublisher(const LibuvPublisher &) = delete;
        LibuvPublisher &operator=(const LibuvPublisher &) = delete;
    };

} // namespace es_amqpcc
