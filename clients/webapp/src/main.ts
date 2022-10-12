import SocketController from "./controllers/SocketController";
import ViewController from "./controllers/ViewController";
import DataManager from "./data/DataManager";
import PageChainInfoModel from "./data/PageChainInfoModel";
import PageModel from "./data/PageModel";
import { AppEvent } from "./lib/AppEvent";
import State, { AppMode } from "./lib/AppState";

DataManager.load().then(onApplicationStart);

function onApplicationStart() {
	const socketConnection = new SocketController();
	const viewController = new ViewController();

	document.body.style.backgroundColor = "orange";

	// ~~~~~~~~~~~~ Socket events ~~~~~~~~~~~~ //

	socketConnection.on(SocketController.ON_REGISTERED, async () => {
		console.log("connected");
		document.body.style.backgroundColor = "green";
		const hasPageChain = await DataManager.hasPageChain();
		if (hasPageChain) {
			State.mode = AppMode.DISPLAYING;
			const page = await DataManager.getCurrentPage();
			if (page) {
				viewController.setPage(page);
			}
		} else {
			State.mode = AppMode.BLANK;
			viewController.setBlank();
		}
	});

	socketConnection.on(SocketController.ON_DISCONNECT, () => {
		console.log("disconnected");
		document.body.style.backgroundColor = "red";
		// reset queue stuff
		if (State.mode === AppMode.REGISTERING) {
			State.toggleRegisterMode();
			viewController.setRegistering(-1);
		}
	});

	socketConnection.on(
		SocketController.ON_SHOW_PAGE,
		(event: AppEvent<number>) => {
			const pageIndex = event.data;
			if (pageIndex !== undefined) {
				DataManager.saveCurrentPageIndex(pageIndex)
					.then(() => DataManager.getPage(pageIndex))
					.then((pageModel) => {
						if (pageModel !== undefined) {
							viewController.setPage(pageModel);
						} else {
							console.error("pageModel is undefined");
						}
					})
					.catch((error) => console.error(error));
			} else {
				console.error("invalid page index");
			}
		}
	);

	socketConnection.on(
		SocketController.ON_UPDATE_DEVICE_INDEX,
		(event: AppEvent<number>) => {
			const deviceIndex = event.data;
			if (deviceIndex !== undefined) {
				DataManager.saveDeviceIndex(deviceIndex).then(() => {
					viewController.setRegistering(deviceIndex);
				});
			} else {
				console.error("invalid device index");
			}
		}
	);

	socketConnection.on(
		SocketController.ON_PAGES_READY,
		(event: AppEvent<number>) => {
			console.log("pages ready main.ts", event);
			const numPages = event.data;
			if (numPages !== undefined) {
				DataManager.getDeviceIndex()
					.then((deviceIndex) =>
						DataManager.saveCurrentPageIndex(deviceIndex ?? 1)
					)
					.then(() => {
						const pageChainInfo: PageChainInfoModel = {
							pageCount: numPages,
						};
						return DataManager.savePageChainInfo(pageChainInfo);
					})
					.then(() => DataManager.getCurrentPage())
					.then((pageModel: PageModel | undefined) => {
						if (pageModel !== undefined) {
							viewController.setPage(pageModel);
						} else {
							console.error("no page model");
						}
					})
					.then(DataManager.getAllExceptCurrentPage) // load the rest
					.then(() => {
						console.log("all pages loaded");
					})
					.catch((error) => console.error(error));
			} else {
				console.error("invalid num pages");
			}
		}
	);

	// ~~~~~~~~~~~~~ View events ~~~~~~~~~~~~~ //

	viewController.on(ViewController.ON_REGISTER_DEVICE_CLICKED, async () => {
		const mode = await State.toggleRegisterMode();
		const uuid = await DataManager.getUUID();
		if (uuid !== undefined) {
			if (mode === AppMode.REGISTERING) {
				socketConnection.sendEnqueueMessage(uuid);
			} else {
				socketConnection.sendDequeueMessage(uuid);
			}
		} else {
			console.error("uuid is undefined");
		}
	});

	viewController.on(ViewController.ON_NEXT_PAGE_CLICKED, () =>
		onNavigatePage(true)
	);

	viewController.on(ViewController.ON_PREVIOUS_PAGE_CLICKED, () =>
		onNavigatePage(false)
	);

	const onNavigatePage = async (doGoNext: boolean): Promise<void> => {
		if (State.mode === AppMode.DISPLAYING) {
			const newPageIndex = await DataManager.stepCurrentPageIndex(doGoNext);
			if (newPageIndex !== undefined) {
				return DataManager.getPage(newPageIndex)
					.then((pageModel) => {
						if (pageModel) viewController.setPage;
					})
					.catch((e) => console.error(e));
			} else {
				// the page does not exist, meaning that the
				// start or end of the page chain has been reached
				console.error("newPageIndex is undefined");
			}
		} else if (State.mode === AppMode.REGISTERING) {
			// abort register mode, go into display mode
			console.log("abort register mode");
			const newMode = await State.toggleRegisterMode();
			const uuid = await DataManager.getUUID();
			if (uuid !== undefined) {
				socketConnection.sendDequeueMessage(uuid);
			} else {
				console.error("uuid is undefined");
			}
			if (newMode == AppMode.DISPLAYING) {
				onNavigatePage(doGoNext);
			}
		}
	};
}
