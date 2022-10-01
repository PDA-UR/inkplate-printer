// Called when a device registers that its ready for a new page chain

const QueueManager = require("../../QueueManager");

const on = (socket, event) => {
	console.log("device ready for new page chain", event);
	QueueManager.enqueue(socket, event);
};

module.exports = on;