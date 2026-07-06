.PHONY: all check clean run

CC      := i686-w64-mingw32-gcc
CFLAGS  := -Wall -Wextra -Wno-unused-parameter
LDFLAGS := -mwindows -static
LDLIBS  := -ldinput -ldxguid -lcomdlg32

TARGET := release/mvp2005ctrlcfg.exe
SOURCE := src/main.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	mkdir -p release
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

check:
	$(CC) $(CFLAGS) -fsyntax-only $(SOURCE)

clean:
	rm -f $(TARGET)
	rmdir release 2>/dev/null || true

run: $(TARGET)
	wine $(TARGET)
