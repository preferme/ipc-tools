package com.github.preferme.ipc;

import org.junit.Test;

import java.nio.ByteBuffer;

import static org.junit.Assert.*;

public class SharedMemeryTest {

    @Test
    public void destroy() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        memery.destroy();
    }

    @Test
    public void writableBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        int writableBytes = memery.writableBytes();
        System.out.println("writableBytes : " + writableBytes);
    }

    @Test
    public void readableBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        int readableBytes = memery.readableBytes();
        System.out.println("readableBytes : " + readableBytes);
    }

    @Test
    public void read() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        ByteBuffer buffer = memery.read();
        System.out.println(Hex.toString(buffer.array()));
    }

    @Test
    public void write() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        byte[] data = new byte[] {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06
        };
        ByteBuffer buffer = ByteBuffer.wrap(data);
        System.out.println("buffer.remaining : " + buffer.remaining());
        int count = memery.write(buffer);
        System.out.println("write count : " + count);
    }

}