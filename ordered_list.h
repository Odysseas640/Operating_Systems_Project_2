#ifndef _ODYS_ORDERED_LIST_
#define _ODYS_ORDERED_LIST_
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/times.h>
using namespace std;

typedef struct ordered_list_node OrderedListNode;
struct ordered_list_node {
	int prime_number;
	double time_taken;
	OrderedListNode* Next;
};

class OrderedList {
public:
	OrderedListNode* first_node;
	OrderedList();
	int insert(int new_prime, double new_time);
	void print();
	~OrderedList();
};
#endif