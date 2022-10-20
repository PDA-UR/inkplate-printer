const DeviceManager = require("../../DeviceManager");
const PairingManager = require("../../PairingManager");

const on = (socket, doPrepend) => {
	console.log("pair");
	const device = DeviceManager.getDeviceBySocketId(socket.id);
	const pageChainId = device?.pageChainId;
	const deviceId = device?.uuid;
	if (pageChainId !== undefined && deviceId !== undefined) {
		PairingManager.pair(pageChainId, deviceId, doPrepend);
	}
};

module.exports = on;
