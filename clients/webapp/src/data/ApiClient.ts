export default class ApiClient {
	static async getPageImage(pageIndex: number, uuid: string): Promise<string> {
		const url = "/api/img?";
		const params = new URLSearchParams({
			client: uuid,
			page_num: pageIndex.toString(),
		}).toString();

		console.log("Fetching page", pageIndex, "from", url + params);
		// response if of type image/bmp
		const response = await fetch(url + params);

		if (response.status !== 200) {
			return Promise.reject("Failed to get page image");
		}

		const b64Image = await response.text();
		return b64Image;
	}
}
