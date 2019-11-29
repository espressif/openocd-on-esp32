# OpenOCD application for ESP32

## Prerequisites

- ESP32 board with PSRAM. ECO3 chip is preferred. This board will act as a debugger.
- ESP32 target board, flashed e.g. with blink example.
- Connect GPIOs to the debugger board to the target board:

  |Master pin | Function | Slave pin |
  |-----------|----------|-----------|
  | 18        | TCK      | 13        |
  | 19        | TMS      | 14        |
  | 21        | TDI      | 12        |
  | 22        | TDO      | 15        |

If necessary, the master pins can be adjusted at run time using `esp32_gpio_jtag_nums <tck> <tms> <tdi> <tdo>` OpenOCD command. For example,

    esp32_gpio_jtag_nums 18 19 21 22

sets the configuration shown above. The default pins are set in `interface/esp32_gpio.cfg` file (in the OpenOCD submodule).

## Build and flash

1. Get the submodules: `git submodule --update --init --recursive`.

2. Configure Wi-Fi SSID and password in menuconfig.

3. Run `idf.py build`. This should build the application, `jimtcl` interpreter and OpenOCD. `jimtcl` and `openocd` require make and autotools for the build, so this might not work on Windows...

4. Run `idf.py flash` to flash the application and the SPIFFS filesystem image (generated at build time). When flashing again, use `idf.py app-flash` to only flash the application.

## Run

Open the monitor and check that the ESP32 output ends with the following:

```
I (4006) example_connect: IPv4 address: 192.168.0.221
I (4016) example_connect: IPv6 address: fe80:0000:0000:0000:1afe:34ff:fe71:e720
Open On-Chip Debugger  v0.10.0-esp32-20191112-2-gab0f29e9-dirty (2019-11-29-17:04)
Licensed under GNU GPL v2
For bug reports, read
        http://openocd.org/doc/doxygen/bugs.html
Warn : Could not determine executable path, using configured BINDIR.
Info : only one transport option; autoselect 'jtag'
adapter speed: 1000 kHz
esp32_gpio GPIO config: tck = 18, tms = 19, tdi = 21, tdo = 22
Warn : Transport "jtag" was already selected
Info : Configured 2 cores
Error: type 'esp32' is missing virt2phys
Info : esp32_gpio GPIO JTAG bitbang driver
Info : clock speed 1000 kHz
Info : JTAG tap: esp32.cpu0 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : JTAG tap: esp32.cpu1 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : Target halted. CPU0: PC=0x400D395A (active)
Info : Target halted. CPU1: PC=0x400E285A 
Info : Listening on port 3333 for gdb connections
Info : JTAG tap: esp32.cpu0 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : JTAG tap: esp32.cpu1 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : cpu0: Debug controller 0 was reset.
Info : cpu0: Core 0 was reset.
Info : cpu0: Target halted, PC=0x500000CF, debug_reason=00000000
Info : esp32: Core 0 was reset.
Info : esp32: Debug controller 1 was reset.
Info : esp32: Core 1 was reset.
Info : Target halted. CPU0: PC=0x40000400 (active)
Info : Target halted. CPU1: PC=0x40000400 
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
```

If OpenOCD complains about the JTAG chain, 0x00's, 0x1f's, check the connections.

Note the IP printed in the "example_connect" line before OpenOCD output starts.

Run GDB and instruct it to connect to the debugger over TCP:

    xtensa-esp32-elf-gdb  -ex "set remotetimeout 30" -ex "target remote 192.168.0.221:3333" build/blink.elf

(replace the IP address with the one you saw in the log).
