export default interface DeviceModel {
	uuid: string;
	screenInfo: ScreenInfo;
	isBrowser: boolean;
}

export interface ScreenInfo {
	resolution: {
		width: number;
		height: number;
	};
	colorDepth: number;
	dpi: number;
}

export const getScreenInfo = (): ScreenInfo => {
	return {
		resolution: {
			width: window.innerWidth,
			height: window.innerHeight,
		},
		colorDepth: window.screen.colorDepth,
		dpi: getDpi(),
	};
};

const getDpi = (): number => {
	const dpr = window.devicePixelRatio || 1;
	return Math.round(dpr * 96);
};
