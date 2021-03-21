#include "prime_functions.h"

int prime1(int n) {
	int i;
	if (n == 1)
		return NO;
	for (i = 2; i < n; i++)
		if (n % i == 0)
			return NO;
	return YES;
}
int prime2(int n) {
	int i=0, limitup=0;
	limitup = (int) (sqrt( (float) n));
	if (n == 1)
		return NO;
	for (i = 2; i <= limitup; i++)
		if (n % i == 0)
			return NO;
	return YES;
}
unsigned int modular_pow(unsigned long long base, unsigned int exponent, unsigned int modulus) { // This is from the Wikipedia article mentioned in the Miller-Rabin assignment
	unsigned int result=1;
	base=base%modulus;
	while (exponent>0) {
		if (exponent%2==1)
			result=(result*base)%modulus;
		exponent=exponent>>1;
		base=(base*base)%modulus;
	}
	return(result);
}
int milrab(int n) { // Miller-Rabin function
	if (n == 1 || n == 2 || n == 3 || n == 5 || n == 7 || n == 61)
		return 1; // Doesn't work for these numbers
	int r, d, x=0,a, a_times, j, nexta=0;
	r=0;
	d=n-1;
	while (d%2==0) { // Write n-1 as 2^r*d
		d=d/2;
		r++;
	}
	for (a_times=1; a_times<=3; a_times++) {
		if (a_times==1) a=2;       // First iteration: a=2
		else if (a_times==2) a=7;  // Second: a=7
		else a=61;                 // Third: a=61
		x=modular_pow(a,d,n);
		if (x==1 || x==n-1) continue;
		for (j=0; j<=r-1; j++) {
			x=((unsigned long long) x * (unsigned long long) x)%n;
			if (x==n-1) {
				nexta=1; // Continue with next a
				break;
			}
		}
		if (nexta==1) { nexta=0; continue; } // Continue with next a
		return(0); // Return "not a prime"
	}
	return(1); // Return "prime"
}