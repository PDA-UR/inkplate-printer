var fs = require("fs"),
	gm = require("gm"),
	YAML = require("js-yaml");
var Printer = require("ipp-printer");
const path = require("path");

const queueFolderPath = path.join(
	require("os").homedir(),
	".inkplate-printer",
	"print_queue"
);

const config = {
	printer_name: "Inkplate Printer",
	port: 3000,
};

console.log(
	`Starting PRINT server named ${config.printer_name} on port ${config.port}`
);

var printer = new Printer({
	name: config.printer_name,
	port: config.port,
});

printer.on("job", function (job) {
	if (printQueueIsOpen()) {
		saveDocument(job);
	} else {
		console.log("Print queue is closed", queueFolderPath);
		job.cancel();
	}
});

function saveDocument(job) {
	console.log("[job %d] Printing document: %s", job.id, job.name);
	var filename = "job-" + job.id + ".ps"; // .ps = PostScript
	const filePath = path.join(queueFolderPath, filename);
	var file = fs.createWriteStream(filePath);
	job.pipe(file);

	job.on("end", function () {
		console.log("[job %d] Document saved as %s", job.id, file.path);
	});
}

function printQueueIsOpen() {
	return fs.existsSync(queueFolderPath);
}
