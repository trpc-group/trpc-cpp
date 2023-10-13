### future/promise常见问题

#### GetValue/GetException接口调用core

- 原因：接口用法有误。
- 解决办法：注意以下两点
  1. 在调用Future对象的GetValue/GetException前需要先判断Future的状态(IsReady或IsFailed)，然后再调用相应的接口获取Value或Exception
  2. 同时，对一个Future对象只能调用一次GetValue和GetException(因为接口内部实现使用了移动语义，直接将内部的value move走了)

#### 什么场景可以使用无返回值的Then作为回调，什么场景不行

- 回答：全异步场景（无BlockingGet操作，不考虑future是否执行完成的场景），链式调用的最后一环可以使用无返回值的Then作为回调的收尾，从而避免构建一次future对象。因为不考虑future是否执行完成，所以使用无返回值的Then时，无法作为WhenAll或WhenAny的条件，但是仍然可以作为WhenAll或者WhenAny调用的最后一环，即WhenAll().Then()或WhenAny.Then()。

#### 异步调用时，每次调用下游都通过GetProxy获取一个proxy，导致服务core

- 原因：GetProxy获取的临时变量proxy在回包还没有处理完时，就释放了，但是回包处理时还需要用到proxy的成员connection,所以导致core。
- 解决办法：一个client/service对应一个proxy，用户自己保存起来，不能每次都创建。
