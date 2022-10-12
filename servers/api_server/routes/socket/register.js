// Called when a device registers itself with uuid

const DeviceManager = require("../../DeviceManager");

const on = (socket, device) => {
	if (device.uuid && device.screenResolution) {
		DeviceManager.register(socket.id, device);
		socket.emit("registered", true);
	} else {
		console.error("invalid register event", device);
		socket.emit("registered", false);
	}
};

module.exports = on;
