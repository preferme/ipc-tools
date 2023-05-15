# 一个简单的 IPC 工具，暂时只实现 共享内存映射 技术

## 共享内存映射
使用场景是 nodejs 与 java 直接实现数据的双攻通信
 - 需要带命名的共享内存映射
 - 数据读写需要互斥锁
 - 互斥锁要被多进程访问，所以，要保存在共享内存当中来使用 
 
 
 javah -jni com.github.preferme.ipc.SharedMemery

plugins {
id 'java'
id 'cpp-library'
id 'idea'
}

group 'com.github.preferme'
version '1.0'

sourceCompatibility '1.8'
targetCompatibility '1.8'

[compileJava, compileTestJava]*.options.collect {options -> options.encoding = 'UTF-8'}


repositories {
mavenCentral()
}

dependencies {
testCompile group: 'junit', name: 'junit', version: '4.12'
}

library {
linkage = [Linkage.SHARED]
targetMachines = [
machines.linux.x86_64,
machines.windows.x86, machines.windows.x86_64,
machines.macOS.x86_64
]
}


def javaHome = System.getenv('JAVA_HOME')
libraries {
main {
spec {
args '-W', '-Wall', '-Wextra',
'-static-libgcc', 				// static link libgcc so everything is standalone
'-Wl,--add-stdcall-alias',		// ensure symbols can be found
"-I${javaHome}/include", "-I${javaHome}/include/win32",	// ugly have to include two directories depending on os
"-I${buildDir}/generated"
outputFileName = 'foo.dll'
}
}
}

// must be same as the logic that loads the library from the jar
String osName = System.getProperty("os.name");
if(osName.toLowerCase().matches(".*windows.*")) osName = "Windows"
String osArch = System.getProperty("os.arch");

jar {
into("lib/${osName}-${osArch}", { from(libraries.main.spec.outputFile) })

    manifest {
        attributes 'Main-Class': 'eu.knightswhosay.demo.gradlejni.JNIFoo'
    }
}

// hack..
task javah {
// I guess this can be done a bit neater...
String inputClass = 'com.github.preferme.ipc.SharedMemery';
String inputClassPath = inputClass.replaceAll('\\.', '/')+".class"
String outputFile = "${buildDir}/generated/SharedMemery.h"
inputs.file sourceSets.main.output.asFileTree.matching {
include inputClassPath
}
outputs.file outputFile
doLast {
ant.javah(class: inputClass, outputFile: outputFile, classpath:sourceSets.main.output.asPath)
}
}

// before we can compile javah must have built the header
compileMain.dependsOn javah

