# 3DP LCD

A Pebble watchface that looks like a classic blue-and-white 3D printer
controller LCD (Marlin / RepRap status screen).

- **Nozzle** temperature = hour
- **Bed** temperature = minute
- **Progress bar** = how much of the day has passed
- **Fan** icon spins in the corner

## Platforms

Pebble Time 2 (Emery), Pebble Time (Basalt), Pebble 2 / Time Round-class
(Diorite), and Pebble 2 Duo (Flint). Color watches use Oxford/Duke blue with
white text; B&W watches use white on black.

## Build and install

The Pebble SDK runs on Linux and macOS only. On **Windows**, `build.ps1` runs it
in a container (install [Podman Desktop](https://podman-desktop.io) or Docker
Desktop first):

```powershell
./build.ps1
```

The first run builds the container image and downloads the SDK. The watchface
ends up in `build/3DP-LCD.pbw`.

On **macOS/Linux**, [install the SDK](https://developer.repebble.com/sdk) and
run `pebble build`.

Put it on the watch by opening the `.pbw` in the Pebble app, or:

```powershell
./build.ps1 -Phone 192.168.1.23
```
