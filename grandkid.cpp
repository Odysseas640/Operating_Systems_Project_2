#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <string>
#include <sys/errno.h>
#include <sys/times.h>
#include "prime_functions.h"
using namespace std;
extern int errno;

int main(int argc, char const *argv[]) {
	int writefd, readfd;
	// cout << "Grandkid " << getpid() << " started: ";
	// for (int i = 0; i < argc; ++i) {
	// 	cout << argv[i] << " - ";
	// }
	// cout << endl;
	int function_to_use = atoi(argv[3]);

	if ( (writefd = open(argv[1], O_WRONLY | O_NONBLOCK)) < 0) {
		perror("Grandkid: Cannot open write fifo");
		exit(1);
	}
	if ( (readfd = open(argv[2], O_RDONLY)) < 0) {
		perror("Grandkid: Cannot open read fifo");
		exit(1);
	}
	
	double t1, t2;
	struct tms tb1, tb2;
	double ticspersec, time_up_to_here;
	ticspersec = (double) sysconf(_SC_CLK_TCK);
	t1 = (double) times (&tb1);
	int upper_bound = atoi(argv[5]);
	int lower_bound = atoi(argv[4]);
	int pipe_size = 2 * (upper_bound - lower_bound);
	// cout << "Grandkid pipe size: " << pipe_size << endl;
	fcntl(writefd, F_SETPIPE_SZ, pipe_size);
	if (fcntl(writefd, F_GETPIPE_SZ) < pipe_size - 10) {
		cout << "(grandkid) WARNING: Pipe size reached limit and may not be enough. Tried to set " << pipe_size << ", got " << fcntl(writefd, F_GETPIPE_SZ) << endl;
		fcntl(writefd, F_SETPIPE_SZ, 1000000);
	}
	// cout << "Grandkid pipe size actual: " << fcntl(writefd, F_GETPIPE_SZ) << endl;
	char number_to_send[30];
	char entire_message[35];
	char size_to_send[5];
	for (int n = lower_bound; n <= upper_bound; ++n) {
		if ((function_to_use == 1 && prime1(n)) || (function_to_use == 2 && prime2(n)) || (function_to_use == 3 && milrab(n))) {
			t2 = (double) times (&tb2);
			time_up_to_here = (t2 - t1) / ticspersec;
			memset(number_to_send, 0, 30);
			memset(entire_message, 0, 35);
			sprintf(number_to_send, "%d?", n);
			sprintf(number_to_send + strlen(number_to_send), "%lg?", time_up_to_here);
			sprintf(size_to_send, "%d", (int) strlen(number_to_send));
			strcpy(entire_message, size_to_send);
			strcpy(entire_message + 5, number_to_send);
			write(writefd, entire_message, 5 + strlen(number_to_send));
		}
	}
	memset(number_to_send, 0, 30);
	t2 = (double) times (&tb2);
	sprintf(number_to_send, "%lg", (t2 - t1) / ticspersec);
	strcpy(entire_message, "stop");
	strncpy(entire_message + 5, number_to_send, 30);
	write(writefd, entire_message, 35);
	int read_return = read(readfd, size_to_send, 5);
	if (read_return != 5)
		cout << "MISTAKE in grandkid: ";
	// cout << "Grandkid received: " << size_to_send << endl;

	close(readfd);
	close(writefd);
	kill(atoi(argv[6]), SIGUSR1);
	return 0;
}