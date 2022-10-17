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
	private readonly $deviceIndexInfo = document.getElementById(
		"device-index"
	) as HTMLSpanElement;
	private readonly $connectionStatus = document.getElementById(
		"connection-status"
	) as HTMLDivElement;

	private readonly gestureController = new GestureController(
		this.$hudContainer
	);

	constructor() {
		super();
		this.registerEvents();
	}

	// ~~~~~~~~~~~~ Public methods ~~~~~~~~~~~ //

	public setPage = (pageModel: PageModel): void => {
		let b64Bmp = pageModel.image;
		this.$currentPageImage.src = `data:image/bmp;base64,${b64Bmp}`;

		this.enable(this.$nextPageButton);
		this.enable(this.$previousPageButton);
	};

	public setBlank = () => {
		console.log("set blank");
		this.toggleDeviceIndex(false);
		this.$currentPageImage.src = "";
		this.disable(this.$nextPageButton);
		this.disable(this.$previousPageButton);
	};

	public setRegistering = (deviceIndex: number): void => {
		if (deviceIndex === -1) {
			this.toggleDeviceIndex(false);
		} else {
			this.toggleDeviceIndex(true);
			this.$deviceIndexInfo.innerText = deviceIndex.toString();
		}
	};

	public setConnectionStatus = (connectionStatus: ConnectionStatus): void => {
		this.$requestPageChainButton.disabled =
			connectionStatus !== ConnectionStatus.CONNECTED;
		this.$connectionStatus.classList.remove("blink");

		switch (connectionStatus) {
			case ConnectionStatus.CONNECTED:
				this.$connectionStatus.style.backgroundColor = "green";
				break;
			case ConnectionStatus.QUEUEING:
				this.$connectionStatus.style.backgroundColor = "green";
				this.$connectionStatus.classList.add("blink");
				break;
			case ConnectionStatus.REGISTERING:
				this.$connectionStatus.style.backgroundColor = "yellow";
				break;
		}
	};

	// ~~~~~~~~~~~~ Event handling ~~~~~~~~~~~ //

	private registerEvents = (): void => {
		// Click

		this.$nextPageButton.addEventListener("click", (e) => {
			this.consume(e);
			console.log("Next page button clicked");
			this.notifyAll(ViewController.ON_NEXT_PAGE);
		});

		this.$previousPageButton.addEventListener("click", (e) => {
			this.consume(e);
			this.notifyAll(ViewController.ON_PREVIOUS_PAGE);
		});

		this.$requestPageChainButton.addEventListener("click", (e) => {
			this.consume(e);
			this.notifyAll(ViewController.ON_ENQUEUE);
		});

		this.$deviceIndexInfo.addEventListener("click", (e) => {
			this.consume(e);
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

		this.gestureController.on(GestureController.ON_SWIPE_LEFT, () =>
			this.notifyAll(ViewController.ON_NEXT_PAGE)
		);
		this.gestureController.on(GestureController.ON_SWIPE_RIGHT, () =>
			this.notifyAll(ViewController.ON_PREVIOUS_PAGE)
		);
		this.gestureController.on(GestureController.ON_SWIPE_DOWN, () =>
			this.notifyAll(ViewController.ON_ENQUEUE)
		);
		this.$hudContainer.addEventListener("click", () => {
			console.log("HUD clicked");
			this.toggleHud();
		});
	};

	private toggleHud = (on?: boolean) => {
		this.toggleClass(this.$hudContainer, "inactive", on);
	};

	private toggleDeviceIndex = (on?: boolean) => {
		this.toggleClass(this.$deviceIndexInfo, "inactive", !on);
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

	// util function to stop an event
	private consume = (event: Event) => {
		event.preventDefault();
		event.stopPropagation();
	};

	private disable = (element: HTMLElement) => {
		element.classList.add("disabled");
	};

	private enable = (element: HTMLElement) => {
		element.classList.remove("disabled");
	};
}
