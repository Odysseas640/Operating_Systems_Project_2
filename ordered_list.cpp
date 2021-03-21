#include "ordered_list.h"

OrderedList::OrderedList() {
	this->first_node = NULL;
}

int OrderedList::insert(int new_prime, double new_time) {
	if (this->first_node == NULL) { // Empty list
		this->first_node = new OrderedListNode;
		this->first_node->prime_number = new_prime;
		this->first_node->time_taken = new_time;
		this->first_node->Next = NULL;
		return 0;
	}
	OrderedListNode* current_node = this->first_node;
	if (current_node->prime_number > new_prime) { // Insert as first element
		OrderedListNode* new_node = new OrderedListNode;
		new_node->prime_number = new_prime;
		new_node->time_taken = new_time;
		new_node->Next = current_node;
		this->first_node = new_node;
		return 0;
	}
	while (current_node->Next != NULL && current_node->Next->prime_number < new_prime) {
		current_node = current_node->Next;
	}
	OrderedListNode* new_node = new OrderedListNode;
	new_node->prime_number = new_prime;
	new_node->time_taken = new_time;
	new_node->Next = current_node->Next;
	current_node->Next = new_node;
	return 0;
}
void OrderedList::print() {
	cout << "List print" << endl;
	int list_size = 0;
	OrderedListNode* current_node = this->first_node;
	while (current_node != NULL) {
		cout << current_node->prime_number << " - " << current_node->time_taken << endl;
		current_node = current_node->Next;
		list_size++;
	}
	cout << "List print end, counted " << list_size << " nodes" << endl;
}
OrderedList::~OrderedList() {
	OrderedListNode* current_node = this->first_node;
	while (current_node != NULL) {
		OrderedListNode* to_delete = current_node;
		current_node = current_node->Next;
		delete to_delete;
	}
}