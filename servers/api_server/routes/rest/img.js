const os = require("os");
const fs = require("fs");

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
			pageNum = req.query.page_num,
			filePath = getFilePath(clientMac, pageNum),
			bitmap = getBitmap(filePath);
		console.log("GET /api/img", clientMac, pageNum, filePath);

		if (bitmap) {
			// send bmp as blob
			res.setHeader("Content-Type", "image/jpeg");
			res.setHeader(
				"Content-Disposition",
				"attachment; filename=" + pageNum + ".jpeg"
			);
			// set file size header
			console.log("len", bitmap.length);
			res.setHeader("Content-Length", bitmap.length);

			res.send(bitmap);
		} else {
			console.log("img not found");
			res.setHeader("Content-Type", "image/jpeg");
			res.status(201);
			res.send("");
		}
	},
};

const serverHomePath = os.homedir() + "/.inkplate-printer",
	imgPath = serverHomePath + "/img";

const getFilePath = (client_mac, page_num) => {
	return imgPath + "/" + client_mac + "/" + page_num + ".jpeg";
};

const getBitmap = (filePath) => {
	try {
		return fs.readFileSync(filePath);
	} catch (e) {
		console.error("Page image file not found: " + filePath, e);
		return null;
	}
};

module.exports = route;

// 2899214 1431655767
