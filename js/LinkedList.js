class LinkedListItem {
	constructor(data) {
		this.prev = null;
		this.next = null;
		this.data = data;
	}
}

class LinkedList {
	constructor() {
		this.head = null;
		this.tail = null;
	}
	push_back(data) {
		const item = new LinkedListItem(data);
		item.prev = this.tail;
		if (this.tail) {
			this.tail.next = item;
		}
		this.tail = item;
		if (!this.head) {
			this.head = item;
		}
	}
	pop_front() {
		const item = this.head;
		if (!item) {
			throw new Error('List is already empty');
		}
		if (item.next) {
			item.next.prev = null;
			this.head = item.next;
		} else {
			this.head = null;
			this.tail = null;
		}
		return item.data;
	}
	empty() {
		return this.head === null;
	}
}

module.exports = LinkedList;
