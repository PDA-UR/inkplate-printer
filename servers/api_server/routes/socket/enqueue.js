// Called when a device registers that its ready for a new page chain

const QueueManager = require("../../QueueManager");
const DeviceManager = require("../../DeviceManager");

const on = (socket) => {
	const device = DeviceManager.getDevice(socket.id);
	if (device) {
		console.log("enqueueing ", device);
		QueueManager.enqueue(socket, device);
	} else {
		console.log("No device found for socket", socket.id);
	}
};

module.exports = on;
