import PageModel from "../data/PageModel";
import { Observable } from "../lib/Observable";
import ConnectionStatus from "../lib/ConnectionStatus";
import GestureController from "./GestureController";

export default class ViewController extends Observable {
	public static ON_NEXT_PAGE = "nextPageClicked";
	public static ON_PREVIOUS_PAGE = "previousPageClicked";
	public static ON_ENQUEUE = "enqueueClicked";

	// key events
	private static NEXT_PAGE_KEYS = ["ArrowRight", "l", "L"];
	private static PREVIOUS_PAGE_KEYS = ["ArrowLeft", "h", "H"];
	private static ENQUEUE_KEYS = ["Space", "j", "J"];

	private readonly $hudContainer = document.getElementById(
		"hud-container"
	) as HTMLDivElement;
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

	private readonly touchController = new GestureController(this.$hudContainer);

	constructor() {
		super();
		this.registerEvents();
	}

	// ~~~~~~~~~~~~ Public methods ~~~~~~~~~~~ //

	public setPage = (pageModel: PageModel): void => {
		const url = URL.createObjectURL(pageModel.image);
		this.$currentPageImage.src = url;
		this.$nextPageButton.disabled = false;
		this.$previousPageButton.disabled = false;
	};

	public setBlank() {
		this.toggleDeviceIndex(false);
		this.$nextPageButton.disabled = true;
		this.$previousPageButton.disabled = true;
		this.$currentPageImage.src = "";
	}

	public setRegistering = (deviceIndex: number): void => {
		if (deviceIndex === -1) {
			this.toggleDeviceIndex(false);
		} else {
			this.toggleDeviceIndex(true);
			this.$deviceIndex.innerText = deviceIndex.toString();
		}
	};

	public setConnectionStatus = (connectionStatus: ConnectionStatus): void => {
		this.$requestPageChainButton.disabled =
			connectionStatus !== ConnectionStatus.CONNECTED;

		const bgColor =
			connectionStatus === ConnectionStatus.CONNECTED
				? "green"
				: connectionStatus === ConnectionStatus.REGISTERING
				? "yellow"
				: "red";

		document.body.style.backgroundColor = bgColor;
	};

	// ~~~~~~~~~~~~ Event handling ~~~~~~~~~~~ //

	private registerEvents = (): void => {
		// Click

		this.$nextPageButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_NEXT_PAGE);
		});

		this.$previousPageButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_PREVIOUS_PAGE);
		});

		this.$requestPageChainButton.addEventListener("click", () => {
			this.notifyAll(ViewController.ON_ENQUEUE);
		});

		document.addEventListener("keydown", (event) => {
			if (event.key === "ArrowRight") {
				this.notifyAll(ViewController.ON_NEXT_PAGE);
			} else if (event.key === "ArrowLeft") {
				this.notifyAll(ViewController.ON_PREVIOUS_PAGE);
			}
		});

		// Key

		document.addEventListener("keydown", (event) => {
			if (ViewController.NEXT_PAGE_KEYS.includes(event.key)) {
				this.notifyAll(ViewController.ON_NEXT_PAGE);
			} else if (ViewController.PREVIOUS_PAGE_KEYS.includes(event.key)) {
				this.notifyAll(ViewController.ON_PREVIOUS_PAGE);
			} else if (ViewController.ENQUEUE_KEYS.includes(event.key)) {
				this.notifyAll(ViewController.ON_ENQUEUE);
			}
		});

		// Touch

		this.touchController.on(GestureController.ON_SWIPE_LEFT, () =>
			this.notifyAll(ViewController.ON_NEXT_PAGE)
		);
		this.touchController.on(GestureController.ON_SWIPE_RIGHT, () =>
			this.notifyAll(ViewController.ON_PREVIOUS_PAGE)
		);
		this.touchController.on(GestureController.ON_SWIPE_DOWN, () =>
			this.notifyAll(ViewController.ON_ENQUEUE)
		);
		this.touchController.on(GestureController.ON_TAP, () => this.toggleHud());
	};

	private toggleHud = (on?: boolean) => {
		this.toggleClass(this.$hudContainer, "inactive", on);
	};

	private toggleDeviceIndex = (on?: boolean) => {
		this.toggleClass(this.$deviceIndex, "inactive", !on);
	};

	private toggleClass = (
		element: HTMLElement,
		className: string,
		on?: boolean
	) => {
		if (on === undefined) {
			element.classList.toggle(className);
		} else {
			element.classList.toggle(className, on);
		}
	};
}
