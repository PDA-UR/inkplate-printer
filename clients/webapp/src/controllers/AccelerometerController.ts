import { Observable } from "../lib/Observable";

export default class AccelerometerController extends Observable {
	public static readonly ON_PAIR_LEFT = "pairLeft";
	public static readonly ON_PAIR_RIGHT = "pairRight";
	public static readonly ON_UNPAIR = "unpair";

	constructor() {
		super();
		document.addEventListener("keydown", this.onKeyDown);
	}

	private onKeyDown = (event: KeyboardEvent) => {
		if (event.key === "1") {
			this.notifyAll(AccelerometerController.ON_PAIR_RIGHT);
		} else if (event.key === "2") {
			this.notifyAll(AccelerometerController.ON_PAIR_LEFT);
		} else if (event.key === "3") {
			this.notifyAll(AccelerometerController.ON_UNPAIR);
		}
	};
}
