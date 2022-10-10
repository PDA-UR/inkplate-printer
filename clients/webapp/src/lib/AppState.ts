import DataManager from "../data/DataManager";

export enum AppMode {
	BLANK,
	REGISTERING,
	DISPLAYING,
}

class AppState {
	mode: AppMode;

	constructor() {
		this.mode = AppMode.BLANK;
	}

	public toggleRegisterMode = async (): Promise<AppMode> => {
		if (this.mode !== AppMode.REGISTERING) {
			this.mode = AppMode.REGISTERING;
		} else {
			const hasPageChain = await DataManager.hasPageChain();
			if (hasPageChain) {
				this.mode = AppMode.DISPLAYING;
			} else {
				this.mode = AppMode.BLANK;
			}
		}
		console.log("toggle", this);
		return this.mode;
	};
}

const State: AppState = new AppState();

export default State;
