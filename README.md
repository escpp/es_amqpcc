# AMQP-CPP 安装和使用指南

## 环境准备

### 1. 克隆 AMQP-CPP 库

```bash
git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
cd AMQP-CPP
```

### 2. 安装 libevent 依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install libevent-dev

# CentOS/RHEL
sudo yum install libevent-devel

# 或者从源码编译安装
wget https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
tar -xzf libevent-2.1.12-stable.tar.gz
cd libevent-2.1.12-stable
./configure
make
sudo make install
```

### 3. 编译安装 AMQP-CPP

```bash
# 创建构建目录
mkdir build
cd build

# 配置编译选项
cmake .. -DAMQP-CPP_LINUX_TCP=ON

# 编译
make

# 安装
sudo make install

# 设置库路径（如果需要）
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### 4. 验证安装

```bash
# 检查头文件
ls /usr/local/include/amqpcpp/

# 检查库文件
ls /usr/local/lib/libamqpcpp*
```

## 示例代码说明

本目录包含两个示例程序：

### 生产者 (producer.cc)
- 连接到 RabbitMQ 服务器
- 声明 "test-exchange" 交换器
- 发布消息到交换器
- 支持消息确认机制

### 消费者 (consumer.cc) 
- 连接到 RabbitMQ 服务器
- 声明 "test-exchange" 交换器和 "test-queue" 队列
- 绑定队列到交换器
- 消费消息并处理不同场景：
  - 处理成功：确认消息
  - 处理失败：重新排队重试
  - 严重错误：丢弃消息

## 编译运行示例

### 编译生产者
```bash
g++ -std=c++11 -o producer producer.cc -lamqpcpp -levent
```

### 编译消费者
```bash
g++ -std=c++11 -o consumer consumer.cc -lamqpcpp -levent
```

### 运行示例
1. 首先启动 RabbitMQ 服务器
2. 运行消费者程序：`./consumer`
3. 运行生产者程序：`./producer`

## 配置说明

默认连接参数：
- 主机：localhost
- 端口：5672
- 用户名：guest
- 密码：guest
- 虚拟主机：/

可以通过修改代码中的连接字符串来更改这些参数。

## 故障排除

### 常见问题

1. **连接失败**
   - 检查 RabbitMQ 服务是否运行：`sudo systemctl status rabbitmq-server`
   - 确认防火墙设置

2. **库找不到错误**
   - 设置 LD_LIBRARY_PATH：`export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH`
   - 或者添加到 /etc/ld.so.conf.d/ 并运行 `sudo ldconfig`

3. **编译错误**
   - 确认 libevent-dev 已安装
   - 检查编译器版本支持 C++11

### 调试模式

编译时添加调试信息：
```bash
g++ -std=c++11 -g -o producer producer.cc -lamqpcpp -levent
```

## 相关资源

- [AMQP-CPP GitHub](https://github.com/CopernicaMarketingSoftware/AMQP-CPP)
- [RabbitMQ 文档](https://www.rabbitmq.com/documentation.html)
- [libevent 文档](http://www.wangafu.net/~nickm/libevent-book/)
