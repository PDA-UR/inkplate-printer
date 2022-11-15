class DeviceManager {
	// sockedId: string, device: Device
	static devices = new Map();

	static register = (socketId, device) => {
		console.log("Registering device", device.uuid);
		this.devices.set(socketId, device);
	};

	static unregister = (socketId) => {
		const device = this.devices.get(socketId);
		this.devices.delete(socketId);
		return device;
	};

	static getDeviceBySocketId = (socketId) => {
		return this.devices.get(socketId);
	};

	static getDeviceByUuid = (uuid) => {
		for (const device of this.devices.values()) {
			if (device.uuid === uuid) return device;
		}
		return null;
	};

	static getAllDevices = () => {
		// as plain js object
		const devices = {};
		for (const [key, value] of this.devices) {
			devices[key] = value;
		}
		return devices;
	};

	static updatePageChainId = (socketId, pageChainId) => {
		const device = this.devices.get(socketId);
		if (device) {
			device.pageChainId = pageChainId;
		}
	};

	static getSocketByDeviceId = (deviceId) => {
		const device = this.getDeviceByUuid(deviceId);
		if (device !== undefined) {
			return device.socket;
		}
		return undefined;
	};
}

module.exports = DeviceManager;
