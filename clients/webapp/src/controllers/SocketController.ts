import { io } from "socket.io-client";
import DataManager from "../data/DataManager";
import { Observable } from "../lib/Observable";
import ConnectionStatus from "../lib/ConnectionStatus";
import DeviceModel, { getScreenInfo } from "../data/DeviceModel";
import PageChainInfoModel from "../data/PageChainInfoModel";

export default class SocketController extends Observable {
	public static ON_DISCONNECT = "disconnect";
	public static ON_CONNECT = "connect";

	public static ON_REGISTER = "register";
	public static ON_REGISTERED = "registered";

	public static ON_UPDATE_DEVICE_INDEX = "updateDeviceIndex";
	public static ON_SHOW_PAGE = "showPage";
	public static ON_PAGES_READY = "pagesReady";

	private readonly socket = io();
	private connectionStatus: ConnectionStatus = ConnectionStatus.DISCONNECTED;

	constructor() {
		super();
		this.registerEvents();
	}

	// basic

	public getConnectionStatus = (): ConnectionStatus => {
		return this.connectionStatus;
	};

	// registering

	public sendRegisterMessage(uuid: string, pageChainId?: number): void {
		this.connectionStatus = ConnectionStatus.REGISTERING;
		const device: DeviceModel = {
			uuid: uuid,
			screenInfo: getScreenInfo(),
			isBrowser: true,
			pageChainId: pageChainId || -1,
		};
		this.socket.emit(SocketController.ON_REGISTER, device);

		this.notifyAll(SocketController.ON_REGISTER);
	}

	// queuing

	public sendEnqueueMessage(): void {
		this.connectionStatus = ConnectionStatus.QUEUEING;
		this.socket.emit("enqueue");
	}

	public sendDequeueMessage(): void {
		this.connectionStatus = this.socket.connected
			? ConnectionStatus.CONNECTED
			: ConnectionStatus.DISCONNECTED;
		this.socket.emit("dequeue");
	}

	// pairing

	public sendPairMessage(doPrepend: boolean): void {
		this.socket.emit("pair", doPrepend);
	}

	public sendUnpairMessage(): void {
		this.socket.emit("unpair");
	}

	// private

	private registerEvents = (): void => {
		this.socket.on(SocketController.ON_CONNECT, async () => {
			const uuid: string = (await DataManager.getUUID()) as string;
			const pageChainId = (await DataManager.getPageChainInfo())?.id;

			this.sendRegisterMessage(uuid, pageChainId);
		});

		this.socket.on(SocketController.ON_REGISTERED, this.onRegistered);

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

	private onRegistered = (wasSuccessful: boolean): void => {
		if (wasSuccessful) {
			this.connectionStatus = ConnectionStatus.CONNECTED;
			this.notifyAll(SocketController.ON_REGISTERED);
		} else {
			this.connectionStatus = ConnectionStatus.DISCONNECTED;
			this.notifyAll(SocketController.ON_DISCONNECT);
		}
	};

	// pages ready
	private onPagesReady = (
		pageChainInfoModel: PageChainInfoModel | undefined
	): void => {
		this.notifyAll(SocketController.ON_PAGES_READY, pageChainInfoModel);
	};

	// page show
	private onShowPage = (pageIndex: number): void => {
		this.notifyAll(SocketController.ON_SHOW_PAGE, pageIndex);
	};

	// device index
	private onUpdateDeviceIndex = (deviceIndex: number): void => {
		this.notifyAll(SocketController.ON_UPDATE_DEVICE_INDEX, deviceIndex);
	};
}
