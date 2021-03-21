#include <iostream>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <signal.h>
#include "ordered_list.h"
using namespace std;
extern int errno;
int sigusr1_received;

void sig_handler(int signo) {
	if (signo == SIGUSR1) {
		signal(SIGUSR1, sig_handler);
		// cout << "Root received SIGUSR1" << endl;
		sigusr1_received++;
	}
}

int main(int argc, char const *argv[]) {
	int* kids_pid_array;
	int* readfd;
	int* writefd;
	char** fifo1;
	char** fifo2;
	sigusr1_received = 0;
	signal(SIGUSR1, sig_handler);
	int numOfChildren = -1, lower_bound = -1, upper_bound = -1;
	char* bufferSize = new char[5];
	strcpy(bufferSize, "100");
	for (int i = 1; i < argc-1; i=i+2) {
		if (strcmp(argv[i],"-l") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "Number of workers is not a number. Terminating." << endl;
					return 1;
				}
			}
			lower_bound = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else if (strcmp(argv[i],"-u") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "Number of workers is not a number. Terminating." << endl;
					return 1;
				}
			}
			upper_bound = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else if (strcmp(argv[i],"-w") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "Number of workers is not a number. Terminating." << endl;
					return 1;
				}
			}
			numOfChildren = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else
			cout << "MISTAKE" << endl;
	}
	// Done finding arguments
	if (numOfChildren <= 0 || lower_bound <= 0 || upper_bound <= 0) {
		cout << "Could not find expected arguments. Terminating." << endl;
		return 1;
	}
	if (numOfChildren > upper_bound - lower_bound)
		numOfChildren = upper_bound - lower_bound;

	char temp10[10];
	readfd = new int[numOfChildren];
	writefd = new int[numOfChildren];
	fifo1 = new char*[numOfChildren];
	fifo2 = new char*[numOfChildren];
	for (int i = 0; i < numOfChildren; ++i) {
		fifo1[i] = new char[15];
		fifo2[i] = new char[15];
	}
	for (int i = 0; i < numOfChildren; i++) {
		strcpy(fifo1[i], "fifo.");
		sprintf(temp10, "%d", 2*i);
		strcat(fifo1[i], temp10);
		strcpy(fifo2[i], "fifo.");
		sprintf(temp10, "%d", 2*i+1);
		strcat(fifo2[i], temp10);

		if (mkfifo(fifo1[i], 0666) < 0 && (errno != EEXIST)) {
			perror("Cannot create FIFO");
		}
		if (mkfifo(fifo2[i], 0666) < 0 && (errno != EEXIST)) {
			unlink(fifo2[i]);
			perror("Cannot create FIFO");
		}
	}
	// Done creating pipes

	kids_pid_array = new int[numOfChildren];
	char numOfChildren_char[10];
	char i_char[10];
	sprintf(numOfChildren_char, "%d", numOfChildren);
	int numbers_for_each_kid = (upper_bound - lower_bound) / numOfChildren;
	for (int i = 0; i < numOfChildren; ++i) {
		int fork_return = fork();
		if (fork_return == 0) { // Decide range of numbers for this kid
			char upper_bound_for_this_kid[20], lower_bound_for_this_kid[20];
			if (i == 0) {
				sprintf(lower_bound_for_this_kid, "%d", lower_bound);
				sprintf(upper_bound_for_this_kid, "%d", lower_bound + numbers_for_each_kid);
			}
			else if (i == numOfChildren - 1) {
				sprintf(lower_bound_for_this_kid, "%d", lower_bound + i * numbers_for_each_kid + 1);
				sprintf(upper_bound_for_this_kid, "%d", upper_bound);
			}
			else {
				sprintf(lower_bound_for_this_kid, "%d", lower_bound + i * numbers_for_each_kid + 1);
				sprintf(upper_bound_for_this_kid, "%d", lower_bound + (i+1) * numbers_for_each_kid);
			}
			sprintf(i_char, "%d", i); // For deciding which prime algorithm to use
			execl("./kid", "./kid", fifo1[i], fifo2[i], numOfChildren_char, lower_bound_for_this_kid, upper_bound_for_this_kid, i_char, NULL); // For some reason, valgrind replaces the first argument
			// ...with the executable name, while, without valgrind, worker's argv[0] is the first argument
		}
		if ((readfd[i] = open(fifo1[i], O_RDONLY | O_NONBLOCK)) < 0) {
			perror("Parent: cannot open read FIFO");
			exit(1);
		}
		if ((writefd[i] = open(fifo2[i], O_WRONLY)) < 0) {
			perror("Parent: cannot open write FIFO");
		}
		kids_pid_array[i] = fork_return;
	}
	// Done creating kids
	
	int quitt = 0;
	int retourn;
	int total_primes = 0;
	char size_to_receive[5];
	char actual_message[30];
	OrderedList primes_list;
	float shortest_grandkid_time = 123456.0;
	float longest_grandkid_time = 0.0;
	int kid_fds_done[numOfChildren];
	for (int i = 0; i < numOfChildren; ++i) {
		kid_fds_done[i] = 0;
	}
	int n_fds;
	fd_set kid_fds;
	do {
		FD_ZERO(&kid_fds);
		for (int i = 0; i < numOfChildren; ++i) {
			if (kid_fds_done[i] == 0)
				FD_SET(readfd[i], &kid_fds);
		}
		n_fds = 0;
		for (int i = 0; i < numOfChildren; ++i) {
			if (n_fds < readfd[i] && kid_fds_done[i] == 0)
				n_fds = readfd[i];
		}
		n_fds++;
		struct timeval tv1;
		tv1.tv_sec = 9999;
		tv1.tv_usec = 0;
		retourn = select(n_fds, &kid_fds, NULL, NULL, &tv1);
		if (retourn <= 0)
			continue;
		for (int i = 0; i < numOfChildren; ++i) {
			if (kid_fds_done[i] == 0 && FD_ISSET(readfd[i], &kid_fds)) { // This grandkid has sent something, read it and put it in the ordered list
				memset(size_to_receive, 0, 5);
				memset(actual_message, 0, 30);
				int prime_received;
				float time_taken;
				read(readfd[i], size_to_receive, 5); // Read size of char array that contains the prime number and the time taken to find it.
				if (size_to_receive[0] < '0' || size_to_receive[0] > '9') { // This kid has finished, don't add it to the FD set again.
					// cout << "Root received STOP" << endl;
					read(readfd[i], actual_message, 30); // shortest_time?longest_time?
					char* saveptr = actual_message;
					time_taken = atof(strtok_r(saveptr, "?", &saveptr));
					if (time_taken < shortest_grandkid_time)
						shortest_grandkid_time = time_taken;
					time_taken = atof(strtok_r(saveptr, "?", &saveptr));
					if (time_taken > longest_grandkid_time)
						longest_grandkid_time = time_taken;
					write(writefd[i], "ST1P", 5);
					quitt = 1;
					kid_fds_done[i] = 1;
					for (int j = 0; j < numOfChildren; ++j) { // If there's a kid that has not quit, don't quit.
						if (kid_fds_done[j] == 0)
							quitt = 0;
					}
				}
				else {
					read(readfd[i], actual_message, atoi(size_to_receive));
					char* saveptr = actual_message;
					prime_received = atoi(strtok_r(saveptr, "?", &saveptr));
					time_taken = atof(strtok_r(saveptr, "?", &saveptr));
					primes_list.insert(prime_received, time_taken);
					total_primes++;
				}
			}
		}
	} while (quitt == 0);
	cout << "Root printing primes and times: " << endl;
	primes_list.print();
	cout << "Root total primes: " << total_primes << endl;
	cout << "Shortest time taken by a leaf process (grandkid): " << shortest_grandkid_time << endl;
	cout << "Longest time taken by a leaf process (grandkid): " << longest_grandkid_time << endl;
	cout << "Root received " << sigusr1_received << " signals" << endl;

	int exit_code;
	for (int i = 0; i < numOfChildren; ++i) {
		close(readfd[i]);
		close(writefd[i]);
		waitpid(kids_pid_array[i], &exit_code, 0);
	}
	delete[] kids_pid_array;
	delete[] writefd;
	delete[] readfd;
	for (int i = 0; i < numOfChildren; ++i) {
		unlink(fifo1[i]);
		unlink(fifo2[i]);
		delete[] fifo1[i];
		delete[] fifo2[i];
	}
	delete[] fifo1;
	delete[] fifo2;
	delete[] bufferSize;
	return 0;
}