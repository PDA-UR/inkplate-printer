const fs = require("fs");
const os = require("os");
const chokidar = require("chokidar");

class QueueManager {
	static QUEUE_FOLDER_PATH = require("path").join(
		require("os").homedir(),
		".inkplate-printer",
		"print_queue"
	);

	static #watcher = null;
	static #waitingDevices = new Map();

	static init() {
		// delete existing queue
		this.#clearQueue();
	}

	static enqueue = (socket, device) => {
		console.log("Enqueueing device", device);
		if (device?.uuid) {
			QueueManager.#waitingDevices.set(device.uuid, {
				socket,
				device,
				index: QueueManager.#waitingDevices.size + 1,
			});
			this.#updateDeviceIndex(socket, QueueManager.#waitingDevices.size);
			if (!this.isOpen()) this.#openQueue();
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

	static isOpen = () => {
		return fs.existsSync(QueueManager.QUEUE_FOLDER_PATH);
	};

	static #closeQueue = (filepath) => {
		console.log("Closing print queue");
		this.#watcher?.close();
		this.#watcher = null;

		const ps2bmpJobs = [];

		for (const device of QueueManager.#waitingDevices.values()) {
			ps2bmpJobs.push(
				this.#ps2bpm(filepath, device.device)
					.then((numPages) => {
						console.log("Sending pages ready message", numPages);
						device.socket.emit("pagesReady", numPages);
					})
					.catch((error) => {
						console.log("Error converting ps to bmp", error, device);
						device.socket.emit("pagesReady"); // send event without num pages
					})
			);
		}
		Promise.all(ps2bmpJobs).then(() => {
			console.log("All ps2bmp jobs done, clearing queue");
			QueueManager.#clearQueue();
		});
	};

	static #clearQueue = () => {
		this.#waitingDevices.clear();
		if (fs.existsSync(QueueManager.QUEUE_FOLDER_PATH)) {
			fs.rm(this.QUEUE_FOLDER_PATH, { recursive: true }, (error) => {
				if (error) console.log("Error removing queue folder", error);
			});
		}
	};

	static #updateDeviceIndex = (socket, deviceIndex) => {
		socket.emit("updateDeviceIndex", deviceIndex);
	};

	static #ps2bpm(filepath, device) {
		console.log("Converting ps to bmp", filepath, device);
		// TODO: use resolution
		const serverHomePath = os.homedir() + "/.inkplate-printer",
			imgPath = serverHomePath + "/img/",
			outDir = imgPath + device.uuid + "/";

		// create out dir if it doesn't exist
		if (!fs.existsSync(outDir)) {
			fs.mkdirSync(outDir, { recursive: true });
		}

		return new Promise((resolve, reject) => {
			const spawn = require("child_process").spawn;
			const process = spawn("gs", [
				"-sDEVICE=bmpgray",
				"-dNOPAUSE",
				"-dBATCH",
				"-dSAFER",
				"-r145",
				"-g825x1200",
				"-dPDFFitPage",
				"-sOutputFile=" + outDir + "%d.bmp",
				filepath,
			]);

			process.stdout.on("data", (data) => {
				console.log("gs: " + data);
			});

			process.on("exit", () => {
				const numFiles = fs.readdirSync(outDir).length;
				console.log("gs: " + numFiles + " files created");
				resolve(numFiles);
			});
			process.on("error", (error) => reject(error));
		});
	}

	static #onFileAdded = (filePath) => {
		console.log("queue changed", filePath);
		if (filePath.endsWith(".ps")) {
			console.log("New PS file added to print queue");
			this.#closeQueue(filePath);
		}
	};

	static getDevices = () => {
		// returns devices as array sorted by property index
		const devices = [];
		// for each key
		for (const key of QueueManager.#waitingDevices.keys()) {
			const device = QueueManager.#waitingDevices.get(key);
			devices.splice(device.index - 1, 0, device);
		}
		return devices;
	};
}

module.exports = QueueManager;
