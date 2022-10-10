export default class ApiClient {
	static async getPageImage(
		docName: string,
		pageIndex: number,
		uuid: string
	): Promise<string> {
		const url = "/api/img";
		const urlParams = {
			client: uuid,
			doc_name: docName,
			page_num: pageIndex,
		};

		// response if of type image/bmp
		const response = await fetch(url, {
			method: "GET",
			headers: {
				"Content-Type": "application/json",
			},
			body: JSON.stringify(urlParams),
		});

		const image = await response.text();
		return image;
	}
}
