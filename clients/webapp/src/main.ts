import SocketConnection from "./SocketConnnection";

const socketConnection = new SocketConnection();

socketConnection.on(SocketConnection.ON_CONNECT, () => {
	console.log("connected");
	// change bg of body to green

	document.body.style.backgroundColor = "green";
});

socketConnection.on(SocketConnection.ON_DISCONNECT, () => {
	console.log("disconnected");
	// change bg of body to red
	document.body.style.backgroundColor = "red";
});

document.querySelector<HTMLDivElement>("#app")!.innerHTML = `
  <div>
   <img src="/favicon.svg" alt="PWA Logo" width="60" height="60">
    <h1>Vite + TypeScript</h1>
    <p>Testing SW with <b>injectRegister=auto,inline,script</b></p>
  </div>
`;
