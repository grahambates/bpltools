CC ?= gcc
PKG_CONFIG := pkg-config

CFLAGS := -Wall -Wextra -std=c11 -O2
DEPFLAGS := -MMD -MP
LIBS := libpng zlib
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(LIBS))
LDLIBS := $(shell $(PKG_CONFIG) --libs $(LIBS)) -lm

SRCS := bplopt.c bplconv.c image.c log.c
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

TARGETS := bplopt bplconv
COMMON_OBJS := image.o log.o

# Build Rules
all: $(TARGETS)

bplopt: bplopt.o $(COMMON_OBJS)
	$(CC) $^ $(LDLIBS) -o $@

bplconv: bplconv.o $(COMMON_OBJS)
	$(CC) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(DEPS) $(TARGETS)

-include $(DEPS)

.PHONY: all clean
