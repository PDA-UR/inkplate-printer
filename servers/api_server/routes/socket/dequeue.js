// Called when a device registers that its ready for a new page chain

const QueueManager = require("../../QueueManager");
const DeviceManager = require("../../DeviceManager");

const on = (socket) => {
	const device = DeviceManager.getDeviceBySocketId(socket.id);

	if (device) {
		console.log("dequeueing ", device);
		QueueManager.dequeue(device.uuid);
	} else {
		console.log("No device found for socket", socket.id);
	}
};

module.exports = on;
