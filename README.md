# mvp2005padcfg

`mvp2005padcfg` creates a gamepad configuration for the PC version of
*MVP Baseball 2005*.

MVP 2005 uses old DirectInput button numbers, and those numbers do not always
match the labels on modern controllers or USB adapters. This is especially
confusing under Wine, where Linux, Wine, and the game may each report the same
controller differently.

The result is often a controller that only partly works: the left stick may aim
pitches correctly, but pitch selection, throws, baserunning, or right-stick
actions may be mapped to the wrong buttons.

This utility walks you through each control, records the DirectInput-style input
that MVP sees, and writes a new `controller.cfg` profile for the game.

## What It Does

- detects the controller name reported through DirectInput
- asks you to press each face button, shoulder button, D-pad direction, and stick
  direction
- generates a console-style MVP Baseball 2005 controller layout
- saves the generated profile as a separate file, leaving your existing
  `controller.cfg` untouched
- works on Windows and under Wine, as long as it is run in the same Wine prefix
  as the game

## How To Use It

Run:

```sh
mvp2005padcfg.exe
```

When using Wine, run it inside the same Wine prefix as MVP Baseball 2005. For
example:

```sh
WINEPREFIX="$HOME/.local/share/bottles/bottles/MVP-Baseball-2005" wine mvp2005padcfg.exe
```

Then follow the prompts:

1. Press the top face button, usually Triangle or Y, on the controller you want
   to configure.
2. Press each requested control when prompted.
3. Press Esc to skip a control your controller does not have.
4. Use `Retry Last` to redo the previous prompt, or `Start Over` to restart the
   mapping process.
5. Save the generated file.

By default, the filename includes the controller profile name, for example:

```text
controller.Wireless_Controller.cfg
```

To use the generated profile in MVP Baseball 2005, back up your existing config
and copy or rename the generated file to:

```text
Documents\MVP Baseball 2005\controller.cfg
```

## Default Layout

The generated profile uses a familiar PlayStation-style layout:

- South face button / Cross: pitch 1, swing, throw home
- East face button / Circle: pitch 2, throw first
- West face button / Square: pitch 3, throw third
- North face button / Triangle: pitch 4, throw second
- Right bumper / R1: fifth pitch, fake throw, retreat all runners
- Left bumper / L1: switch fielder, advance all runners
- Right trigger / R2: relay or cutoff throw
- D-pad: individual runner advancement
- Left stick: player movement and pitch aiming
- Right stick: dive, jump, and slide actions

## Build From Source

On Linux, build the Windows executable with MinGW:

```sh
make
```

The executable is written to:

```text
release/mvp2005padcfg.exe
```

Requirements:

- `i686-w64-mingw32-gcc`
- `make`
