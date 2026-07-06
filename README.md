# mvp2005ctrlcfg

Native Win32 controller configuration wizard for MVP Baseball 2005.

The tool measures the controller through old DirectInput, then generates an MVP
`controller.cfg` profile using a PS2-style layout. It does not overwrite the live
game config. Run it on Windows, or run it under the same Wine/Bottles environment
as MVP Baseball 2005.

## Requirements

- Windows or Wine
- `i686-w64-mingw32-gcc` to build from source on Linux

## Build

```sh
make
```

The Windows executable is written to:

```text
release/mvp2005ctrlcfg.exe
```

## Run

On Windows, run `mvp2005ctrlcfg.exe`.

Under Wine/Bottles, run it in the same prefix as MVP Baseball 2005. For example:

```sh
WINEPREFIX="$HOME/.local/share/bottles/bottles/MVP-Baseball-2005" wine release/mvp2005ctrlcfg.exe
```

## Workflow

1. Start the app.
2. Press X/Cross on the controller you want to configure. If multiple controllers are attached, the first controller that produces input is selected for the whole mapping session.
3. Follow each prompt and press or move the requested controller input. Press Esc to skip an input your controller does not have.
4. Review the generated config.
5. Save the generated config. The default filename includes the controller profile name, for example `controller.Wireless_Controller.cfg`.
6. Manually copy or rename it to:

```text
Documents\MVP Baseball 2005\controller.cfg
```

Back up your existing `controller.cfg` before replacing it.

Use `Retry Last` to redo the previous prompt, or `Retry All` to restart the mapping session from the beginning.

## Layout

The generated profile uses MVP's PS2-style controls:

- X: pitch 1, swing, throw home
- Circle: pitch 2, throw first
- Square: pitch 3, throw third
- Triangle: pitch 4, throw second
- R1: fifth pitch, fake throw
- L1: switch fielder, advance all
- R2: relay/cutoff, retreat all
- D-pad: runner advancement
- Left stick: player movement and pitch aiming
- Right stick: dive/jump/slide virtual directions


## Development Smoke Test

```sh
make
WINEPREFIX="$HOME/.local/share/bottles/bottles/MVP-Baseball-2005" wine release/mvp2005ctrlcfg.exe
```
