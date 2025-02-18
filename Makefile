CFLAGS := -Wall -Wextra -std=c11
PKG_CONFIG := pkg-config

LIBS := libpng zlib
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(LIBS))
LDFLAGS := $(shell $(PKG_CONFIG) --libs $(LIBS))
OBJS := $(SRCS:.c=.o)

all: optimise convert

optimise: optimise.o image.o log.o
	$(CC) $^ $(LDFLAGS) -o $@

convert: convert.o image.o log.o
	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) optimise comvert
