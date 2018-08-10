CC 		= gcc
TARGET 	= resource_tool

HEADERS = resource_tool.h common.h
SOURCES = resource_tool.c common.c
OBJS    = $(SOURCES:.c=.o)

CFLAGS  = -fshort-wchar -m32 -ffunction-sections -Os
LDFLAGS = -Wl,--gc-sections -static

SOURCES += tests.c
HEADERS += tests.h

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f *.o $(TARGET) *.img


