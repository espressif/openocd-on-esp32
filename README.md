# OpenOCD application for ESP32S3

## Prerequisites

- ESP32S3 board with PSRAM. This board will act as a debugger. Make sure to adjust Flash and PSRAM spi modes (DIO, QIO, OPI) from the menuconfig.
- ESP32 target board, flashed e.g. with blink example.
- Connect GPIOs to the debugger board to the target board:

  |Master pin | Function | Slave pin |
  |-----------|----------|-----------|
  | 13        | TCK      | 13        |
  | 14        | TMS      | 14        |
  | 12        | TDI      | 12        |
  | 11        | TDO      | 15        |

If necessary, the master pins can be adjusted at run time using `esp32_gpio_jtag_nums <tck> <tms> <tdi> <tdo>` OpenOCD command. For example,

    esp32_gpio_jtag_nums 13 14 12 11

sets the configuration shown above. The default pins are set in `interface/esp32_gpio.cfg` file (in the OpenOCD submodule).

- Connect a led to see the JTAG tx/rx activity (optional)

  If a led is connected to one of the GPIO pins, it can be activated with  `esp32_gpio_blink_num <led_pin_num>` OpenOCD command inside `interface/esp32_gpio.cfg`

## Build and flash

1. Get the submodules: `git submodule --update --init --recursive`.

2. Configure Wi-Fi SSID and password in menuconfig.

3. Run `idf.py flash monitor` to build and flash the application and the FATFS filesystem image (generated at build time). When flashing again, use `idf.py app-flash` to only flash the application.

## Flash from jtag

With the help of OpenOCD, it is also possible to load application from the jtag. During the cmake configuration phase, openocd binary is beging searched in the installed idf tools directory. If the binary is found, 2 commands will be ready to run;

1. Use `cmake --build build -t jtag-flasher-full`  to flash the application and the FATFS filesystem

2. Use `cmake --build build -t jtag-flasher` to only flash the application to the specific locatioon. Which is 0x10000 by default.

`board/esp32s3-builtin.cfg` is used as an OpenOCD config file by default.

## Run

Open the monitor and check that the ESP32S3 output ends with the following:

```
I (6393) example_common: - IPv4 address: 192.168.32.56,
I (6393) example_common: - IPv6 address: fe80:0000:0000:0000:7edf:a1ff:fee0:104c, type: ESP_IP6_ADDR_IS_LINK_LOCAL
Open On-Chip Debugger 0.11.0 (2022-11-09-16:08)
Licensed under GNU GPL v2
For bug reports, read
        http://openocd.org/doc/doxygen/bugs.html
debug_level: 2

Warn : Could not determine executable path, using configured BINDIR.
Info : only one transport option; autoselect 'jtag'
esp32_gpio GPIO config: tck = 13, tms = 14, tdi = 12, tdo = 11

Warn : Transport "jtag" was already selected
Info : esp32_gpio GPIO JTAG bitbang driver
Info : clock speed 1000 kHz
Info : JTAG tap: esp32.cpu0 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : JTAG tap: esp32.cpu1 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : starting gdb server for esp32.cpu0 on 3333
Info : Listening on port 3333 for gdb connections
Info : [esp32.cpu0] Debug controller was reset.
Info : [esp32.cpu0] Core was reset.
Info : [esp32.cpu1] Debug controller was reset.
Info : [esp32.cpu1] Core was reset.
Info : JTAG tap: esp32.cpu0 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : JTAG tap: esp32.cpu1 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : [esp32.cpu0] requesting target halt and executing a soft reset
Info : [esp32.cpu0] Target halted, PC=0x400845FE, debug_reason=00000000
Info : [esp32.cpu0] Reset cause (1) - (Power on reset)
Info : Set GDB target to 'esp32.cpu0'
Info : [esp32.cpu1] Target halted, PC=0x400845FE, debug_reason=00000000
Info : [esp32.cpu1] Reset cause (14) - (CPU1 reset by CPU0)
Info : [esp32.cpu0] Debug controller was reset.
Info : [esp32.cpu0] Core was reset.
Info : [esp32.cpu0] Target halted, PC=0x500000CF, debug_reason=00000000
Info : [esp32.cpu0] Reset cause (3) - (Software core reset)
Info : [esp32.cpu1] requesting target halt and executing a soft reset
Info : [esp32.cpu0] Core was reset.
Info : [esp32.cpu0] Target halted, PC=0x40000400, debug_reason=00000000
Info : [esp32.cpu1] Debug controller was reset.
Info : [esp32.cpu1] Core was reset.
Info : [esp32.cpu1] Target halted, PC=0x40000400, debug_reason=00000000
Info : [esp32.cpu1] Reset cause (14) - (CPU1 reset by CPU0)
Info : [esp32.cpu0] Reset cause (3) - (Software core reset)
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
```

If OpenOCD complains about the JTAG chain, 0x00's, 0x1f's, check the connections.

Note the IP printed in the "example_connect" line before OpenOCD output starts.

Run GDB and instruct it to connect to the debugger over TCP:

    xtensa-esp32-elf-gdb  -ex "set remotetimeout 30" -ex "target extended-remote 192.168.0.221:3333" build/blink.elf

(replace the IP address with the one you saw in the log).

## Web server connection

In the first run, ESP32-OOCD runs as an access point (ap mode) with default ssid `ESP32-OOCD` and default password `esp32pass`. You can access the web server by typing the IP address `192.168.4.1` on a browser. Then, you will see the configuration menu to instantly change wifi settings and OpenOCD command line arguments.
The last saved Wi-Fi and OpenOCD configuration will be used in every reset. Wifi mode will be switched to the `station` after the default password is changed. If there will be a connection issue after some retrying ESP32-OOCD will return to the wifi factory setting.

## OpenOCD configuration

For the OpenOCD configuration, there are three parameters, which are --file/-f, --command/-c and --debug/-d. If we detail them:

- `--file/-f` expects only connected target config file name, e.g. `target/esp32s2.cfg`. The default value of this parameter is "target/esp32.cfg" and it cannot be empty in the OpenOCD configuration. The interface config file is set to statically `interface/esp32_gpio.cfg`
- `--command/-c` takes command arguments. e.g. `init; reset halt`
- `--debug/-d` takes debug verbosity value e.g. `3`
