import { AppEvent } from "./AppEvent";

// ====================================================== //
// ===================== Observable ===================== //
// ====================================================== //

/**
 * @class Observable
 * @description An observable object
 * @example
 * Extending `Observable` provides a class with basic pubSub functionality:
 * 1. create a class that extends `Observable`
 * 2. call `.on` on objects of that class to subscribe to events
 * 3. call `notifyAll` to publish an event to all subscribers
 */
export abstract class Observable {
	private listeners: Map<string, Listener<any>[]> = new Map();

	public on<T>(
		eventType: string,
		callback: (event: AppEvent<T>) => void
	): void {
		this.listeners.set(eventType, [
			...(this.listeners.get(eventType) || []),
			callback,
		]);
	}

	public notifyAll(eventType: string, data: any = {}): void {
		const event = new AppEvent(eventType, data);
		const listeners = this.listeners.get(eventType) || [];
		listeners.forEach((listener) => listener(event));
	}
}

type Listener<T> = (event: AppEvent<T>) => void;
