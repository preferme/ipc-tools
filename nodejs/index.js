var ipc_tools = require('bindings')('ipc-tools');

console.log(ipc_tools);

var obj = ipc_tools.makeSharedMemory("abcde", 4096);
console.log( "obj = ", obj ); // 11
console.log( "writableBytes = ",obj.writableBytes() ); // 12
console.log( "readableBytes = ",obj.readableBytes() );
// obj.destroy();

// var obj2 = ipc_tools.makeSharedMemory("abcdefg", 10);
// console.log( obj2 ); // 21
// console.log( obj2.readableBytes() ); // 22

console.log("hello.");

