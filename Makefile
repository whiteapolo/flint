CC := cc
CFLAGS := -Wall -Wextra -Wno-unused-result -g -O0 '-DPROMPT="::dev:: "'
RELEASE_CFLAGS = -O3 -Wno-unused-result '-DPROMPT="::white:: "'
LIBS := -lreadline -lm

all:
	cc $(CFLAGS) src/main.c -o flint $(LIBS)

release:
	cc $(RELEASE_CFLAGS) src/main.c -o flint $(LIBS)

clean:
	rm -rf ./flint

.PHONY: all release clean
