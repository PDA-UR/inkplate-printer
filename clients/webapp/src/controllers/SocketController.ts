import { io } from "socket.io-client";
import DataManager from "../data/DataManager";
import { Observable } from "../lib/Observable";

export default class SocketController extends Observable {
	public static ON_DISCONNECT = "disconnect";
	public static ON_CONNECT = "connect";

	public static ON_REGISTERED = "registered";

	public static ON_UPDATE_DEVICE_INDEX = "updateDeviceIndex";
	public static ON_SHOW_PAGE = "showPage";
	public static ON_PAGES_READY = "pagesReady";

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

	// registering

	public sendRegisterMessage(uuid: string): void {
		console.log("register");
		this.socket.emit("register", {
			uuid,
			screenResolution: {
				width: window.innerWidth,
				height: window.innerHeight,
			},
		});
	}

	// queuing

	public sendEnqueueMessage(uuid: string): void {
		console.log("sendEnqueueMessage");
		this.socket.emit("enqueue", {
			screenResolution: {
				width: window.innerWidth,
				height: window.innerHeight,
			},
			uuid,
		});
	}

	public sendDequeueMessage(uuid: string): void {
		console.log("sendDequeueMessage");
		this.socket.emit("dequeue", {
			uuid,
		});
	}

	private registerEvents = (): void => {
		this.socket.on(SocketController.ON_CONNECT, () => {
			DataManager.getUUID().then((uuid) => {
				this.sendRegisterMessage(uuid!);
			});
		});

		this.socket.on(SocketController.ON_REGISTERED, (wasSuccessful) => {
			if (wasSuccessful) this.notifyAll(SocketController.ON_REGISTERED);
		});

		this.socket.on(SocketController.ON_DISCONNECT, () => {
			this.notifyAll(SocketController.ON_DISCONNECT);
		});
		this.socket.on(SocketController.ON_PAGES_READY, this.onPagesReady);
		this.socket.on(SocketController.ON_SHOW_PAGE, this.onShowPage);
		this.socket.on(
			SocketController.ON_UPDATE_DEVICE_INDEX,
			this.onUpdateDeviceIndex
		);
	};

	// pages ready
	private onPagesReady = (numPages: number | undefined): void => {
		console.log("onPagesReady", numPages);
		this.notifyAll(SocketController.ON_PAGES_READY, numPages);
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
