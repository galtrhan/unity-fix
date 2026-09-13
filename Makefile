CC      := gcc
CFLAGS  := -shared -fPIC -ldl -Wall -Wextra
SRC     := unity-fix.c

TARGETS := unity-fix-64.so unity-fix-32.so

all: $(TARGETS)

unity-fix-64.so: $(SRC)
	$(CC) $(CFLAGS) -o $@ $<
	@echo "  -> $@"

unity-fix-32.so: $(SRC)
	@if $(CC) -m32 $(CFLAGS) -o $@ $< 2>/dev/null; then \
	    echo "  -> $@"; \
	else \
	    echo "  !! 32-bit build skipped (install gcc-multilib for 32-bit support)"; \
	    rm -f $@; \
	fi

install: all
	@echo "Nothing to install; libraries are built in place."

clean:
	rm -f unity-fix-64.so unity-fix-32.so

.PHONY: all install clean