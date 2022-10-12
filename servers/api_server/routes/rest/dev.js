// Dev dashboard for testing and debugging

const DeviceManager = require("../../DeviceManager");
const QueueManager = require("../../QueueManager");

// Really ugly, but it works

const route = {
	type: "get",
	on: (req, res) => {
		const style = buildStyle();
		const devices = DeviceManager.getAllDevices();
		const deviceHeaderColumns = buildDeviceHeaderColumns(
			Object.values(devices)[0]
		);
		const deviceRows = buildDeviceRows(devices);

		const html = `
        <html>
            <head>
                <title>Dev Dashboard</title>
            </head>
            <body>
                ${style}
                <h1>Dev Dashboard</h1>
                <h2>Connected devices</h2>
                <table>
                    <tr>
                        <th>Socket ID</th>
                        ${deviceHeaderColumns}
                    </tr>
                    ${deviceRows}
                </table>

                <h2>Print Queue</h2>
                ${buildQueueStatus()}
                ${buildPrintQueueDevices()}
            </body>
        </html>
        `;

		res.send(html);
	},
};

function buildStyle() {
	return `
        <style>
            table {
                border-collapse: collapse;
            }
            table, th, td {
                border: 1px solid black;
            }
        </style>
    `;
}

function buildDeviceHeaderColumns(device) {
	const columns = [];
	if (device) {
		for (const key in device) {
			columns.push(`<th>${key}</th>`);
		}
	}
	return columns.join("");
}

function buildDeviceDataColumns(device) {
	const columns = [];
	for (const key in device) {
		const property = device[key];
		try {
			columns.push(`<td>${JSON.stringify(property)}</td>`);
		} catch (error) {
			columns.push(`<td>${property}</td>`);
		}
	}
	return columns.join("");
}

function buildDeviceRows(devices) {
	const rows = [];
	for (const sockedId in devices) {
		const device = devices[sockedId];
		rows.push(`<tr><td>${sockedId}</td>${buildDeviceDataColumns(device)}</tr>`);
	}
	return rows.join("");
}

function buildQueueStatus() {
	const isOpen = QueueManager.isOpen();
	// status text with green/red color on the word "open" or "closed"
	return `
        <p>Queue is <span style="color: ${isOpen ? "green" : "red"}">${
		isOpen ? "open" : "closed"
	}</span></p>
    `;
}

function buildPrintQueueDevicesRow(device, isHeader) {
	const columns = [];
	if (device) {
		for (const key in device) {
			let content = isHeader ? key : device[key];
			// if object
			if (typeof content === "object") {
				try {
					content = JSON.stringify(content);
				} catch (error) {
					content = content;
				}
			}
			if (key === "index") {
				// prepend
				columns.unshift(`<th>${content}</th>`);
			} else if (key !== "socket") {
				columns.push(`<th>${content}</th>`);
			}
		}
	}
	return `<tr>${columns.join("")}</tr>`;
}

function buildPrintQueueDevicesRows(devices) {
	const rows = [];
	for (const device of devices) {
		rows.push(`<tr>${buildPrintQueueDevicesRow(device, false)}</tr>`);
	}
	return rows.join("");
}

function buildPrintQueueDevices() {
	const isOpen = QueueManager.isOpen();
	if (isOpen) {
		const devices = QueueManager.getDevices();
		// sort by property index
		const header = buildPrintQueueDevicesRow(devices[0], true);
		const rows = buildPrintQueueDevicesRows(devices);

		return `    
            <table>
                ${header}
                ${rows}
            </table>
        `;
	}
	return "";
}

module.exports = route;
