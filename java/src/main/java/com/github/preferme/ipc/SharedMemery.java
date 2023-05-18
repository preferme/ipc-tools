package com.github.preferme.ipc;

public class SharedMemery {

    /**
     * 加载动态链接库
     */
    static {
//        System.loadLibrary("ipc-tools");
        EmbeddedLoader.load("ipc-tools");
    }

    private final String name;
    private final int length;
    private int fd;
    private long address;

    /**
     * 实例化 共享内存映射 的对象
     * @param name 命名
     * @param length 共享内存的大小，一般为 32K 的倍数
     */
    public SharedMemery(String name, int length) {
        this.name = name;
        this.length = length;
        initialize();
    }

    /**
     * 初始化操作。创建fd，映射共享内存，设置互斥锁，设置读写标记。
     */
    private native void initialize();

    /**
     * 回收资源。销毁互斥锁，销毁共享内存。
     */
    public native void destroy();

    /**
     * 写入标记
     */
    public native int writerIndex();
    /**
     * 可写入的字节数
     */
    public native int writableBytes();
    /**
     * 读取标记
     */
    public native int readerIndex();
    /**
     * 可读取的字节数
     */
    public native int readableBytes();

    /**
     * 获取指定位置的数据。不会改变 readerIndex 。
     * @param index 指定的位置
     * @param buffer 输出数据的缓冲区
     * @param offset 输出缓冲区的起始偏移量
     * @param length 输出数据的长度
     */
    public native SharedMemery getBytes(int index, byte[] buffer, int offset, int length);
    /**
     * 设置指定位置的数据。不会改变 writerIndex 。
     * @param index 指定的位置
     * @param buffer 输入数据的缓冲区
     * @param offset 输入缓冲区的起始偏移量
     * @param length 输入数据的长度
     */
    public native SharedMemery setBytes(int index, byte[] buffer, int offset, int length);
    /**
     * 读取指定位置的数据。读取标记 readerIndex 会自动后移。
     * @param buffer 输出数据的缓冲区
     * @param offset 输出缓冲区的起始偏移量
     * @param length 输出数据的长度
     */
    public native SharedMemery readBytes(byte[] buffer, int offset, int length);
    /**
     * 写入指定位置的数据。写入标记 writerIndex 会自动后移。
     * @param buffer 输入数据的缓冲区
     * @param offset 输入缓冲区的起始偏移量
     * @param length 输入数据的长度
     */
    public native SharedMemery writeBytes(byte[] buffer, int offset, int length);

}

