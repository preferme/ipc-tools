package com.github.preferme.ipc;

import org.junit.jupiter.api.Test;

import java.io.File;
import java.io.InputStream;
import java.net.URL;

import static org.junit.jupiter.api.Assertions.*;

class SharedMemeryTest {

    @Test
    void init() {
//        EmbeddedLoader.load("ipc-tools");
        SharedMemery memery = new SharedMemery("/abc", 4 * 1024);

    }

    @Test
    void testFinalize() {
    }

    @Test
    void destroy() {
    }

    @Test
    void writerIndex() {
    }

    @Test
    void writableBytes() {
    }

    @Test
    void readerIndex() {
    }

    @Test
    void readableBytes() {
    }

    @Test
    void getBytes() {
    }

    @Test
    void setBytes() {
    }

    @Test
    void readBytes() {
    }

    @Test
    void writeBytes() {
    }
}