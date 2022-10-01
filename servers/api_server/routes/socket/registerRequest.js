// Called when a device registers that its ready for a new page chain

const on = (socket, event) => {
	console.log("device ready for new page chain", event);
};

module.exports = on;
