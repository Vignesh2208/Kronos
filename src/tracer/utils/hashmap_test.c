#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>


int main (char * argv, int argc) {
	hashmap test;
	hmap_init(&test, 1000);
	int a [100];
	for (int i = 0; i < 100; i++) {
		a[i] = i;
		hmap_put_abs(&test, i, &a[i]);
	}
	for (int i = 0; i < 50; i++) {
		hmap_put_abs(&test, i, &a[i]);
		hmap_remove_abs(&test, i);
	}
	for (int i = 50; i < 100; i++) {
		printf("Hmap get = %d\n", *(int *) hmap_get_abs(&test, i));
	}

	for (int i = 0; i < 50; i++) {
		if (hmap_get_abs(&test, i)) 
			return 0;
	}
	
	hmap_destroy(&test);

	return 0;
}
