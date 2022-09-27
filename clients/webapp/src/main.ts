import { pwaInfo } from "virtual:pwa-info";
import { io } from "socket.io-client";

// eslint-disable-next-line no-console
console.log(pwaInfo);

// socket url is same url as the origin of this page
const socket = io();

socket.on("connect", () => {
	console.log("connected");
	socket.emit("hello", "world");
});

document.querySelector<HTMLDivElement>("#app")!.innerHTML = `
  <div>
   <img src="/favicon.svg" alt="PWA Logo" width="60" height="60">
    <h1>Vite + TypeScript</h1>
    <p>Testing SW with <b>injectRegister=auto,inline,script</b></p>
  </div>
`;
