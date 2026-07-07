.PHONY: all check clean

CC      := i686-w64-mingw32-gcc
STRIP   := i686-w64-mingw32-strip
CFLAGS  := -O2 -Wall -Wextra -Wno-unused-parameter
LDFLAGS := -mwindows -static
LDLIBS  := -ldinput -ldxguid -lcomdlg32

TARGET := release/mvp2005padcfg.exe
SOURCE := src/main.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	mkdir -p release
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDFLAGS) $(LDLIBS)
	$(STRIP) $(TARGET)

check:
	$(CC) $(CFLAGS) -fsyntax-only $(SOURCE)

clean:
	rm -f $(TARGET)
	rmdir release 2>/dev/null || true
