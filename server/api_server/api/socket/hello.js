const route = {
	message: "hello",
	callback: (socket, e) => {
		console.log("test: " + e);
	},
};

module.exports = route;
