import { io } from "socket.io-client";
import { Observable } from "./lib/Observable";

export default class SocketConnection extends Observable {
	public static ON_DISCONNECT = "disconnect";
	public static ON_CONNECT = "connect";

	private readonly socket = io();

	constructor() {
		super();
		this.registerEvents();
	}

	public connect(): void {
		this.socket.connect();
	}

	public isConnected = (): boolean => {
		return this.socket.connected;
	};

	private registerEvents = (): void => {
		this.socket.on(SocketConnection.ON_CONNECT, () => {
			this.notifyAll(SocketConnection.ON_CONNECT);
		});

		this.socket.on(SocketConnection.ON_DISCONNECT, () => {
			this.notifyAll(SocketConnection.ON_DISCONNECT);
		});
	};
}
