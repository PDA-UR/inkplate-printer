class DeviceManager {
	// sockedId: string, device: Device
	static devices = new Map();

	static register = (socketId, device) => {
		console.log("Registering device", device);
		this.devices.set(socketId, device);
	};

	static unregister = (socketId) => {
		console.log("Unregistering device", socketId);
		this.devices.delete(socketId);
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
