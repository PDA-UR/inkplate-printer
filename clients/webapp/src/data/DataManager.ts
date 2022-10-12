import { get, set } from "idb-keyval";
import { v4 as uuidv4 } from "uuid";
import ApiClient from "./ApiClient";
import PageChainInfoModel from "./PageChainInfoModel";
import PageModel from "./PageModel";

export default class DataManager {
	// ~~~~~~~~~~~~~ Device index ~~~~~~~~~~~~ //

	static async saveDeviceIndex(deviceIndex: number) {
		await set("deviceIndex", deviceIndex);
	}

	static async getDeviceIndex(): Promise<number | undefined> {
		return await get("deviceIndex");
	}

	// ~~~~~~~~~~~ Page chain info ~~~~~~~~~~~ //

	static async savePageChainInfo(pageChain: PageChainInfoModel) {
		await set("pageChain", pageChain);
	}

	static async getPageChainInfo(): Promise<PageChainInfoModel | undefined> {
		return await get("pageChain");
	}

	static async hasPageChain(): Promise<boolean> {
		const pageChainInfo = await DataManager.getPageChainInfo();
		return pageChainInfo !== undefined;
	}

	// ~~~~~~~~~~ Current page index ~~~~~~~~~ //

	static async saveCurrentPageIndex(pageIndex: number) {
		await set("pageIndex", pageIndex);
	}

	static async getCurrentPageIndex(): Promise<number | undefined> {
		return await get("pageIndex");
	}

	static async stepCurrentPageIndex(
		doGoNext: boolean
	): Promise<number | undefined> {
		const currentPageIndex = await DataManager.getCurrentPageIndex();
		if (currentPageIndex !== undefined) {
			const newPageIndex = currentPageIndex + (doGoNext ? 1 : -1);
			await DataManager.saveCurrentPageIndex(newPageIndex);
			return newPageIndex;
		} else {
			return undefined;
		}
	}

	// ~~~~~~~~~~~~~ Single pages ~~~~~~~~~~~~ //

	static async getPage(pageIndex: number): Promise<PageModel | undefined> {
		console.log("Getting page", pageIndex);
		const pageModel: PageModel | undefined = await get(
			"page-" + pageIndex.toString()
		);
		console.log("Got image", pageModel);
		if (pageModel === undefined || pageModel.image === undefined) {
			// try to download the page
			const uuid = await DataManager.getUUID();
			if (uuid === undefined) {
				return undefined;
			}

			return ApiClient.getPageImage(pageIndex, uuid)
				.then((image) => {
					const pageModel: PageModel = {
						index: pageIndex,
						image: image,
					};
					return DataManager.savePage(pageModel);
				})
				.then(() => DataManager.getPage(pageIndex));
		}
		return pageModel;
	}

	static async savePage(page: PageModel) {
		await set("page-" + page.index.toString(), page);
	}

	static async getCurrentPage(): Promise<PageModel | undefined> {
		console.log("Getting current page");
		const pageIndex = await DataManager.getCurrentPageIndex();
		console.log("Current page index is", pageIndex);
		if (pageIndex === undefined) {
			return undefined;
		}
		return await DataManager.getPage(pageIndex);
	}

	// ~~~~~~~~~~~~ Multiple pages ~~~~~~~~~~~ //

	static async getAllExceptCurrentPage(): Promise<PageModel[]> {
		const pageIndex = await DataManager.getCurrentPageIndex();
		if (pageIndex === undefined) {
			return [];
		}
		const pageChainInfo = await DataManager.getPageChainInfo();
		if (pageChainInfo === undefined) {
			return [];
		}

		const pages: PageModel[] = [];
		for (let i = 0; i < pageChainInfo.pageCount; i++) {
			if (i !== pageIndex) {
				const pm = await DataManager.getPage(i);
				if (pm) pages.push(pm);
			}
		}
		return pages;
	}

	// ~~~~~~~~~~~~~~~~ UUID ~~~~~~~~~~~~~~~~ //

	static async getUUID(): Promise<string | undefined> {
		return await get("uuid");
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
