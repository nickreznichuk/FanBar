# FanBar

A tiny macOS menu bar app for Apple Silicon MacBooks that shows the hottest SoC die temperature and the current fan speed.

- Menu bar: `<temp>° <rpm>` (`off` when the fans are stopped). Orange from 80°C or "fair" thermal state, red from 90°C or "serious"/"critical".
- Click for details: each fan's actual/target RPM and range, exact temperature, macOS thermal state.
- Read-only: reads AppleSMC fan keys and IOHID temperature sensors, never controls the fans. No root needed.

## Build

```sh
./build.sh
open FanBar.app
```

Requires Xcode command line tools. The app is ad-hoc signed.
