import SocketController from "./controllers/SocketController";
import ViewController from "./controllers/ViewController";
import DataManager from "./data/DataManager";

DataManager.load().then(onApplicationStart);

function onApplicationStart() {
	const socketConnection = new SocketController();
	const viewController = new ViewController();
	let isRegistering = false;
	document.body.style.backgroundColor = "red";

	socketConnection.on(SocketController.ON_CONNECT, () => {
		console.log("connected");
		document.body.style.backgroundColor = "green";
	});

	socketConnection.on(SocketController.ON_DISCONNECT, () => {
		console.log("disconnected");
		document.body.style.backgroundColor = "red";
	});

	viewController.on(ViewController.ON_REQUEST_PAGE_CHAIN_CLICKED, () => {
		isRegistering = !isRegistering;
		if (isRegistering) {
			socketConnection.sendRegisterRequest();
		} else {
			socketConnection.sendUnregisterRequest();
		}
	});
}
