class DevicePair {
	#deviceIds = []; // uuids

	constructor(deviceIds) {
		this.#deviceIds = deviceIds || [];
	}

	getDeviceIds() {
		return this.#devices;
	}

	prepend(deviceId) {
		this.#deviceIds.unshift(deviceId);
		this.#updateDeviceParingIndices();
	}

	append(deviceId) {
		this.#deviceIds.push(deviceId);
		this.#updateDeviceParingIndices();
	}

	remove(deviceId) {
		this.#deviceIds = this.#deviceIds.filter((id) => id !== deviceId);
		this.#updateDeviceParingIndices();
	}

	#updateDeviceParingIndices = () => {
		for (const deviceId of this.#deviceIds) {
			const device = DeviceManager.getDeviceByUuid(deviceId);
			if (device) {
				const index = this.#deviceIds.indexOf(deviceId);
				this.#updateDevicePairingIndex(device, index);
			}
		}
	};

	#updateDevicePairingIndex = (device, index) => {
		if (device.socket) {
			console.log("Updating device pairing index", device.uuid, index);
			device.socket.emit("deviceIndex", index);
		}
	};
}

class PairingManager {}

module.exports = PairingManager;
