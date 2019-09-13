#include "linkedlist.h"
#include <stdio.h>
#include <stdlib.h>


int main (char * argv, int argc) {
	llist test_list;
	llist_init(&test_list);
	int a [100];
	for (int i = 0; i < 100; i++) {
		a[i] = i;
		llist_append(&test_list, &a[i]);
	}
	for (int i = 0; i < 50; i++) {
		llist_append(&test_list, &a[i]);
		llist_pop(&test_list);
	}

	for (int i = 0; i < 50; i++) {
		llist_requeue(&test_list);
	}

	for (int i = 0; i < 50; i++) {
		llist_remove_at(&test_list, 0);
	}
	printf("LList size = %d", llist_size(&test_list));
	llist_destroy(&test_list);

	return 0;
}
