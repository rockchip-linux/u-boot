CC 		= gcc
TARGET 	= resource_tool

HEADERS = resource_tool.h common.h
SOURCES = resource_tool.c common.c
OBJS    = $(SOURCES:.c=.o)

CFLAGS  = -fshort-wchar -m32 -static

TEST_LOAD = 1

ifdef TEST_LOAD
	SOURCES += tests.c
	CFLAGS += -DTEST_LOAD
endif

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f *.o $(TARGET) *.img


