# Clone

```
west init -m https://github.com/XenuIsWatching/stm32-h5-zephyr.git stm32-h5-zephyr
cd stm32-h5-zephyr
west update
```

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
west build --pristine=auto --extra-dtc-overlay=stm32h503-nucleo-i3c.overlay --extra-dtc-overlay=stm32h503-nucleo-counter.overlay .
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

# Attach USB to WSL from Windows

## From Windows Shell

```
usbipd list
usbipd bind --busid 4-4
usbipd attach --wsl --busid <busid>
```

## In WSL Shell

```
lsusb
```

## Detach from Windows

```
usbipd detach --busid <busid>
```

# Pins on Nucleo

## TIM2

TIM2 CH1 - A0 -- CN8 Arduino Connector Pin 1
TIM2 CH2 - D3 -- CN9 Arduino Connector Pin 4

## I3C

I3C1 SCL - D15 -- CN5 Arduino Connector Pin 10
I3C1 SDA - D14 -- CN5 Arduino Connector Pin 9

I3C2 SCL - D9 -- CN5 Arduino Connector Pin 2
I3C2 SDA - D8 -- CN5 Arduino Connector Pin 1
