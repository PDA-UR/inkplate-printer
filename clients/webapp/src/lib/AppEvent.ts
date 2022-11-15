// ====================================================== //
// ======================== AppEvent ==================== //
// ====================================================== //

/**
 * @class AppEvent
 * @description An application internal event
 */
export class AppEvent<T> {
	public type: string;
	public data?: T;

	constructor(type: string, data?: T) {
		this.type = type;
		this.data = data;
	}
}
