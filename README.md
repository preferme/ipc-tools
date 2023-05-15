# 一个简单的 IPC 工具，暂时只实现 共享内存映射 技术

## 共享内存映射
使用场景是 nodejs 与 java 直接实现数据的双攻通信
 - 需要带命名的共享内存映射
 - 数据读写需要互斥锁
 - 互斥锁要被多进程访问，所以，要保存在共享内存当中来使用 
 
 
 javah -jni com.github.preferme.ipc.SharedMemery