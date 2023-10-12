# OpenOCD application for ESP32-S3

This repository contains a port of OpenOCD which runs on an ESP32-S3. It allows using an ESP32-S3 board as a stand-alone debugger for other chips, which exposes a GDB server port over Wi-Fi or Ethernet.

The repository also demonstrates how a complex application (such as OpenOCD) can be ported to ESP-IDF.

## Prerequisites

- This application can run on ESP32 and ESP32-S3 boards which will function as a debugger.
- PSRAM is necessary for both boards.
- We recommend using ESP32-S3 for improved performance.
- Make sure to adjust Flash and PSRAM spi modes (DIO, QIO, OPI) from the menuconfig.
- ESP32 target board should be flashed e.g. with blink example.
- Connect GPIOs from the debugger board to the target board using JTAG interface.

  |Master pin | Function | Slave pin |
  |-----------|----------|-----------|
  | 38        | TCK      | 13        |
  | 39        | TMS      | 14        |
  | 40        | TDI      | 12        |
  | 41        | TDO      | 15        |

If necessary, the master pins can be adjusted at run time using `esp_gpio_jtag_nums <tck> <tms> <tdi> <tdo>` OpenOCD command. For example,

    esp_gpio_jtag_nums 38 39 40 41

sets the configuration shown above. The default pins are set in `interface/esp_gpio_jtag.cfg` file (in the OpenOCD submodule).

- Connect a led to see the JTAG tx/rx activity (optional)

  If a led is connected to one of the GPIO pins, it can be activated with  `esp_gpio_blink_num <led_pin_num>` OpenOCD command inside `interface/esp_gpio_jtag.cfg`

SWD interface is also supported. The target board can be connected from the SWD pins using `interface/esp_gpio_swd.cfg`

|Master pin | Slave Pin Function |
|-----------|--------------------|
| 38        |       SWCLK        |
| 39        |       SWDIO        |

## Supported IDF versions

All components are expected to be usable with IDF 5.0 or later versions.

## Build and flash

1. Get the submodules: `git submodule update --init --recursive`.

2. Configure Wi-Fi SSID and password in menuconfig.

3. Run `idf.py flash monitor` to build and flash the application and the FATFS filesystem image (generated at build time). When flashing again, use `idf.py app-flash` to only flash the application.

## Flash from jtag

With the help of OpenOCD, it is also possible to load application from the jtag. During the cmake configuration phase, openocd binary is beging searched in the installed idf tools directory. If the binary is found, 2 commands will be ready to run;

1. Use `cmake --build build -t jtag-flasher-full`  to flash the application and the FATFS filesystem

2. Use `cmake --build build -t jtag-flasher` to only flash the application to the specific locatioon. Which is 0x10000 by default.

`board/esp32s3-builtin.cfg` is used as an OpenOCD config file by default.

## Run

Open the monitor and check that the ESP32-S3 output ends with the following:

```
I (6393) example_common: - IPv4 address: 192.168.32.56,
I (6393) example_common: - IPv6 address: fe80:0000:0000:0000:7edf:a1ff:fee0:104c, type: ESP_IP6_ADDR_IS_LINK_LOCAL
Open On-Chip Debugger 0.12.0 (2023-12-20-21:53)
Licensed under GNU GPL v2
For bug reports, read
        http://openocd.org/doc/doxygen/bugs.html
debug_level: 2
Warn : Could not determine executable path, using configured BINDIR.
1
esp_gpio GPIO config: tck = 38, tms = 39, tdi = 40, tdo = 41
3
Info : auto-selecting first available session transport "jtag". To override use 'transport select <transport>'.
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : esp_gpio GPIO JTAG/SWD bitbang driver
Info : clock speed 1000 kHz
Info : JTAG tap: esp32.cpu0 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : JTAG tap: esp32.cpu1 tap/device found: 0x120034e5 (mfg: 0x272 (Tensilica), part: 0x2003, ver: 0x1)
Info : [esp32.cpu0] Examination succeed
Info : [esp32.cpu1] Examination succeed
Info : starting gdb server for esp32.cpu0 on 3333
Info : Listening on port 3333 for gdb connections
Info : [esp32.cpu0] Target halted, PC=0x4008456A, debug_reason=00000000
Info : [esp32.cpu0] Reset cause (1) - (Power on reset)
Info : [esp32.cpu1] Target halted, PC=0x4008456A, debug_reason=00000000
Info : [esp32.cpu1] Reset cause (14) - (CPU1 reset by CPU0)
```

If OpenOCD complains about the JTAG chain, 0x00's, 0x1f's, check the connections.

Note the IP printed in the "example_connect" line before OpenOCD output starts.

Run GDB and instruct it to connect to the debugger over TCP:

    xtensa-esp32-elf-gdb  -ex "set remotetimeout 30" -ex "target extended-remote 192.168.0.221:3333" build/blink.elf

(replace the IP address with the one you saw in the log).

## Web server connection

On the first run, the application creates an access point with default SSID `esp-openocd` without password. You can access the web server by connecting to this network and typing the IP address `192.168.4.1` in a browser. Then, you will see the configuration menu to instantly change Wi-Fi settings and OpenOCD command line arguments.

## ESP-BOX

OpenOCD application has been ported to work on the ESP-BOX development board, with configuration screen and a provisioning feature.
By default, the ESP-BOX and UI functionality are disabled but can be enabled from the menuconfig.

## OpenOCD configuration

The settings for OpenOCD can be configured either through the web page or the touch screen on the ESP-BOX. By default, only the configurations for Espressif chips are preloaded into the file system. However, it is also possible to use other chip configuration files by directly uploading them from the web page.

## License

The code in this repository is Copyright (c) 2022-2023 Espressif Systems (Shanghai) Co. Ltd., and licensed under Apache 2.0 license, available in [LICENSE](LICENSE) file.

OpenOCD source code added as a submodule is licensed under GPL v2.0 or later license.

## Contributing

We welcome contributions to this project in the form of bug reports, feature requests and pull requests.

Issue reports and feature requests can be submitted using Github Issues: https://github.com/espressif/openocd-on-esp32/issues. Please check if the issue has already been reported before opening a new one.

Contributions in the form of pull requests should follow ESP-IDF project's [contribution guidelines](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/index.html).

## Pre-commit hooks

OpenOCD-on-ESP32 project uses [pre-commit hooks](https://pre-commit.com/) to perform code formatting and other checks when you run `git commit`.

To install pre-commit hooks, run `pip install pre-commit && pre-commit install`.

If a pre-commit hook has modified any of the files when you run `git commit`, add these changes using `git add` and run `git commit` again.
