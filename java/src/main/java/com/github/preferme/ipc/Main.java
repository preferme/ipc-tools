package com.github.preferme.ipc;

public class Main {

    public static void main(String[] args) {

//        EmbeddedLoader.load("ipc-tools");
        SharedMemery memery = new SharedMemery("abc", 4* 1024);
        System.out.println("memery" + memery);
memery.destroy();
    }

}
