package com.github.preferme.ipc;

public class Main {

    public static void main(String[] args) {

//        EmbeddedLoader.load("ipc-tools");
        SharedMemery memery = new SharedMemery("/abcdef", 4* 1024);

    }

}
