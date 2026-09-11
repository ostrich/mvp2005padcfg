# mvp2005padcfg

`mvp2005padcfg` creates a gamepad profile for the PC version of MVP Baseball 2005.

The game's DirectInput mappings may not match modern controllers or USB
adapters. This can put actions on the wrong buttons and cause erratic stick
behavior, especially when fielding.

Press each button and move each stick as prompted, and the tool records the
inputs as the game sees them to create a console-style layout. It works on
Windows and under Wine, and saves a separate file so your existing config stays
untouched.

## How to use it

Download and run the `.exe` from the
[latest release](https://github.com/ostrich/mvp2005padcfg/releases/latest) or
[master build](https://github.com/ostrich/mvp2005padcfg/releases/tag/master-build).

Under Wine, use the same prefix as the game. Adjust the path and filename in
this example:

```sh
WINEPREFIX="$HOME/.local/share/bottles/bottles/MVP-Baseball-2005" wine mvp2005padcfg-VERSION.exe
```

1. Press the top face button to select your controller.
2. Follow the prompts to map each control. Press **Esc** to skip a missing
   control, **Retry Last** to redo a prompt, or **Start Over** to begin again.
3. Save the profile. The default filename includes your controller's name,
   such as `controller.Wireless_Controller.cfg`.
4. Back up your existing `controller.cfg`, then copy or rename the saved file to:

   ```text
   Documents\MVP Baseball 2005\controller.cfg
   ```

## Default layout

Common controls in the PlayStation-style layout:

| Button | Pitching | Batting/Baserunning | Fielding |
| --- | --- | --- | --- |
| South / Cross | Pitch 1; pitchout with L1 | Swing; release quickly to check swing | Throw home |
| East / Circle | Pitch 2; pickoff to first | Select runner on first | Throw to first |
| West / Square | Pitch 3; pickoff to third | Select runner on third | Throw to third |
| North / Triangle | Pitch 4; pickoff to second | Select runner on second; charge mound after being hit | Throw to second |
| Right bumper / R1 | Pitch 5 | Retreat all runners | Fake throw (after a throw button) |
| Left bumper / L1 | Hold for quick pickoffs | Advance all runners | Switch fielder |
| Left trigger / L2 | Hold for normal pickoffs | Hold + left stick: move in batter's box | — |
| Right trigger / R2 | Bullpen/dugout menu and defensive alignment | — | Relay or cutoff throw |
| D-pad | Navigate R2 menus | Choose destination base / steal | — |
| Left stick | Aim pitch | Swing direction | Move player |
| Right stick | — | Slide / return during a pickoff | Dive / jump / sliding catch |
| Left stick click / L3 | Pitch history; hit batter with L1 | — | — |
| Right stick click / R3 | Intentional walk with L1 | Bunt (hold) | — |
| Start / Menu | Pause | Pause | Pause |
| Select / Back | — | Pitch/swing analysis replay | — |

For pickoffs, hold L1 for a quick throw or L2 for a normal throw, then press
the base's face button. In pitch history, use L1/R1 to switch at-bats.

For baserunning, select a runner, then use the D-pad: right for first, up for
second, left for third, or down for home. With no runner selected, commands
apply to the lead runner.

## Build from source

On Linux, install `make` and MinGW's `i686-w64-mingw32-gcc` and
`i686-w64-mingw32-strip`, then run:

```sh
make
```

The Windows executable is saved to `release/mvp2005padcfg.exe`.

GitHub Actions also builds the executable. Successful `master` builds update the
[master build](https://github.com/ostrich/mvp2005padcfg/releases/tag/master-build);
`v*` tags publish versioned releases.
