import SocketController from "./controllers/SocketController";
import ViewController from "./controllers/ViewController";
import DataManager from "./data/DataManager";
import DevicePairingUpdateModel from "./data/DevicePairingUpdateModel";
import PageChainInfoModel from "./data/PageChainInfoModel";
import { AppEvent } from "./lib/AppEvent";
import State, { DisplayMode } from "./lib/AppState";
import ConnectionStatus from "./lib/ConnectionStatus";

DataManager.load().then(onApplicationStart);

function onApplicationStart() {
	const socketConnection = new SocketController();
	const viewController = new ViewController();

	initState(viewController, socketConnection);

	// ~~~~~~~~~~~~ Socket events ~~~~~~~~~~~~ //

	socketConnection.on(SocketController.ON_REGISTER, () => {
		console.log("registering");
		updateConnectionStatus();
	});

	socketConnection.on(SocketController.ON_REGISTERED, async () => {
		console.log("connected");
		updateConnectionStatus();
	});

	socketConnection.on(SocketController.ON_DISCONNECT, () => {
		console.log("disconnected");
		if (socketConnection.getConnectionStatus() === ConnectionStatus.QUEUEING) {
			State.toggleDisplayMode(false);
			viewController.setDeviceIndex(-1);
			updateConnectionStatus();
		}
	});

	socketConnection.on(
		SocketController.ON_SHOW_PAGE,
		async (event: AppEvent<number>) => {
			const pageIndex = event.data;
			if (pageIndex !== undefined) {
				try {
					const pageCount = (await DataManager.getPageChainInfo())?.pageCount;
					if (
						pageCount !== undefined &&
						pageIndex > 0 &&
						pageIndex <= pageCount
					) {
						await DataManager.saveCurrentPageIndex(pageIndex);
						const pageModel = await DataManager.getPage(pageIndex);

						if (pageModel !== undefined) viewController.setPage(pageModel);
						else console.error("pageModel is undefined");
					} else {
						console.error(
							"pageCount is undefined or pageIndex is out of range"
						);
					}
				} catch (e) {
					console.error(e);
				}
			} else {
				console.error("invalid page index");
			}
		}
	);

	socketConnection.on(
		SocketController.ON_UPDATE_DEVICE_INDEX,
		async (event: AppEvent<number>) => {
			const deviceIndex = event.data;
			if (deviceIndex !== undefined) {
				try {
					await DataManager.saveDeviceIndex(deviceIndex);
					viewController.setDeviceIndex(deviceIndex);
				} catch (e) {
					console.error(e);
				}
			} else {
				console.error("invalid device index");
			}
		}
	);

	socketConnection.on(
		SocketController.ON_PAGES_READY,
		async (event: AppEvent<PageChainInfoModel | undefined>) => {
			console.log("pages ready main.ts", event);
			const pageChainInfoModel = event.data;
			console.log("pageChainInfoModel1", pageChainInfoModel);
			if (pageChainInfoModel !== undefined && pageChainInfoModel.id !== -1) {
				try {
					const deviceIndex = await DataManager.getDeviceIndex();
					if (deviceIndex !== undefined) {
						await DataManager.resetPageChainData();

						console.log("device index", deviceIndex);
						await DataManager.saveCurrentPageIndex(deviceIndex);
						await DataManager.savePageChainInfo(pageChainInfoModel);
						const pageModel = await DataManager.getCurrentPage();

						console.log("pageChainInfoModel2", pageChainInfoModel);
						viewController.setPageCount(pageChainInfoModel.pageCount);

						if (pageModel !== undefined) viewController.setPage(pageModel);
						else throw new Error("pageModel is undefined");

						State.mode = DisplayMode.DISPLAYING;

						viewController.setDeviceIndex(-1);

						await DataManager.getAllExceptCurrentPage();

						viewController.setConnectionStatus(ConnectionStatus.CONNECTED);
						console.log("all pages loaded");
					} else {
						throw new Error("device index is undefined");
					}
				} catch (e) {
					console.error(e);
				}
			} else {
				console.error("invalid num pages");
			}
		}
	);

	socketConnection.on(
		SocketController.ON_UPDATE_PAIRING_INDEX,
		(event: AppEvent<DevicePairingUpdateModel>) =>
			viewController.setPairingIndex(event.data as DevicePairingUpdateModel)
	);

	// ~~~~~~~~~~~~~ View events ~~~~~~~~~~~~~ //

	viewController.on(ViewController.ON_ENQUEUE, async () => {
		console.log("enqueue");
		const uuid = await DataManager.getUUID();
		if (uuid !== undefined) {
			if (
				socketConnection.getConnectionStatus() !== ConnectionStatus.QUEUEING
			) {
				socketConnection.sendEnqueueMessage();
			} else {
				socketConnection.sendDequeueMessage();
			}
			viewController.setConnectionStatus(
				socketConnection.getConnectionStatus()
			);
		} else {
			console.error("uuid is undefined");
		}
	});

	viewController.on(ViewController.ON_NEXT_PAGE, () => onNavigatePage(true));

	viewController.on(ViewController.ON_PREVIOUS_PAGE, () =>
		onNavigatePage(false)
	);

	viewController.on(ViewController.ON_PAIR_LEFT, () => onDoPair(false));
	viewController.on(ViewController.ON_PAIR_RIGHT, () => onDoPair(true));
	viewController.on(ViewController.ON_UNPAIR, () => onDoUnpair());

	const onNavigatePage = async (
		doGoNext: boolean,
		triggeredByPairing = false
	): Promise<void> => {
		if (socketConnection.getConnectionStatus() !== ConnectionStatus.QUEUEING) {
			if (State.mode === DisplayMode.DISPLAYING) {
				try {
					const newPageIndex = await DataManager.stepCurrentPageIndex(doGoNext);
					if (newPageIndex !== undefined) {
						console.log("newPageIndex", newPageIndex);
						const pageModel = await DataManager.getPage(newPageIndex);
						if (pageModel !== undefined) {
							viewController.setPage(pageModel);
							if (!triggeredByPairing)
								socketConnection.sendUpdatePageIndexMessage(newPageIndex);
						}
					} else {
						console.log("limit reached");
					}
				} catch (e) {
					console.error(e);
				}
			} else {
				console.log("not in display mode");
			}
		} else {
			// abort queueing
			socketConnection.sendDequeueMessage();
			if (State.mode === DisplayMode.DISPLAYING) {
				onNavigatePage(doGoNext);
			}
		}
	};

	const updateConnectionStatus = () => {
		viewController.setConnectionStatus(socketConnection.getConnectionStatus());
	};

	const onDoPair = async (doPrepend: boolean) => {
		socketConnection.sendPairMessage(doPrepend);
	};

	const onDoUnpair = async () => {
		socketConnection.sendUnpairMessage();
	};
}

async function initState(
	viewController: ViewController,
	socketConnection: SocketController
) {
	const hasPageChain = await DataManager.hasPageChain();
	if (hasPageChain) {
		State.mode = DisplayMode.DISPLAYING;
		const pageChain = await DataManager.getPageChainInfo();
		if (pageChain !== undefined && pageChain?.id !== -1) {
			console.log("pageChain", pageChain);
			viewController.setPageCount(pageChain.pageCount);
		}

		const page = await DataManager.getCurrentPage();
		if (page !== undefined) {
			viewController.setPage(page);
		}
	} else {
		State.mode = DisplayMode.BLANK;
		viewController.setBlank();
	}
	viewController.setConnectionStatus(socketConnection.getConnectionStatus());
}
