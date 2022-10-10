import SocketController from "./controllers/SocketController";
import ViewController from "./controllers/ViewController";
import DataManager from "./data/DataManager";
import { AppEvent } from "./lib/AppEvent";
import State, { AppMode } from "./lib/AppState";

DataManager.load().then(onApplicationStart);

function onApplicationStart() {
	const socketConnection = new SocketController();
	const viewController = new ViewController();

	document.body.style.backgroundColor = "red";

	// ~~~~~~~~~~~~ Socket events ~~~~~~~~~~~~ //

	socketConnection.on(SocketController.ON_CONNECT, () => {
		console.log("connected");
		document.body.style.backgroundColor = "green";
	});

	socketConnection.on(SocketController.ON_DISCONNECT, () => {
		console.log("disconnected");
		document.body.style.backgroundColor = "red";
	});

	socketConnection.on(
		SocketController.ON_SHOW_PAGE,
		(event: AppEvent<number>) => {
			const pageIndex = event.data;
			if (pageIndex !== undefined) {
				DataManager.saveCurrentPageIndex(pageIndex)
					.then(() => DataManager.getPage(pageIndex))
					.then(viewController.showPage)
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
					viewController.updateDeviceIndex(deviceIndex);
				});
			} else {
				console.error("invalid device index");
			}
		}
	);

	// ~~~~~~~~~~~~~ View events ~~~~~~~~~~~~~ //

	viewController.on(ViewController.ON_REGISTER_DEVICE_CLICKED, async () => {
		const mode = await State.toggleRegisterMode();
		const uuid = await DataManager.getUUID();

		if (mode === AppMode.REGISTERING) {
			socketConnection.sendRegisterRequest(uuid);
		} else {
			socketConnection.sendUnregisterRequest(uuid);
		}
	});

	viewController.on(ViewController.ON_NEXT_PAGE_CLICKED, () =>
		onNavigatePage(true)
	);

	viewController.on(ViewController.ON_PREVIOUS_PAGE_CLICKED, () =>
		onNavigatePage(false)
	);

	const onNavigatePage = (doGoNext: boolean): Promise<void> =>
		DataManager.stepCurrentPageIndex(doGoNext)
			.then(DataManager.getPage)
			.then(viewController.showPage)
			.catch((e) => console.error(e));
}
