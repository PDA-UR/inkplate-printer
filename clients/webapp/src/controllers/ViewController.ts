import PageModel from "../data/PageModel";
import { Observable } from "../lib/Observable";

export default class ViewController extends Observable {
	public static ON_NEXT_PAGE_CLICKED = "nextPageClicked";
	public static ON_PREVIOUS_PAGE_CLICKED = "previousPageClicked";
	public static ON_REGISTER_DEVICE_CLICKED = "registerDeviceClicked";

	private readonly $nextPageButton = document.getElementById(
		"next-page-button"
	) as HTMLButtonElement;
	private readonly $previousPageButton = document.getElementById(
		"previous-page-button"
	) as HTMLButtonElement;
	private readonly $requestPageChainButton = document.getElementById(
		"request-page-chain-button"
	) as HTMLButtonElement;
	private readonly $currentPageImage = document.getElementById(
		"page-image"
	) as HTMLImageElement;
	private readonly $deviceIndex = document.getElementById(
		"device-index"
	) as HTMLSpanElement;

	constructor() {
		super();
		this.registerEvents();
	}

	// ~~~~~~~~~~~~ Public methods ~~~~~~~~~~~ //

	public setPage = (pageModel: PageModel): void => {
		this.$deviceIndex.hidden = true;
		const url = URL.createObjectURL(pageModel.image);
		this.$currentPageImage.src = url;
		this.$nextPageButton.disabled = false;
		this.$previousPageButton.disabled = false;
	};

	public setBlank() {
		this.$deviceIndex.hidden = true;
		this.$nextPageButton.disabled = true;
		this.$previousPageButton.disabled = true;
		this.$currentPageImage.src = "";
	}

	public setRegistering = (deviceIndex: number): void => {
		if (deviceIndex === -1) {
			this.$deviceIndex.hidden = true;
		} else {
			this.$deviceIndex.hidden = false;
			this.$deviceIndex.innerText = deviceIndex.toString();
		}
	};

	// ~~~~~~~~~~~~ Event handling ~~~~~~~~~~~ //

	private registerEvents = (): void => {
		this.$nextPageButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_NEXT_PAGE_CLICKED);
		});

		this.$previousPageButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_PREVIOUS_PAGE_CLICKED);
		});

		this.$requestPageChainButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_REGISTER_DEVICE_CLICKED);
		});
	};
}
