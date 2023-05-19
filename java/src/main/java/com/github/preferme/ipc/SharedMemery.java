package com.github.preferme.ipc;

import java.nio.ByteBuffer;
import java.util.function.Supplier;

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
     * 可写入的字节数
     */
    public native int writableBytes();

    /**
     * 可读取的字节数
     */
    public native int readableBytes();

    public native ByteBuffer read(ByteBufferFactory factory);

    public ByteBuffer read() {
        return read(ByteBufferFactory.Default);
    }

    public native boolean write(ByteBuffer buffer);

}

