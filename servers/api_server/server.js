const fs = require("fs");
const key = fs.readFileSync("./ssl/key.pem");
const cert = fs.readFileSync("./ssl/cert.pem");

const express = require("express");
const app = express();
const https = require("https");
const server = https.createServer({ key, cert }, app);
const glob = require("glob");
const { Server } = require("socket.io");
const io = new Server(server);

getSocketRouteFiles()
	.then((files) => importSocketRouteHandlers(files))
	.then((socketRoutes) => {
		startExpress();
		startSocketIO(socketRoutes);
	});

// Starts the express server (HTTP)
function startExpress() {
	app.get("/", (req, res) => {
		res.sendFile(__dirname + "/public/index.html");
	});

	app.get("/cert", (req, res) => {
		res.sendFile(__dirname + "/ssl/cert.pem");
	});

	// serve static files in public/
	app.use(express.static("public"));

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
async function getSocketRouteFiles() {
	return new Promise((resolve, reject) => {
		const files = new Map(); // message name -> route function
		glob("routes/socket/*.js")
			.on("end", (files) => {
				resolve(files);
			})
			.on("error", (err) => reject(err));
	});
}

function importSocketRouteHandlers(files) {
	const jobs = [];
	for (const i in files) {
		const file = files[i];
		console.log("importing " + file);
		jobs.push(getSocketRoute(file));
	}
	return Promise.all(jobs);
}

function getSocketRoute(file) {
	return new Promise((resolve, reject) => {
		const callback = require("./" + file);
		// check if callback is a function

		const message = getSocketRouteMessage(file);
		console.log("message: " + message);
		if (message && callback) {
			if (typeof callback === "function") {
				resolve([message, callback]);
			} else {
				reject("Socket route " + file + " does not export a function");
			}
		} else {
			reject(
				"Socket route " + file + " does not have a message or on function"
			);
		}
	});
}

function getSocketRouteMessage(file) {
	try {
		let message = file.split("/").pop().split(".")[0];
		return message;
	} catch (err) {
		console.error("Invalid socket route file name: " + file);
		return null;
	}
}
