const LinkedList = require('./LinkedList');

class Queue {
	constructor() {
		this._list = new LinkedList();
		this._notify = null;
		this._wait = new Promise(res => { this._notify = res; });
	}
	notify() {
		const notify = this._notify;
		this._wait = new Promise(res => {
			this._notify = res;
			notify();
		});
	}
	give(item) {
		this._list.push_back(item);
		this.notify();
	}
	async take() {
		while (this._list.empty()) {
			await this._wait;
		}
		return this._list.pop_front();
	}
}

module.exports = Queue;
