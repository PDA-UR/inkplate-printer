import { Observable } from "../lib/Observable";

export default class GestureController extends Observable {
	public static readonly ON_TOUCH_START = "touchstart";
	public static readonly ON_TOUCH_END = "touchend";

	public static readonly ON_SWIPE_DOWN = "swipeDown";
	public static readonly ON_SWIPE_UP = "swipeUp";
	public static readonly ON_SWIPE_LEFT = "swipeLeft";
	public static readonly ON_SWIPE_RIGHT = "swipeRight";

	private readonly $element: HTMLElement;
	private readonly swipeThreshold: number;
	private touchStartX = 0;
	private touchStartY = 0;

	constructor($element: HTMLElement, swipeThreshold = 100) {
		super();
		this.$element = $element;
		this.swipeThreshold = swipeThreshold;
		this.registerEvents();
	}

	private registerEvents() {
		// touch
		this.$element.addEventListener(
			GestureController.ON_TOUCH_START,
			({ touches }) => this.onTouchStart(touches[0].clientX, touches[0].clientY)
		);
		this.$element.addEventListener(
			GestureController.ON_TOUCH_END,
			({ changedTouches }) =>
				this.onTouchEnd(changedTouches[0].clientX, changedTouches[0].clientY)
		);

		// mouse
		this.$element.addEventListener("mousedown", ({ clientX, clientY }) =>
			this.onTouchStart(clientX, clientY)
		);
		this.$element.addEventListener("mouseup", ({ clientX, clientY }) =>
			this.onTouchEnd(clientX, clientY)
		);
	}

	private onTouchStart = (x: number, y: number) => {
		this.touchStartX = x;
		this.touchStartY = y;
		this.notifyAll(GestureController.ON_TOUCH_START);
	};

	private onTouchEnd = (x: number, y: number) => {
		const xDiff = this.touchStartX - x;
		const xDiffAbs = Math.abs(xDiff);
		const yDiff = this.touchStartY - y;
		const yDiffAbs = Math.abs(yDiff);

		if (xDiffAbs > yDiffAbs && xDiffAbs > this.swipeThreshold) {
			if (xDiff > 0) {
				this.notifyAll(GestureController.ON_SWIPE_LEFT);
			} else {
				this.notifyAll(GestureController.ON_SWIPE_RIGHT);
			}
		} else if (xDiffAbs < yDiffAbs && yDiffAbs > this.swipeThreshold) {
			if (yDiff > 0) {
				this.notifyAll(GestureController.ON_SWIPE_UP);
			} else {
				this.notifyAll(GestureController.ON_SWIPE_DOWN);
			}
		}
		this.notifyAll(GestureController.ON_TOUCH_END);
	};
}
