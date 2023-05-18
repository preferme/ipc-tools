package com.github.preferme.ipc;

import org.junit.Test;

import static org.junit.Assert.*;

public class EmbeddedLoaderTest {

    @Test
    public void load() {
        EmbeddedLoader.load("ipc-tools");
    }
}