const fs = require("fs");
const chokidar = require("chokidar");

class QueueManager {
	static QUEUE_FOLDER_PATH = require("path").join(
		require("os").homedir(),
		".inkplate-printer",
		"print_queue"
	);

	static #watcher = null;
	static #waitingDevices = new Map();

	static enqueue = (socket, deviceData) => {
		console.log("Enqueueing device", deviceData);
		if (deviceData?.uuid) {
			QueueManager.#waitingDevices.set(deviceData.uuid, {
				socket,
				deviceData,
			});
			this.#updateDeviceIndex(socket, QueueManager.#waitingDevices.size);
			if (!this.queueIsOpen()) this.#openQueue();
		} else {
			console.log("No device uuid provided");
		}
	};

	static dequeue = (uuid) => {
		console.log("Dequeueing device", uuid);
		const deviceToDequeue = QueueManager.#waitingDevices.get(uuid);
		if (deviceToDequeue !== undefined) {
			this.#updateDeviceIndex(deviceToDequeue.socket, -1);
			this.#waitingDevices.delete(uuid);
			let index = 1;
			for (const device of QueueManager.#waitingDevices.values()) {
				this.#updateDeviceIndex(device.socket, index);
				index++;
			}
			if (QueueManager.#waitingDevices.size === 0) {
				this.#closeQueue();
			}
		}
	};

	static #openQueue = () => {
		console.log("Opening print queue");

		fs.mkdirSync(QueueManager.QUEUE_FOLDER_PATH);
		if (this.#watcher === null) {
			this.#watcher = chokidar
				.watch(QueueManager.QUEUE_FOLDER_PATH)
				.on("add", QueueManager.#onFileAdded);
		}
	};

	static queueIsOpen = () => {
		return fs.existsSync(QueueManager.QUEUE_FOLDER_PATH);
	};

	static #closeQueue = () => {
		console.log("Closing print queue");
		this.#watcher?.close();
		this.#watcher = null;

		// TODO: Move ps to save location
		this.#processPsFile().then(() =>
			fs.rm(this.QUEUE_FOLDER_PATH, { recursive: true }, (error) => {})
		);
	};

	static #updateDeviceIndex = (socket, deviceIndex) => {
		socket.emit("updateDeviceIndex", deviceIndex);
	};

	static #processPsFile = () => {
		return new Promise((resolve, reject) => {});
	};

	static #onFileAdded = (filePath) => {
		console.log("queue changed", filePath);
		if (filePath.endsWith(".ps")) {
			console.log("New PS file added to print queue");
			this.#closeQueue();
		}
	};
}

module.exports = QueueManager;
