#!/bin/bash

# AMQP-CPP 示例编译测试脚本

echo "=== AMQP-CPP 示例编译测试 ==="

# 检查依赖
echo "检查 libevent 头文件..."
if [ -d "/usr/local/include/event2" ]; then
    echo "✓ libevent 头文件存在 (/usr/local/include/event2)"
    EVENT_INCLUDE="/usr/local/include"
elif [ -d "/usr/include/event2" ]; then
    echo "✓ libevent 头文件存在 (/usr/include/event2)"
    EVENT_INCLUDE="/usr/include"
else
    echo "✗ libevent 头文件未找到，请安装 libevent-dev"
    exit 1
fi

# 编译生产者
echo "编译生产者..."
cd /home/zhangshichong/test/github/AMQP-CPP
g++ -I include -I $EVENT_INCLUDE -c es_amqpcc/examples/producer.cc -o es_amqpcc/examples/producer.o 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ 生产者编译成功"
    rm es_amqpcc/examples/producer.o
else
    echo "✗ 生产者编译失败"
    exit 1
fi

# 编译消费者
echo "编译消费者..."
g++ -I include -I $EVENT_INCLUDE -c es_amqpcc/examples/consumer.cc -o es_amqpcc/examples/consumer.o 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ 消费者编译成功"
    rm es_amqpcc/examples/consumer.o
else
    echo "✗ 消费者编译失败"
    exit 1
fi

echo ""
echo "=== 编译测试完成 ==="
echo "所有示例代码编译成功！"
echo ""
echo "下一步："
echo "1. 确保 RabbitMQ 服务器正在运行"
echo "2. 运行消费者：g++ -std=c++11 -o consumer es_amqpcc/examples/consumer.cc -lamqpcpp -levent && ./consumer"
echo "3. 运行生产者：g++ -std=c++11 -o producer es_amqpcc/examples/producer.cc -lamqpcpp -levent && ./producer"
