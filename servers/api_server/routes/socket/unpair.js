const PairingManager = require("../../PairingManager");
const DeviceManager = require("../../DeviceManager");

const on = (socket, event) => {
	console.log("un pair");
	const device = DeviceManager.getDeviceBySocketId(socket.id);
	const pageChainId = device?.pageChainId;
	const deviceId = device?.uuid;
	if (pageChainId !== undefined && deviceId !== undefined) {
		PairingManager.unpair(pageChainId, deviceId);
	}
};

module.exports = on;
