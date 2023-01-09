CC ?= cc
RM ?= rm -f

CFLAGS ?= -O2 -Wall -Wextra -Wformat=2 -Wp,-MT,$@ -Wp,-MD,$(dir $@).$(notdir $@).d -fstack-protector
CFLAGS += -DPRINTF=printf
CFLAGS += -DUNIT_TESTS
CFLAGS += -I../../../src

LDFLAGS ?= -Wl,-O1,-as-needed,-no-undefined,-z,relro,-z,now,--fatal-warnings -fstack-protector

# Use Address Sanitizer (ASAN) and Undefined Behavior Sanitizer (UBSAN)
CFLAGS += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined

test: qatozil
	./test.sh

qatozil: main.o qatozil.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	$(RM) qatozil ./*.o .*.d

qatozil.o: ../../../src/qatozil.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: clean test

.PHONY: clean test
