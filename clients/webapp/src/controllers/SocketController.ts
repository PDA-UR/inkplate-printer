import { io } from "socket.io-client";
import DataManager from "../data/DataManager";
import { Observable } from "../lib/Observable";
import ConnectionStatus from "../lib/ConnectionStatus";
import DeviceModel, { getScreenInfo } from "../data/DeviceModel";
import PageChainInfoModel from "../data/PageChainInfoModel";

export default class SocketController extends Observable {
	public static DISCONNECT_MESSAGE = "disconnect";
	public static CONNECT_MESSAGE = "connect";

	public static REGISTER_MESSAGE = "register";
	public static REGISTERED_MESSAGE = "registered";

	public static ENQUEUE_MESSAGE = "enqueue";
	public static DEQUEUE_MESSAGE = "dequeue";

	public static UPDATE_DEVICE_INDEX_MESSAGE = "updateDeviceIndex";
	public static UPDATE_PAGE_INDEX = "updatePageIndex";
	public static SHOW_PAGE_MESSAGE = "showPage";
	public static PAGES_READY_MESSAGE = "pagesReady";

	public static PAIR_MESSAGE = "pair";
	public static UNPAIR_MESSAGE = "unpair";
	public static UPDATE_PAIRING_INDEX_MESSAGE = "updatePairingIndex";

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
		this.socket.emit(SocketController.REGISTER_MESSAGE, device);

		this.notifyAll(SocketController.REGISTER_MESSAGE);
	}

	// queuing

	public sendEnqueueMessage(): void {
		this.connectionStatus = ConnectionStatus.QUEUEING;
		this.socket.emit(SocketController.ENQUEUE_MESSAGE);
	}

	public sendDequeueMessage(): void {
		this.connectionStatus = this.socket.connected
			? ConnectionStatus.CONNECTED
			: ConnectionStatus.DISCONNECTED;
		this.socket.emit(SocketController.DEQUEUE_MESSAGE);
	}

	// pairing

	public sendPairMessage(doPrepend: boolean): void {
		this.socket.emit(SocketController.PAIR_MESSAGE, doPrepend);
	}

	public sendUnpairMessage(): void {
		this.socket.emit(SocketController.UNPAIR_MESSAGE);
	}

	// paging, pairing

	public sendUpdatePageIndexMessage(pageIndex: number): void {
		this.socket.emit(SocketController.UPDATE_PAGE_INDEX, pageIndex);
	}

	// private

	private registerEvents = (): void => {
		this.socket.on(SocketController.CONNECT_MESSAGE, async () => {
			const uuid: string = (await DataManager.getUUID()) as string;
			const pageChainId = (await DataManager.getPageChainInfo())?.id;

			this.sendRegisterMessage(uuid, pageChainId);
		});

		this.socket.on(SocketController.REGISTERED_MESSAGE, this.onRegistered);

		this.socket.on(SocketController.DISCONNECT_MESSAGE, () => {
			this.notifyAll(SocketController.DISCONNECT_MESSAGE);
		});
		this.socket.on(SocketController.PAGES_READY_MESSAGE, this.onPagesReady);
		this.socket.on(SocketController.SHOW_PAGE_MESSAGE, this.onShowPage);
		this.socket.on(
			SocketController.UPDATE_DEVICE_INDEX_MESSAGE,
			this.onUpdateDeviceIndex
		);
		this.socket.on(
			SocketController.UPDATE_PAIRING_INDEX_MESSAGE,
			this.onUpdatePairingIndex
		);
	};

	private onRegistered = ({
		wasSuccessful,
	}: {
		wasSuccessful: boolean;
	}): void => {
		if (wasSuccessful) {
			console.log("Registered successfully");
			this.connectionStatus = ConnectionStatus.CONNECTED;
			this.notifyAll(SocketController.REGISTERED_MESSAGE);
		} else {
			this.connectionStatus = ConnectionStatus.DISCONNECTED;
			this.notifyAll(SocketController.DISCONNECT_MESSAGE);
		}
	};

	// pages ready
	private onPagesReady = (
		pageChainInfoModel: PageChainInfoModel | undefined
	): void => {
		this.notifyAll(SocketController.PAGES_READY_MESSAGE, pageChainInfoModel);
	};

	// page show
	private onShowPage = ({ pageIndex }: { pageIndex: number }): void => {
		this.notifyAll(SocketController.SHOW_PAGE_MESSAGE, pageIndex);
	};

	// device index
	private onUpdateDeviceIndex = ({
		deviceIndex,
	}: {
		deviceIndex: number;
	}): void => {
		console.log("update device index", deviceIndex);
		this.notifyAll(SocketController.UPDATE_DEVICE_INDEX_MESSAGE, deviceIndex);
	};

	// pairing index update
	private onUpdatePairingIndex = (
		deviceIndex: number,
		numDevices?: number
	): void => {
		console.log("onUpdatePairingIndex", deviceIndex, numDevices);
		this.notifyAll(SocketController.UPDATE_PAIRING_INDEX_MESSAGE, {
			deviceIndex,
			numDevices: numDevices || 0,
		});
	};
}
