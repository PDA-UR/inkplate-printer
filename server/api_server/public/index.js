import hello from "/public/src/test.js";

var socket = io();

socket.on("connect", () => {
	console.log("connected");
	socket.emit("hello", hello());
});
console.log(hello());

export {};
