// Called when a device registers that its ready for a new page chain

const QueueManager = require("../../QueueManager");

const on = (socket, event) => {
	console.log("unregister", event);
	if (event?.uuid) {
		QueueManager.dequeue(event.uuid);
	}
};

module.exports = on;
