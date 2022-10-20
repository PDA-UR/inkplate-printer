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
const DeviceManager = require("./DeviceManager");
const QueueManager = require("./QueueManager");
const PairingManager = require("./PairingManager");

const socketRouteHandlers = [],
	restRouteHandlers = [];

initSocketRouteHandlers()
	.then(() => initRestRouteHandlers())
	.then(() => {
		QueueManager.init();
		startExpress();
		startSocketIO();
	});

// Starts the express server (HTTP)
function startExpress() {
	app.get(
		"/",
		(webapp = (req, res) => {
			res.sendFile(__dirname + "/public/index.html");
		})
	);

	app.get(
		"/cert",
		(on = (req, res) => {
			res.sendFile(__dirname + "/ssl/cert.pem");
		})
	);

	// register dynamic routes
	restRouteHandlers.forEach((route) => {
		console.log("Registering route: " + route.path);
		app[route.type](route.path, route.callback);
	});

	// serve static files in public/ under /
	app.use(express.static("public"));

	server.listen(8000, () => {
		console.log("listening on *:8000");
	});

	if (process.env.NODE_ENV === "development") {
		// Only in dev environment

		// Absolute path to output file
		var path = require("path");
		var filepath = path.join(__dirname, "./docs/rest.routes.generated.txt");

		// Invoke express-print-routes
		require("express-print-routes")(app, filepath);
	}
}

// Starts the socket.io server
function startSocketIO() {
	io.on("connection", (socket) => {
		for (const [message, callback] of socketRouteHandlers) {
			socket.on(message, (e) => callback(socket, e));
		}

		socket.on("disconnect", () => {
			const device = DeviceManager.unregister(socket.id);
			console.log("Device disconnected: " + device.uuid);
			if (device?.uuid) {
				QueueManager.dequeue(device.uuid);
				PairingManager.unpair(device.pageChainId, device.uuid);
			}
		});
	});
}

// import all api routes (.js files)
// they are located at api/
async function getFilesInDir(pattern) {
	return new Promise((resolve, reject) => {
		glob(pattern)
			.on("end", (files) => {
				resolve(files);
			})
			.on("error", (err) => reject(err));
	});
}

async function initRestRouteHandlers() {
	return getFilesInDir("routes/rest/*.js")
		.then((files) => importModules(files, restRouteImportHandler))
		.then((restRoutes) => restRouteHandlers.push(...restRoutes));
}

function restRouteImportHandler(file) {
	return new Promise((resolve, reject) => {
		const route = require("./" + file),
			type = route.type?.toLowerCase(),
			path = getRestRoutePath(file),
			callback = route.on;

		if (route && type && path && callback) {
			if (typeof callback === "function") {
				resolve({ type, path, callback });
			} else {
				reject("Rest route " + file + " does not export a function");
			}
		} else {
			reject("Rest route " + file + " does not have a route or on function");
		}
	});
}

function getRestRoutePath(file) {
	try {
		let path = file.split(".")[0];
		// remove "routes/rest/" from path
		path = path.split("/").slice(2).join("/");
		// prepend "api"
		return "/api/" + path;
	} catch (err) {
		console.error("Invalid rest route file name: " + file);
		return null;
	}
}

async function initSocketRouteHandlers() {
	return getFilesInDir("routes/socket/*.js")
		.then((files) => importModules(files, socketRouteImportHandler))
		.then((socketRoutes) => socketRouteHandlers.push(...socketRoutes));
}

function importModules(files, importHandle) {
	const jobs = [];
	for (const i in files) {
		const file = files[i];
		jobs.push(importHandle(file));
	}
	return Promise.all(jobs);
}

function socketRouteImportHandler(file) {
	return new Promise((resolve, reject) => {
		const callback = require("./" + file);
		// check if callback is a function
		const message = getSocketRouteMessage(file);
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
