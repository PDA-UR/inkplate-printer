import { io } from "socket.io-client";
import { Observable } from "../lib/Observable";

export default class SocketController extends Observable {
	public static ON_DISCONNECT = "disconnect";
	public static ON_CONNECT = "connect";

	public static ON_UPDATE_DEVICE_INDEX = "updateDeviceIndex";
	public static ON_SHOW_PAGE = "showPage";

	private readonly socket = io();

	constructor() {
		super();
		this.registerEvents();
	}

	// basic

	public connect(): void {
		this.socket.connect();
	}

	public isConnected = (): boolean => {
		return this.socket.connected;
	};

	// register

	public sendRegisterRequest(uuid: string): void {
		console.log("sendRegisterRequest");
		this.socket.emit("registerRequest", {
			screenResolution: {
				width: window.innerWidth,
				height: window.innerHeight,
			},
			uuid,
		});
	}

	public sendUnregisterRequest(uuid: string): void {
		console.log("sendUnregisterRequest");
		this.socket.emit("unRegisterRequest", {
			uuid,
		});
	}

	private registerEvents = (): void => {
		this.socket.on(SocketController.ON_CONNECT, () => {
			this.notifyAll(SocketController.ON_CONNECT);
		});

		this.socket.on(SocketController.ON_DISCONNECT, () => {
			this.notifyAll(SocketController.ON_DISCONNECT);
		});
		this.socket.on("showPage", this.onShowPage);
		this.socket.on("updateDeviceIndex", this.onUpdateDeviceIndex);
	};

	// page show
	private onShowPage = (pageIndex: number): void => {
		console.log("onShowPage", pageIndex);
		this.notifyAll(SocketController.ON_SHOW_PAGE, pageIndex);
	};

	// device index
	private onUpdateDeviceIndex = (deviceIndex: number): void => {
		console.log("onUpdateDeviceIndex", deviceIndex);
		this.notifyAll(SocketController.ON_UPDATE_DEVICE_INDEX, deviceIndex);
	};
}
