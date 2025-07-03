CC ?= gcc
SRC = bosk.c xdg-shell-protocol.c
HEADERS = xdg-shell-client-protocol.h
OUT = bosk

CFLAGS ?= 
PKG_CONFIG ?= pkg-config

WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)
WAYLAND_SCANNER = $(shell pkg-config --variable=wayland_scanner wayland-scanner)
XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml

all: $(OUT)

$(OUT): $(HEADERS) $(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $@ -lrt $(WAYLAND_FLAGS)
	
xdg-shell-client-protocol.h:
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) $@

xdg-shell-protocol.c:
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) $@

.PHONY: clean
clean:
	rm -f $(OUT)
