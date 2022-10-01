import { Observable } from "../lib/Observable";

export default class ViewController extends Observable {
	public static ON_NEXT_PAGE_CLICKED = "nextPageClicked";
	public static ON_PREVIOUS_PAGE_CLICKED = "previousPageClicked";
	public static ON_REQUEST_PAGE_CHAIN_CLICKED = "requestPageChainClicked";

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

	constructor() {
		super();
		this.registerEvents();
	}

	private registerEvents = (): void => {
		this.$nextPageButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_NEXT_PAGE_CLICKED);
		});

		this.$previousPageButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_PREVIOUS_PAGE_CLICKED);
		});

		this.$requestPageChainButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_REQUEST_PAGE_CHAIN_CLICKED);
		});
	};
}
