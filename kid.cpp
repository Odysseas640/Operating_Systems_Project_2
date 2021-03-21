#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <sys/errno.h>
#include "ordered_list.h"
using namespace std;
extern int errno;

int main(int argc, char const *argv[]) {
	// cout << "Kid " << getpid() << " started\n";
	// for (int i = 0; i < argc; ++i) {
	// 	cout << i << " - " << argv[i] << endl;
	// }
	int numOfChildren = atoi(argv[3]);
	int lower_bound = atoi(argv[4]);
	int upper_bound = atoi(argv[5]);
	if (numOfChildren > upper_bound - lower_bound)
		numOfChildren = upper_bound - lower_bound;

	int writefd, readfd;
	if ( (writefd = open(argv[1], O_WRONLY | O_NONBLOCK)) < 0) {
		perror("Kid: Cannot open write fifo");
		exit(1);
	}
	if ( (readfd = open(argv[2], O_RDONLY)) < 0) {
		perror("Kid: Cannot open read fifo");
		exit(1);
	}

	// Create numOfChildren children and pass them their lower and upper bound, PPID (root's PID), and algorithm to use
	int* kids_pid_array = new int[numOfChildren];
	int* readfd1;
	int* writefd1;
	char** fifo1;
	char** fifo2;
	char temp10[10];
	readfd1 = new int[numOfChildren];
	writefd1 = new int[numOfChildren];
	fifo1 = new char*[numOfChildren];
	fifo2 = new char*[numOfChildren];
	for (int i = 0; i < numOfChildren; ++i) {
		fifo1[i] = new char[15];
		fifo2[i] = new char[15];
	}
	for (int i = 0; i < numOfChildren; i++) {
		strcpy(fifo1[i], "fifo_");
		sprintf(temp10, "%d", getpid()); // FIFO between "PID" kid...
		strcat(fifo1[i], temp10);
		sprintf(temp10, ".%d", 2*i); // ...and grandkid number "i"
		strcat(fifo1[i], temp10);
		strcpy(fifo2[i], "fifo_");
		sprintf(temp10, "%d", getpid());
		strcat(fifo2[i], temp10);
		sprintf(temp10, ".%d", 2*i+1);
		strcat(fifo2[i], temp10);

		if (mkfifo(fifo1[i], 0666) < 0 && (errno != EEXIST)) {
			perror("Cannot create FIFO");
			exit(2);
		}
		if (mkfifo(fifo2[i], 0666) < 0 && (errno != EEXIST)) {
			unlink(fifo2[i]);
			perror("Cannot create FIFO");
			exit(2);
		}
	}

	int this_kid_index = atoi(argv[6]);
	char function_to_use_char[10];
	char numOfChildren_char[10];
	sprintf(numOfChildren_char, "%d", numOfChildren);
	char root_pid_char[10];
	sprintf(root_pid_char, "%d", getppid());
	int numbers_for_each_kid = (atoi(argv[5]) - atoi(argv[4])) / numOfChildren;
	for (int i = 0; i < numOfChildren; ++i) {
		int fork_return = fork();
		if (fork_return == 0) { // Only the KID should exec this, not the parent.
			// Decide which function this grandkid should use
			char upper_bound_for_this_kid[20], lower_bound_for_this_kid[20];
			if (i == 0) {
				sprintf(lower_bound_for_this_kid, "%d", atoi(argv[4]));
				sprintf(upper_bound_for_this_kid, "%d", atoi(argv[4]) + numbers_for_each_kid);
			}
			else if (i == numOfChildren - 1) {
				sprintf(lower_bound_for_this_kid, "%d", atoi(argv[4]) + i * numbers_for_each_kid + 1);
				sprintf(upper_bound_for_this_kid, "%d", atoi(argv[5]));
			}
			else {
				sprintf(lower_bound_for_this_kid, "%d", atoi(argv[4]) + i * numbers_for_each_kid + 1);
				sprintf(upper_bound_for_this_kid, "%d", atoi(argv[4]) + (i+1) * numbers_for_each_kid);
			}
			sprintf(function_to_use_char, "%d", ((this_kid_index) * numOfChildren + i) % 3 + 1);
			// cout << "SENDING TO GRANDKID: " << lower_bound_for_this_kid << " == " << upper_bound_for_this_kid << endl;
			// cout << " = Kid " << getppid() << " creating kid with algorithm " << function_to_use_char << endl;
			execl("./grandkid", "./grandkid", fifo1[i], fifo2[i], function_to_use_char, lower_bound_for_this_kid, upper_bound_for_this_kid, root_pid_char, NULL);
		}
		if ((readfd1[i] = open(fifo1[i], O_RDONLY | O_NONBLOCK)) < 0) {
			perror("Kid: cannot open read FIFO");
			exit(1);
		}
		if ((writefd1[i] = open(fifo2[i], O_WRONLY)) < 0) {
			perror("Kid: cannot open write FIFO");
			exit(1);
		}
		kids_pid_array[i] = fork_return;
	}
	for (int i = 0; i < numOfChildren; ++i) {
		fcntl(writefd1[i], F_SETPIPE_SZ, 100); // This is to save space
		// cout << fcntl(writefd1[i], F_GETPIPE_SZ) << endl;
	}

	int quitt = 0;
	int retourn;
	int total_primes = 0;
	char size_to_receive[5];
	char actual_message[30];
	float total_grandkid_times[numOfChildren];
	OrderedList primes_list;
	int pipe_size = 2 * (upper_bound - lower_bound); // Set pipe to just enough to comfortably send primes. I decided on this size in my testing.
	// cout << "Kid pipe size set: " << pipe_size << endl;
	fcntl(writefd, F_SETPIPE_SZ, pipe_size);
	if (fcntl(writefd, F_GETPIPE_SZ) < pipe_size) {
		cout << "(kid) WARNING: Pipe size reached limit and may not be enough. Tried to set " << pipe_size << ", got " << fcntl(writefd, F_GETPIPE_SZ) << endl;
		fcntl(writefd, F_SETPIPE_SZ, 1000000); // Max pipe size is (probably) 1048576
	}
	// cout << "Kid pipe size actual: " << fcntl(writefd, F_GETPIPE_SZ) << endl;
	int kid_fds_done[numOfChildren];
	for (int i = 0; i < numOfChildren; ++i) {
		kid_fds_done[i] = 0;
		total_grandkid_times[i] = 54.0; // Arbitrary number to spot mistakes easier
	}
	int n_fds;
	fd_set kid_fds;
	do {
		FD_ZERO(&kid_fds);
		for (int i = 0; i < numOfChildren; ++i) {
			if (kid_fds_done[i] == 0)
				FD_SET(readfd1[i], &kid_fds);
		}
		n_fds = 0;
		for (int i = 0; i < numOfChildren; ++i) {
			if (n_fds < readfd1[i] && kid_fds_done[i] == 0)
				n_fds = readfd1[i];
		}
		n_fds++;
		struct timeval tv1;
		tv1.tv_sec = 9999;
		tv1.tv_usec = 0;
		retourn = select(n_fds, &kid_fds, NULL, NULL, &tv1);
		if (retourn <= 0)
			continue;
		for (int i = 0; i < numOfChildren; ++i) {
			if (kid_fds_done[i] == 0 && FD_ISSET(readfd1[i], &kid_fds)) { // This grandkid has sent something, read it and put it in the ordered list
				memset(size_to_receive, 0, 5);
				memset(actual_message, 0, 30);
				int prime_received;
				float time_taken;
				read(readfd1[i], size_to_receive, 5);
				if (size_to_receive[0] < '0' || size_to_receive[0] > '9') { // This grandkid has finished, remove it from the list of FDs
					// cout << "Kid received STOP" << endl;
					read(readfd1[i], actual_message, 30);
					total_grandkid_times[i] = atof(actual_message);
					write(writefd1[i], "ST1P", 5);
					quitt = 1;
					kid_fds_done[i] = 1;
					for (int j = 0; j < numOfChildren; ++j) {
						if (kid_fds_done[j] == 0)
							quitt = 0;
					}
				}
				else {
					read(readfd1[i], actual_message, atoi(size_to_receive));
					char* saveptr = actual_message;
					prime_received = atoi(strtok_r(saveptr, "?", &saveptr));
					time_taken = atof(strtok_r(saveptr, "?", &saveptr));
					primes_list.insert(prime_received, time_taken);
					total_primes++;
				}
			}
		}
	} while (quitt == 0); // Done receiving primes from grandkids
	// Now go through the list and send every prime/time to the root process
	// primes_list.print();
	char number_to_send[30];
	char entire_message[35];
	char size_to_send[5];
	float shortest_time = total_grandkid_times[0], longest_time = total_grandkid_times[0];
	for (int i = 0; i < numOfChildren; ++i) {
		if (total_grandkid_times[i] > longest_time)
			longest_time = total_grandkid_times[i];
		if (total_grandkid_times[i] < shortest_time)
			shortest_time = total_grandkid_times[i];
	}
	OrderedListNode* current_node = primes_list.first_node;
	while (current_node != NULL) {
		memset(number_to_send, 0, 50);
		sprintf(number_to_send, "%d?", current_node->prime_number);
		sprintf(number_to_send + strlen(number_to_send), "%lg?", current_node->time_taken);
		sprintf(size_to_send, "%d", (int) strlen(number_to_send));
		strcpy(entire_message, size_to_send);
		strcpy(entire_message + 5, number_to_send);
		write(writefd, entire_message, 5 + strlen(number_to_send));
		current_node = current_node->Next;
	}
	memset(number_to_send, 0, 30);
	sprintf(number_to_send, "%lg?", shortest_time);
	sprintf(number_to_send + strlen(number_to_send), "%lg?", longest_time);
	strcpy(entire_message, "stop");
	strncpy(entire_message + 5, number_to_send, 30);
	write(writefd, entire_message, 35); // stop/0<SHORTEST_TIME>?<LONGEST_TIME>?
	int read_return = read(readfd, size_to_send, 5);
	if (read_return != 5)
		cout << "MISTAKE in kid: ";
	// cout << "Kid received: " << size_to_send << endl;

	close(readfd);
	close(writefd);
	int exit_code;
	for (int i = 0; i < numOfChildren; ++i) {
		close(readfd1[i]);
		close(writefd1[i]);
		waitpid(kids_pid_array[i], &exit_code, 0);
	}
	delete[] kids_pid_array;
	delete[] writefd1;
	delete[] readfd1;
	for (int i = 0; i < numOfChildren; ++i) {
		unlink(fifo1[i]);
		unlink(fifo2[i]);
		delete[] fifo1[i];
		delete[] fifo2[i];
	}
	delete[] fifo1;
	delete[] fifo2;
	return 0;
}