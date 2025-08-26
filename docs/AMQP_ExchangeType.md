# ExchangeType的枚举类型
```c++
/**
 *  @param fanout（扇出）：将消息发送到所有绑定的队列，忽略路由键。
 *  @param direct（直接）：将消息发送到路由键完全匹配的队列。
 *  @param topic（主题）：基于模式匹配路由键，支持通配符。
 *  @param headers（头部）：根据消息的头部属性进行路由，而不是路由键。
 *  @param consistent_hash（一致性哈希）：使用一致性哈希算法进行消息路由，常用于分布式系统中。
 *  @param message_deduplication（消息去重）：可能是一种支持消息去重功能的交换机类型。
 */
enum ExchangeType
{
    fanout,
    direct,
    topic,
    headers,
    consistent_hash,
    message_deduplication
};
```
