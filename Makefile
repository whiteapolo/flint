CC := cc
CFLAGS := -Wall -Wextra -Wno-unused-result -O0
LIBS := -lreadline -lm

all:
	cc $(CFLAGS) src/main.c -o flint $(LIBS)

clean:
	rm -rf ./flint

.PHONY: all clean