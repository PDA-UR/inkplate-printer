import PageModel from "../data/PageModel";
import { Observable } from "../lib/Observable";
import ConnectionStatus from "../lib/ConnectionStatus";
import GestureController from "./GestureController";
import AccelerometerController from "./AccelerometerController";
import DevicePairingUpdateModel from "../data/DevicePairingUpdateModel";

export default class ViewController extends Observable {
	public static ON_NEXT_PAGE = "nextPageClicked";
	public static ON_PREVIOUS_PAGE = "previousPageClicked";
	public static ON_ENQUEUE = "enqueueClicked";

	// key events
	private static NEXT_PAGE_KEYS = ["ArrowRight", "l", "L"];
	private static PREVIOUS_PAGE_KEYS = ["ArrowLeft", "h", "H"];
	private static ENQUEUE_KEYS = ["Space", " ", "j", "J"];

	// pairing
	public static ON_PAIR_LEFT = "pairLeft";
	public static ON_PAIR_RIGHT = "pairRight";
	public static ON_UNPAIR = "unpair";
	public static ON_PAIR_BUTTON_CLICKED = "pairButtonClicked";

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
	private readonly $pageNumber = document.getElementById(
		"page-number"
	) as HTMLSpanElement;
	private readonly $pageCount = document.getElementById(
		"page-count"
	) as HTMLSpanElement;
	private readonly $pairingInfoIcon = document.getElementById(
		"pairing-info"
	) as HTMLDivElement;
	private readonly $pairingInfo = document.getElementById(
		"pairing-info"
	) as HTMLDivElement;
	private readonly $pairingInfoIndex = document.getElementById(
		"pairing-info-index"
	) as HTMLSpanElement;
	private readonly $pairingInfoCount = document.getElementById(
		"pairing-info-count"
	) as HTMLSpanElement;
	private readonly $pairMenuButton = document.getElementById(
		"pair-menu-button"
	) as HTMLButtonElement;

	private readonly gestureController = new GestureController(
		this.$hudContainer
	);

	private readonly accelerometerController = new AccelerometerController();

	constructor() {
		super();
		this.registerEvents();
	}

	// ~~~~~~~~~~~~ Public methods ~~~~~~~~~~~ //

	public setPage = (pageModel: PageModel, doAnimate = false): void => {
		let bmpBlob = pageModel.image;

		if (bmpBlob !== undefined) {
			if (doAnimate) {
				this.$currentPageImage.classList.add("page-print-animation");
				// remove after .5s
				setTimeout(() => {
					this.$currentPageImage.classList.remove("page-print-animation");
				}, 500);
			}
			this.$currentPageImage.src = URL.createObjectURL(bmpBlob);
		}

		this.setPageIndex(pageModel.index);
		this.enable(this.$nextPageButton);
		this.enable(this.$previousPageButton);
	};

	public setPageIndex = (pageIndex: number): void => {
		this.$pageNumber.innerHTML = pageIndex?.toString();
	};

	public setPageCount = (pageCount: number): void => {
		this.$pageCount.innerHTML = pageCount?.toString();
	};

	public setBlank = () => {
		console.log("set blank");
		this.toggleDeviceIndex(false);
		// super small blank image
		// from:https://stackoverflow.com/questions/19126185/setting-an-image-src-to-empty
		this.$currentPageImage.src =
			"data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7";
		this.disable(this.$nextPageButton);
		this.disable(this.$previousPageButton);
	};

	public setDeviceIndex = (deviceIndex: number): void => {
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
		this.$pairMenuButton.disabled =
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

	public setPairingIndex = (
		devicePairingUpdate: DevicePairingUpdateModel
	): void => {
		const deviceIndex = devicePairingUpdate.deviceIndex + 1;
		const numDevices = devicePairingUpdate.numDevices;
		if (deviceIndex < 1) {
			console.log("unpairing");
			this.$pairingInfoIcon.hidden = true;
			this.$pairingInfo.classList.remove("index-info");
		} else {
			console.log("pairing at index: " + deviceIndex);
			this.$pairingInfoIcon.hidden = false;
			this.$pairingInfo.classList.add("index-info");

			this.$pairingInfoIndex.innerText = deviceIndex.toString();
			this.$pairingInfoCount.innerText = numDevices.toString();
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

		this.$pairMenuButton.addEventListener("click", (e) => {
			this.consume(e);
			this.notifyAll(ViewController.ON_PAIR_BUTTON_CLICKED);
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

		// Accelerometer

		this.accelerometerController.on(
			AccelerometerController.ON_PAIR_LEFT,
			() => {
				this.toggleHud(true);
				this.notifyAll(ViewController.ON_PAIR_LEFT);
			}
		);

		this.accelerometerController.on(
			AccelerometerController.ON_PAIR_RIGHT,
			() => {
				this.toggleHud(true);
				this.notifyAll(ViewController.ON_PAIR_RIGHT);
			}
		);

		this.accelerometerController.on(AccelerometerController.ON_UNPAIR, () => {
			this.toggleHud(true);
			this.notifyAll(ViewController.ON_UNPAIR);
		});
	};

	public toggleHud = (on?: boolean) => {
		if (on === undefined) this.toggleClass(this.$hudContainer, "inactive");
		else this.toggleClass(this.$hudContainer, "inactive", !on);
	};

	private toggleDeviceIndex = (on?: boolean) => {
		if (on === undefined) this.toggleClass(this.$deviceIndexInfo, "inactive");
		else this.toggleClass(this.$deviceIndexInfo, "inactive", !on);
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
