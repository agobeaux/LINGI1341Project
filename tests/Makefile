# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

LDFLAGS= -rdynamic
LDFLAGS += -lz
LDFLAGS += -lcunit

# Default target
all: clean tests

# If we run `make debug` instead, keep the debug symbols for gdb
# and define the DEBUG macro.
debug: CFLAGS += -g -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer
debug: clean srcmake_debug run

.PHONY: tests run

tests: srcmake run

srcmake:
	cd ../src && $(MAKE) && cp packet_implem.o ../tests && cp queue_sender.o ../tests && cp real_address.o ../tests && cp create_socket.o ../tests && cp wait_for_client.o ../tests && cp sender ../tests && cp receiver ../tests

srcmake_debug:
	cd ../src && $(MAKE) debug && cp packet_implem.o ../tests && cp queue_sender.o ../tests && cp real_address.o ../tests && cp create_socket.o ../tests && cp wait_for_client.o ../tests && cp sender ../tests && cp receiver ../tests

test : test.o packet_implem.o queue_sender.o real_address.o create_socket.o wait_for_client.o -lcunit -lz

run : test
		./test
		./test.sh

.PHONY: clean

clean:
	cd ../src && $(MAKE) clean
	@rm -f test test.o packet_implem.o queue_sender.o real_address.o create_socket.o wait_for_client.o link.log receiver.log sender.log sender receiver received_file
