import SocketController from "./controllers/SocketController";
import ViewController from "./controllers/ViewController";
import DataManager from "./data/DataManager";
import PageChainInfoModel from "./data/PageChainInfoModel";
import PageModel from "./data/PageModel";
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
		viewController.setConnectionStatus(socketConnection.getConnectionStatus());
	});

	socketConnection.on(SocketController.ON_REGISTERED, async () => {
		console.log("connected");
		viewController.setConnectionStatus(socketConnection.getConnectionStatus());
	});

	socketConnection.on(SocketController.ON_DISCONNECT, () => {
		console.log("disconnected");
		document.body.style.backgroundColor = "red";
		if (socketConnection.getConnectionStatus() === ConnectionStatus.QUEUEING) {
			State.toggleDisplayMode(false);
			viewController.setRegistering(-1);
		}
	});

	socketConnection.on(
		SocketController.ON_SHOW_PAGE,
		async (event: AppEvent<number>) => {
			const pageIndex = event.data;
			if (pageIndex !== undefined) {
				try {
					await DataManager.saveCurrentPageIndex(pageIndex);
					const pageModel = await DataManager.getPage(pageIndex);

					if (pageModel !== undefined) viewController.setPage(pageModel);
					else console.error("pageModel is undefined");
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
					viewController.setRegistering(deviceIndex);
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
		async (event: AppEvent<number>) => {
			console.log("pages ready main.ts", event);
			const pageCount = event.data;
			if (pageCount !== undefined) {
				try {
					const deviceIndex = await DataManager.getDeviceIndex();
					if (deviceIndex !== undefined) {
						await DataManager.resetPageChainData();

						console.log("device index", deviceIndex);
						await DataManager.saveCurrentPageIndex(deviceIndex);
						await DataManager.savePageChainInfo({
							pageCount,
						});
						const pageModel = await DataManager.getCurrentPage();

						if (pageModel !== undefined) viewController.setPage(pageModel);
						else throw new Error("pageModel is undefined");

						await DataManager.getAllExceptCurrentPage();
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

	// ~~~~~~~~~~~~~ View events ~~~~~~~~~~~~~ //

	viewController.on(ViewController.ON_ENQUEUE, async () => {
		const uuid = await DataManager.getUUID();
		if (uuid !== undefined) {
			if (
				socketConnection.getConnectionStatus() !== ConnectionStatus.QUEUEING
			) {
				socketConnection.sendEnqueueMessage();
			} else {
				socketConnection.sendDequeueMessage();
			}
		} else {
			console.error("uuid is undefined");
		}
	});

	viewController.on(ViewController.ON_NEXT_PAGE, () => onNavigatePage(true));

	viewController.on(ViewController.ON_PREVIOUS_PAGE, () =>
		onNavigatePage(false)
	);

	const onNavigatePage = async (doGoNext: boolean): Promise<void> => {
		if (
			socketConnection.getConnectionStatus() !== ConnectionStatus.QUEUEING &&
			State.mode === DisplayMode.DISPLAYING
		) {
			try {
				const newPageIndex = await DataManager.stepCurrentPageIndex(doGoNext);
				if (newPageIndex !== undefined) {
					const pageModel = await DataManager.getPage(newPageIndex);
					if (pageModel) viewController.setPage(pageModel);
				} else {
					console.log("limit reached");
				}
			} catch (e) {
				console.error(e);
			}
		} else {
			// abort queueing
			socketConnection.sendDequeueMessage();
			if (State.mode === DisplayMode.DISPLAYING) {
				onNavigatePage(doGoNext);
			}
		}
	};
}

async function initState(
	viewController: ViewController,
	socketConnection: SocketController
) {
	const hasPageChain = await DataManager.hasPageChain();
	if (hasPageChain) {
		State.mode = DisplayMode.DISPLAYING;
		const page = await DataManager.getCurrentPage();
		if (page) {
			viewController.setPage(page);
		}
	} else {
		State.mode = DisplayMode.BLANK;
		viewController.setBlank();
	}
	viewController.setConnectionStatus(socketConnection.getConnectionStatus());
}
