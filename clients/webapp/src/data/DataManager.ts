import { get, set } from "idb-keyval";
import { v4 as uuidv4 } from "uuid";
import PageChainInfoModel from "./PageChainInfoModel";
import PageModel from "./PageModel";

export default class DataManager {
	// ~~~~~~~~~~~~~ Device index ~~~~~~~~~~~~ //

	static async saveDeviceIndex(deviceIndex: number) {
		await set("deviceIndex", deviceIndex);
	}

	static async getDeviceIndex(): Promise<number> {
		const deviceIndex = await get("deviceIndex");
		if (deviceIndex === undefined) {
			return Promise.reject("deviceIndex is undefined");
		}
		return deviceIndex;
	}

	// ~~~~~~~~~~~ Page chain info ~~~~~~~~~~~ //

	static async savePageChainInfo(pageChain: PageChainInfoModel) {
		await set("pageChain", pageChain);
	}

	static async getPageChainInfo(): Promise<PageChainInfoModel> {
		const pageChain = await get("pageChain");
		if (pageChain === undefined) {
			return Promise.reject("pageChain is undefined");
		}
		return pageChain;
	}

	static async hasPageChain(): Promise<boolean> {
		return this.getPageChainInfo()
			.then(() => true)
			.catch(() => false);
	}

	// ~~~~~~~~~~ Current page index ~~~~~~~~~ //

	static async saveCurrentPageIndex(pageIndex: number) {
		await set("pageIndex", pageIndex);
	}

	static async getCurrentPageIndex(): Promise<number> {
		const index = await get("pageIndex");
		if (index !== undefined) {
			return index;
		} else {
			return Promise.reject("Index undefined");
		}
	}

	static async stepCurrentPageIndex(doGoNext: boolean): Promise<number> {
		const currentPageIndex = await this.getCurrentPageIndex();
		const newPageIndex = currentPageIndex + (doGoNext ? 1 : -1);
		await this.saveCurrentPageIndex(newPageIndex);
		return newPageIndex;
	}

	// ~~~~~~~~~~~~~ Single pages ~~~~~~~~~~~~ //

	static async getPage(pageIndex: number): Promise<PageModel> {
		// mock data for now
		// TODO: remove this
		const mockImage = "https://openclipart.org/image/800px/269610";
		return {
			image: mockImage,
			index: pageIndex,
		};

		const image: string | undefined = await get("page-" + pageIndex.toString());
		if (image === undefined) {
			return Promise.reject("image is undefined");
		}
		return {
			index: pageIndex,
			image: image,
		} as PageModel;
	}

	static async savePage(page: PageModel) {
		await set("page-" + page.index.toString(), page);
	}

	// ~~~~~~~~~~~~~~~~ UUID ~~~~~~~~~~~~~~~~ //

	static async getUUID(): Promise<string> {
		const uuid = await get("uuid");
		if (uuid !== undefined) {
			return uuid;
		} else {
			return Promise.reject("UUID undefined");
		}
	}

	// ~~~~~~~~~ Initialization stuff ~~~~~~~~ //

	static async load() {
		if (!(await DataManager.dataStoreIsValid())) {
			return await DataManager.init();
		}
		return Promise.resolve();
	}

	// initializes the data store
	private static async init() {
		const uuid = uuidv4();
		await DataManager.saveUUID(uuid);
	}

	private static async dataStoreIsValid() {
		const uuid = await DataManager.getUUID();
		return uuid !== undefined;
	}

	private static async saveUUID(uuid: string) {
		await set("uuid", uuid);
	}
}
