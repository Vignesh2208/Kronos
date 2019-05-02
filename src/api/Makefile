home_dir=$(HOME)
api_dir=${home_dir}/Kronos/src/api

build_api:

	 @gcc -w -c -Wall -Werror -fpic ${api_dir}/Kronos_functions.c \
	 ${api_dir}/utility_functions.c
	 @gcc -w -shared -o libkronosapi.so ${api_dir}/Kronos_functions.o \
	 ${api_dir}/utility_functions.o
	 @mv libkronosapi.so /usr/lib
	 @chmod 0777 /usr/lib/libkronosapi.so

	 @cp ${api_dir}/Kronos_functions.h /usr/local/include
	 @cp ${api_dir}/utility_functions.h /usr/local/include

	 @python setup.py install

clean:

	@rm -rf build/
	@rm -f *.o
	@rm -f /usr/lib/libkronosapi.so > /dev/null
	@rm -f /usr/local/include/Kronos_functions.h > /dev/null
