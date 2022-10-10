const os = require("os");

// REST API route to download a page

// Url: /api/img
// Params:
// 	client: string
// 	doc_name: string
// 	page_num: number

const route = {
	type: "get",
	on: (req, res) => {
		const clientMac = req.query.client,
			docName = req.query.doc_name,
			pageNum = req.query.page_num,
			filePath = getFilePath(clientMac, docName, pageNum),
			bitmap = getBitmap(filePath);

		if (bitmap) {
			res.setHeader("Content-Disposition", "attachment; filename=unknown.bmp");
			res.setHeader("Content-Type", "image/bmp");
			res.send(bitmap);
		} else {
			res.setHeader("Content-Type", "image/bmp");
			res.status(201);
			res.send("");
		}
	},
};

const serverHomePath = os.homedir() + "/.inkplate-printer",
	queuePath = serverHomePath + "/queue";

const getFilePath = (client_mac, doc_name, page_num) => {
	return queuePath + "/" + doc_name + "/" + page_num + ".bmp";
};

const getBitmap = (filePath) => {
	try {
		return fs.readFileSync(filePath);
	} catch {
		console.error("Page image file not found: " + filePath);
		return null;
	}
};

module.exports = route;
