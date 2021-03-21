Odysseas Stefas, 1700151

Usage example:
make
./myprime -l 6 -u 40 -w 1

I refer to the root process as "root", the in-between processes as "kids" and the leaf processes as "grandkids".

The root process, "myprime", begins by finding the arguments. If they're not correct, it quits.
Then it creates 2 named pipes for every child. One non-blocking for the child to send results, and one blocking, which is only used when a child quits (more on that later).
The root process splits the search space (almost) evenly, creates kids, and passes that space, the name of the 2 pipes and the number of kids, as arguments to the kids. The kids also get the "i" from the kid-creating loop as an argument, which is used to decide what algorithm the grandkids (leaf processes) will use.

The kids read their arguments, open their 2 pipes, and then split their respective search space evenly, create grandkids and pass them their arguments, in much the same way as the root process. The algorithm that every grandkid uses is decided in the kid, and is passed as an argument (1, 2 or 3). The grandkids also get the PID of the root, so they can send it SIGUSR1 before they quit.

The grandkids (leaf processes) read their arguments and open their 2 pipes. Then they call the specified function (1, 2 or 3) to decide which numbers in their search space are primes. When they find a prime, they send it to the kid (their parent) through a non-blocking pipe.
For every prime found, a char array of variable size is written to the non-blocking pipe. It has the form <LENGTH><PRIME_NUMBER>?<TIME_UP_TO_HERE>?
The first part is <LENGTH>. That part is always 5 bytes, and contains the length of the string from the 6th byte to the end.
The second part of the string starts at the 6th byte. The prime number starts at the 6th byte and ends at the "?", an escape character. After the escape character is the time taken up to here, and then the message ends with another "?" escape character.
The kid reads this message from the grandkid in 2 parts, 2 read() calls. The first read is exactly 5 bytes. These 5 bytes contain the size of the rest of the message, or "stop", if the grandkid has finished. If it's not "stop", the kid readsthe number, and knows exactly how many bytes to read to get the second part of the message. Then it uses strtok_r with "?" as the delimiter to get the number and time taken, and it puts them in a sorted list that contains all the primes and times from this kid's grandkids.

When a grandkid has finished its search space, it sends stop\0<TIME>\0 (in a single string, as described above) through the non-blocking pipe, where <TIME> is how long this grandkid took to go through all of its numbers. It's usually equal to the time it took to find its last prime. Then, the grandkid waits to read from a blocking pipe until the kid (its parent) has read the stop message from the non-blocking pipe. The kid sends a confirmation message through the blocking pipe, the grandkid reads it and then quits.
I tried to have the grandkid close its non-blocking pipe and quit right after it's done finding primes, and then the kid would read from the non-blocking pipe when it got a chance. Then I wouldn't need a second, blocking pipe, I could just have one non-blocking pipe for the grandkid to send its primes. But after the grandkid had closed its end of the pipe and quit, the kid couldn't read anything from the pipe. To get round this problem, I made this second, blocking pipe for the kid to tell the grandkid "I read all the primes, you can quit now". The assignment says to use non-blocking pipes, but I think this doesn't block anything important. The grandkid has sent all of its primes and the time it took to finish, and then it waits at the pipe. The kid never really blocks at the blocking pipe. By the time the kid gets to the blocking pipe, the grandkid is already waiting there, so the kid immediately writes "OK" and moves on. I also tried to make the second pipe non-blocking and put a select() in the grandkid to wait for a confirmation message in the end, even though, effectively, I'd still get a blocking behavior. But I was having problems with opening the second non-blocking pipe, so I gave up on that.

Anyway, after the kid has started all its grandkids, it puts their file descriptor in a select() set and waits for them. I have an array that stores which grandkids are still running, and when a grandkid finishes and sends a STOP message, I mark it as finished so its file descriptor doesn't get in the select() set again.
The kid also stores which ones of its grandkids were the fastest and slowest.

After all the grandkids are finished, the kid goes through its list of primes and sends them all to the root, in the same manner as the grandkid sends to the kid, with a message in the form <LENGTH><PRIME_NUMBER>?<TIME>?. When it has send all its primes to the root, the kid sends a STOP message and waits to read a confirmation to quit from the root, at a blocking pipe. The kid also sends to the root the shortest and longest time taken by a grandkid to finish, in the STOP message.
The root also has a select() and reads from the kids their prime numbers and puts them in a sorted list, in much the same way as the kid reads from the grandkids.

The root-kid communication is pretty much the same as the kid-grandkid communication.
Then the root prints its sorted primes list, how many they are, and the shortest and longest time taken by a grandkid to finish.

The third prime finding algorithm is from Introduction to Programming, where we had to find primes with the Miller-Rabin algorithm.

in the root process, if the search space is not enough for every kid to get at least one number to check if it's a prime, fewer kids are created, each with one number. Same happens in the kid when creating grandkids.

In my testing, the program didn't work in too extreme cases. The main limitation, especially with a large search space and few children, is that the pipes fill up and some results get lost. I tried making the pipes larger, but it didn't consistently make a difference.
In my testing, when a kid or grandkid has a search space of more than about 600.000 to 700.000 numbers, the pipe fills up and the program stalls. I did some testing and made the kid and grandkid dynamically adjust their pipe size according to how large their search space is. If the max pipe size of 1.048.576 is reached, I set it to that and I print a warning message.
Also, when there's too many processes with too big pipes, apparently there's not enough space and the pipes shrink to 4KB, which may be enough for the grandkids, but not nearly enough for the kids.
Here's some examples:
1 to 500.000 with 1 kids: works fine.
1 to 5.000.000 with 10 kids: works fine.
1 to 100.000 with 40 kids works fine.
