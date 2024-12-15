# Set Board

```
west config --local build.board nucleo_h503rb
```

# Build

## Blinky example

```
west build --pristine=auto deps/zephyr/samples/basic/blinky
```

## This application

```
west build --pristine=auto --extra-dtc-overlay=stm32h503-nucleo-i3c.overlay .
```

# udev rules

you must add the stlink to udev for pyocd

start udev
```
sudo service udev start
```

copy the stlink rules from https://github.com/pyocd/pyOCD/blob/main/udev/README.md

# Flash

```
west flash --runner pyocd
```

# Shell

```
minicom --color=on -D /dev/ttyACM0
```