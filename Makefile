.PHONY: all release run clean

MINGW_CC ?= i686-w64-mingw32-gcc

EXE := release/mvp2005ctrlcfg.exe
SRC := src/main.c

all: $(EXE)

release: $(EXE)

$(EXE): $(SRC)
	mkdir -p release
	$(MINGW_CC) $(SRC) -o $(EXE) -mwindows -static -ldinput -ldxguid -lcomdlg32

run: $(EXE)
	wine $(EXE)

clean:
	rm -f $(EXE)
