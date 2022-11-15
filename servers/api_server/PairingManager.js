const DeviceManager = require("./DeviceManager");

class DevicePair {
	#deviceIds = []; // uuids

	constructor(deviceIds) {
		this.#deviceIds = deviceIds || [];
		this.#updateParingIndices();
	}

	getDeviceIds() {
		return this.#deviceIds;
	}

	prepend(deviceId) {
		this.#deviceIds.unshift(deviceId);
		this.#updateParingIndices();
	}

	append(deviceId) {
		this.#deviceIds.push(deviceId);
		const device = DeviceManager.getDeviceByUuid(deviceId);
		if (device) this.#updatePairingIndex(device, this.#deviceIds.length - 1);
	}

	remove(deviceId) {
		this.#deviceIds = this.#deviceIds.filter((id) => id !== deviceId);
		const pairingIsEmpty = this.#deviceIds.length === 0;
		if (!pairingIsEmpty) {
			this.#updateParingIndices();
		}
		const removedDevice = DeviceManager.getDeviceByUuid(deviceId);
		if (removedDevice) {
			removedDevice.socket.emit("updatePairingIndex", -1);
		}
		return !pairingIsEmpty;
	}

	index(deviceId) {
		return this.#deviceIds.indexOf(deviceId);
	}

	has = (deviceId) => {
		return this.#deviceIds.includes(deviceId);
	};

	onUpdatePageIndex(deviceId, pageIndex) {
		const deviceIndex = this.#deviceIds.indexOf(deviceId);
		console.log(
			"update page index",
			deviceIndex,
			pageIndex,
			deviceId,
			this.#deviceIds
		);
		if (deviceIndex !== -1) {
			this.#deviceIds.forEach((id, index) => {
				if (index !== deviceIndex) {
					const newPageIndex = pageIndex + (index - deviceIndex);
					const device = DeviceManager.getDeviceByUuid(id);
					if (device) {
						console.log("updateing page index", newPageIndex);
						device.socket.emit("showPage", newPageIndex);
					}
				}
			});
		}
	}

	#updateParingIndices = () => {
		for (const deviceId of this.#deviceIds) {
			const device = DeviceManager.getDeviceByUuid(deviceId);
			if (device) {
				const index = this.#deviceIds.indexOf(deviceId);
				this.#updatePairingIndex(device, index);
			}
		}
	};

	#updatePairingIndex = (device, index) => {
		if (device.socket) {
			const numDevices = this.#deviceIds.length;
			device.socket.emit("updatePairingIndex", index, numDevices);
		}
	};
}

class PairingManager {
	static #pairings = new Map(); // doc id -> DevicePair

	static pair(pageChainId, deviceId, doPrepend) {
		let pairing = this.#pairings.get(pageChainId);
		if (pairing) {
			if (!pairing.has(deviceId)) {
				if (doPrepend) {
					pairing.prepend(deviceId);
				} else {
					pairing.append(deviceId);
				}
			} else {
				console.log("Device already paired");
			}
		} else {
			pairing = new DevicePair([deviceId]);
			this.#pairings.set(pageChainId, pairing);
		}
	}

	static unpair(pageChainId, deviceId) {
		const pairing = this.#pairings.get(pageChainId);
		if (pairing) {
			console.log("unpairing", deviceId);
			const hasPairingsLeft = pairing.remove(deviceId);
			if (!hasPairingsLeft) {
				this.#pairings.delete(pageChainId);
			}
		}
	}

	static appendDevice(pairing, deviceId) {
		if (pairing) {
			pairing.append(deviceId);
		} else {
			this.#pairings.set(pageChainId, new DevicePair([deviceId]));
		}
	}

	static onUpdatePageIndex(pageChainId, deviceId, pageIndex) {
		const pairing = this.#pairings.get(pageChainId);
		if (pairing) {
			pairing.onUpdatePageIndex(deviceId, pageIndex);
		}
	}

	static hasPairing(pageChainId) {
		return this.#pairings.has(pageChainId);
	}

	static getPairings() {
		return this.#pairings;
	}
}

module.exports = PairingManager;
