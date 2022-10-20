const DeviceManager = require("../../DeviceManager");
const PairingManager = require("../../PairingManager");

const on = (socket, pageIndex) => {
	console.log("update page index", pageIndex);
	const device = DeviceManager.getDeviceBySocketId(socket.id);
	const pageChainId = device?.pageChainId;
	const deviceId = device?.uuid;
	if (
		pageIndex !== undefined &&
		pageChainId !== undefined &&
		deviceId !== undefined
	) {
		if (PairingManager.hasPairing(pageChainId)) {
			PairingManager.onUpdatePageIndex(pageChainId, deviceId, pageIndex);
		}
	}
};

module.exports = on;
