CC = gcc
CFLAGS = -l wayland-client
SRC = src/*.c
OUT = bosk

all:
	$(CC) $(SRC) $(CFLAGS) -o $(OUT)

clean:
	rm -f $(OUT)
