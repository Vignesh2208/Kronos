API_DIR=$(HOME)/Kronos/src/api
GCC:=gcc
RM:=rm
PYTHON:=python
PYTHON2:=python2
CHMOD:=chmod
CP:=cp
MV:=mv

.PHONY: build_api
build_api:

	 @$(GCC) -w -c -Wall -Werror -fpic ${API_DIR}/Kronos_functions.c \
	 ${API_DIR}/kronos_utility_functions.c
	 @$(GCC) -w -shared -o libkronosapi.so ${API_DIR}/Kronos_functions.o \
	 ${API_DIR}/kronos_utility_functions.o
	 @$(MV) libkronosapi.so /usr/lib
	 @$(CHMOD) 0777 /usr/lib/libkronosapi.so

	 @$(CP) ${API_DIR}/Kronos_functions.h /usr/local/include
	 @$(CP) ${API_DIR}/kronos_utility_functions.h /usr/local/include
	 @$(PYTHON2) setup.py install
	 @$(PYTHON) setup.py install

.PHONY: clean
clean:

	@$(RM) -rf build/
	@$(RM) -f *.o
	@$(RM) -f /usr/lib/libkronosapi.so > /dev/null
	@$(RM) -f /usr/local/include/Kronos_functions.h > /dev/null
	@$(RM) -f /usr/local/include/kronos_utiliy_functions.h > /dev/null
