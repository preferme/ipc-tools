package com.github.preferme.ipc;

import org.junit.Test;

import static org.junit.Assert.*;

public class SharedMemeryTest {

    @Test
    public void destroy() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        memery.destroy();
    }

    @Test
    public void writerIndex() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        int writerIndex = memery.writerIndex();
        System.out.println("writerIndex : " + writerIndex);
    }

    @Test
    public void writableBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        int writableBytes = memery.writableBytes();
        System.out.println("writableBytes : " + writableBytes);
    }

    @Test
    public void readerIndex() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        int readerIndex = memery.readerIndex();
        System.out.println("readerIndex : " + readerIndex);
    }

    @Test
    public void readableBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        int readableBytes = memery.readableBytes();
        System.out.println("readableBytes : " + readableBytes);
    }

    @Test
    public void getBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        byte[] buffer = new byte[6];
        memery.getBytes(1, buffer, 0, buffer.length);
        System.out.println(Hex.toString(buffer, 0, buffer.length));
    }

    @Test
    public void setBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        byte[] buffer = new byte[] {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06
        };
        memery.setBytes(1, buffer, 0, buffer.length);
    }

    @Test
    public void readBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        int readableBytes = memery.readableBytes();
        byte[] buffer = new byte[readableBytes];
        memery.readBytes(buffer, 0, buffer.length);
        System.out.println(Hex.toString(buffer, 0, buffer.length));
    }

    @Test
    public void writeBytes() {
        SharedMemery memery = new SharedMemery("abc", 4*1024);
        byte[] buffer = new byte[] {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06
        };
        memery.writeBytes(buffer, 0, buffer.length);
        memery.writeBytes(buffer, 0, buffer.length);
    }
}