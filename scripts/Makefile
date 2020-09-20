MKDIR:=mkdir
GCC:=gcc
CHMOD:=chmod
CP:=cp
RM:=rm

.phony: build
build: 
	@$(MKDIR) -p bin
	@$(GCC) -o bin/x64_synchronizer x64_synchronizer.c
	@$(CHMOD) 777 bin/x64_synchronizer
	@$(CP) bin/x64_synchronizer /bin/
	

.phony: clean
clean:
	@$(RM) -f /bin/x64_synchronizer > /dev/null
	@$(RM) -rf bin > /dev/null
