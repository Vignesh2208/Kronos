GCC:=gcc
RM:=rm

.PHONY: build
build:
	@$(GCC) linked_list_test.c linkedlist.c -o linked_list_test
	@$(GCC) hashmap_test.c hashmap.c linkedlist.c -o hmap_test

.PHONY: clean
clean:
	@$(RM) -f linked_list_test > /dev/null
	@$(RM) -f hmap_test > /dev/null
