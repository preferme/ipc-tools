package com.github.preferme.ipc;

import java.nio.ByteBuffer;

@FunctionalInterface
public interface ByteBufferFactory {

    ByteBuffer create(int capacity);

    ByteBufferFactory Default = (capacity -> ByteBuffer.allocate(capacity));

}
