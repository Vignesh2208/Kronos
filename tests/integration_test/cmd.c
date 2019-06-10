#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <unistd.h>


unsigned long fibonacci(unsigned long curr, unsigned long prev) {

	return prev + curr;
}

int main() {
	int i = 1;
	unsigned long prev = 0, curr = 1;
	int delay = 1000;
	unsigned long ret = 1;
	printf("Starting Fibonacci computation !\n");
	fflush(stdout);
	while(i < 500000000) {		
		ret = fibonacci(curr, prev);
		prev = curr;
		curr = ret;
		i++;
	}
	printf("Finished Fibonacci computation !\n");
	fflush(stdout);
	return 0;
}
