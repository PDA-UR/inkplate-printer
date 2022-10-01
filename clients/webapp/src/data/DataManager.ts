import { get, set } from "idb-keyval";
import { v4 as uuidv4 } from "uuid";
import PageChainModel from "./PageChainModel";

export default class DataManager {
	static async savePageIndex(pageIndex: number) {
		await set("pageIndex", pageIndex);
	}

	static async getPageIndex() {
		return await get("pageIndex");
	}

	static async savePageChain(pageChain: PageChainModel) {
		await set("pageChain", pageChain);
	}

	static async getPageChain() {
		return await get("pageChain");
	}

	static async getUUID() {
		return await get("uuid");
	}

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
