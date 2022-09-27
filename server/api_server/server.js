const express = require("express");
const app = express();
const http = require("http");
const server = http.createServer(app);
const glob = require("glob");
const { Server } = require("socket.io");
const io = new Server(server);

importSocketRoutes().then((socketRoutes) => {
	startExpress();
	startSocketIO(socketRoutes);
});

// Starts the express server (HTTP)
function startExpress() {
	app.get("/", (req, res) => {
		res.sendFile(__dirname + "/public/index.html");
	});

	app.use("/public", express.static("public"));

	server.listen(3000, () => {
		console.log("listening on *:3000");
	});
}

// Starts the socket.io server
function startSocketIO(socketRoutes) {
	io.on("connection", (socket) => {
		console.log("a user connected");
		for (const [message, callback] of socketRoutes) {
			socket.on(message, (e) => callback(socket, e));
		}
	});
}

// import all api routes (.js files)
// they are located at api/
async function importSocketRoutes() {
	return new Promise((resolve, reject) => {
		const apiRoutes = new Map(); // message name -> route function
		glob("api/socket/*.js")
			.on("end", (files) => {
				for (const i in files) {
					const file = files[i];
					const route = require("./" + file);
					if (route.message && route.callback) {
						apiRoutes.set(route.message, route.callback);
					} else {
						reject(
							"api route " + file + " does not have a message or on function"
						);
					}
				}
				resolve(apiRoutes);
			})
			.on("error", (err) => reject(err));
	});
}
