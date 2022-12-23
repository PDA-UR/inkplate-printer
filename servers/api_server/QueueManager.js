const fs = require("fs");
const os = require("os");
const chokidar = require("chokidar");
const DeviceManager = require("./DeviceManager");

const util = require("util");
const exec = util.promisify(require("child_process").exec);

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
		const pageChainId = new Date().getTime();

		for (const device of QueueManager.#waitingDevices.values()) {
			ps2bmpJobs.push(
				this.#ps2bpm(filepath, device.device)
					.then((pageCount) => {
						console.log("Sending pages ready message");
						device.socket.emit("pagesReady", {
							pageCount,
							id: pageChainId,
						});
						DeviceManager.updatePageChainId(device.socket.id, pageChainId);
					})
					.catch((error) => {
						console.log("Error converting ps to bmp", error, device);
						device.socket.emit("pagesReady", { pageChainId: -1 }); // send event without num pages
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
		socket.emit("updateDeviceIndex", { deviceIndex });
	};

	static #ps2bpm(filepath, device) {
		console.log("Converting ps to jpeg", filepath, device);
		// TODO: use resolution
		const serverHomePath = os.homedir() + "/.inkplate-printer",
			imgPath = serverHomePath + "/img/",
			outDir = imgPath + device.uuid + "/";

		// delete out dir if it already exists (e.g. previous print job)
		if (fs.existsSync(outDir)) {
			fs.rmSync(outDir, { recursive: true }, (error) => {
				if (error) console.log("Error removing out dir", error);
			});
		}

		// create out dir
		fs.mkdirSync(outDir, { recursive: true });

		return new Promise((resolve, reject) => {
			QueueManager.#spawnConvertProcess(
				filepath,
				device,
				outDir,
				(error, stdout, stderr) => {
					if (error) {
						console.log(`error: ${error.message}`);
						reject(error);
						return;
					}
					if (stderr) {
						console.log(`stderr: ${stderr}`);
						reject(stderr);
						return;
					}
					// if complete code
					if (stdout) {
						console.log(`stdout: ${stdout}`);
						if (stdout.includes("done")) {
							const files = fs.readdirSync(outDir);
							console.log("done", files.length);
							resolve(files.length);
						}
					}
				}
			);
		});
	}

	static #spawnConvertProcess = (filepath, device, outDir, cb) => {
		let { width, height } = device.screenInfo.resolution;
		// if browser
		if (device.isBrowser) {
			width = width * 2;
			height = height * 2;
		}
		const o = outDir.substring(0, outDir.length - 1);
		const j = `./scripts/convert.sh ${width} ${height} "${filepath}" "${o}"`;
		console.log("Executing", j);
		const process = exec(j, cb);
		return process;
	};

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

	static pxToPoint = (px, dpi) => {
		const pt = px; // TODO: Caclulate pointsize
		return pt;
	};
}

module.exports = QueueManager;
