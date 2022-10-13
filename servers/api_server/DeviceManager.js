class DeviceManager {
	// sockedId: string, device: Device
	static devices = new Map();

	static register = (socketId, device) => {
		console.log("Registering device", device);
		this.devices.set(socketId, device);
	};

	static unregister = (socketId) => {
		const device = this.devices.get(socketId);
		this.devices.delete(socketId);
		return device;
	};

	static getDevice = (socketId) => {
		return this.devices.get(socketId);
	};

	static getAllDevices = () => {
		// as plain js object
		const devices = {};
		for (const [key, value] of this.devices) {
			devices[key] = value;
		}
		return devices;
	};
}

module.exports = DeviceManager;
