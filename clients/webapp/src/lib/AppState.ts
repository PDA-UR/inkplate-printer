export enum DisplayMode {
	BLANK,
	DISPLAYING,
}

class AppState {
	mode: DisplayMode;

	constructor() {
		this.mode = DisplayMode.BLANK;
	}

	public toggleDisplayMode = (doToggleOn?: boolean) => {
		if (doToggleOn === undefined) {
			this.mode =
				this.mode === DisplayMode.DISPLAYING
					? DisplayMode.BLANK
					: DisplayMode.DISPLAYING;
		} else {
			this.mode = doToggleOn ? DisplayMode.DISPLAYING : DisplayMode.BLANK;
		}
		return this.mode;
	};
}

const State: AppState = new AppState();

export default State;
