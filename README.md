# mvp2005ctrlcfg

`mvp2005ctrlcfg` helps create a working controller config for the PC version of
MVP Baseball 2005.

MVP 2005 uses old DirectInput controller numbers, and those numbers do not always
match the labels on modern controllers or adapters. This is especially confusing
under Wine, where Linux may report one button order while the game sees
another. The result is usually a controller that half works: pitching may aim
correctly, but pitch selection, throws, baserunning, or right-stick actions are
wrong.

This utility asks you to press each control, reads the same DirectInput-style
inputs that MVP sees, and writes a ready-to-use `controller.cfg` profile.

## What It Does

- detects the controller name reported to DirectInput
- asks for each face button, shoulder button, D-pad direction, and stick direction
- generates a PS2-style MVP Baseball 2005 mapping
- saves a new config file without touching your existing `controller.cfg`
- works on Windows, and under Wine when run in the same prefix as the game

## How To Use It

Run `mvp2005ctrlcfg.exe`.

If you are using Wine, run it in the same Wine prefix as MVP Baseball 2005. For
example:

```sh
WINEPREFIX="$HOME/.local/share/bottles/bottles/MVP-Baseball-2005" wine mvp2005ctrlcfg.exe
```

Then:

1. Press Triangle/north on the controller you want to configure.
2. Follow the prompts.
3. Press Esc to skip a control your controller does not have.
4. Use `Retry Last` to redo one prompt, or `Start Over` to restart mapping.
5. Save the generated file.

The default filename includes the controller profile name, for example:

```text
controller.Wireless_Controller.cfg
```

To use it in MVP, back up your current config and copy or rename the generated
file to:

```text
Documents\MVP Baseball 2005\controller.cfg
```

## Default Layout

The generated profile uses a familiar console-style layout:

- X/Cross: pitch 1, swing, throw home
- Circle: pitch 2, throw first
- Square: pitch 3, throw third
- Triangle: pitch 4, throw second
- R1: fifth pitch, fake throw
- L1: switch fielder, advance all runners
- R2: relay/cutoff, retreat all runners
- D-pad: individual runner advancement
- Left stick: movement and pitch aiming
- Right stick: dive/jump/slide actions

## Build From Source

On Linux, build the Windows executable with MinGW:

```sh
make
```

The executable is written to:

```text
release/mvp2005ctrlcfg.exe
```

Requirements:

- `i686-w64-mingw32-gcc`
- `make`
