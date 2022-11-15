export default class ApiClient {
	static async getPageImage(pageIndex: number, uuid: string): Promise<Blob> {
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

		return response.blob();
	}
}
