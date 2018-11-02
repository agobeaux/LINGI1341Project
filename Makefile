
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

# Default target
all: clean srcmake

srcmake:
	cd src && $(MAKE) && mv receiver .. && mv sender ..

srcmake_debug :
	cd src && $(MAKE) debug && mv receiver .. && mv sender ..

debug: clean srcmake_debug

.PHONY: tests

tests:
	$(MAKE) tests -C tests

.PHONY: clean

clean:
	@rm -f sender receiver
	+$(MAKE) clean -C src
	+$(MAKE) clean -C tests
