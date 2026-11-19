# ESP32 PTT trigger with PC818 optocoupler (Icom CI-V emulation)

> 🇵🇱 Polish version: [README.pl.md](README.pl.md)

<!-- ecosystem-link:aprs -->
📡 Part of the **SP9TKN APRS station** — see [how the pieces work together](#part-of-the-sp9tkn-aprs-station).

A tiny USB PTT interface for digital modes (APRS, packet, FT8 …): an **ESP32-WROOM-32** board keys the radio
through a **PC818 optocoupler** (galvanic isolation), with a green RX LED and a red TX LED. The PC controls it
over USB by **emulating the Icom CI-V protocol of the IC-7100** (address 0x88, 9600 bps), so any program with
CAT PTT for the IC-7100 works. The BOOT button keys PTT manually (press and hold) for testing without a PC.

## Wiring

| ESP32 | Part | Note |
|---|---|---|
| GPIO27 | 330 Ω → PC818 pin 1 (anode); pin 2 (cathode) → GND | PTT output |
| — | PC818 pin 4 (collector) → radio PTT, pin 3 (emitter) → radio ground | radio side, isolated |
| GPIO25 | 330 Ω → green LED → GND | RX (listening) |
| GPIO26 | 330 Ω → red LED → GND | TX (transmitting) |
| GPIO0 (BOOT) | on-board button | manual PTT, active low |

The device always starts in RX (PTT off). Pins and CI-V settings are in [`src/config.h`](src/config.h).

## CI-V emulation

The firmware answers the commands needed for PTT: read frequency (`03`), read mode (`04`) and the
transceive / PTT command `1C 00 01` / `1C 00 00` with an OK reply (`FB`). It reports a fixed frequency
(14.074 MHz USB) so that programs which poll the radio are satisfied.

## Program settings (tested)

- **UZ7HO Soundmodem 1.14** — CAT PTT, `Radio=2` in `ptt.ini` (= IC-7100, address 0x88), 9600 bps — works
  reliably. COM PTT with "Swap COM pins" (DTR) also works; COM PTT on RTS resets the board.
- **Direwolf 1.7 / 1.8 (Windows, no hamlib CAT)** — use `PTT COMx DTR`. On ESP32 dev boards the auto-reset circuit
  connects RTS to EN (reset) and DTR to GPIO0, which the firmware treats as the BOOT button = TX.
  Run `mode COMx dtr=off rts=off` before starting Direwolf — the CH340 driver remembers line states from the
  previous session, and a left-over RTS keeps the ESP32 in reset. Never use `PTT COMx RTS` (reset) or
  `-DTR` (continuous transmit).

<!-- ecosystem:aprs:start -->

## Part of the SP9TKN APRS station

Four projects work together as one APRS station on a Windows PC with a Yaesu FT-290R II (2 m). Each part does one job and also works on its own.

| Project | Role |
|---|---|
| [ESP32 PTT trigger](https://github.com/SP9TKN/esp32-ptt-pc818) **← this repository** | keys the radio's PTT through a PC818 optocoupler (galvanic isolation), red/green TX/RX LEDs |
| [Audio PTT](https://github.com/SP9TKN/audio-ptt) | radio audio as a UZ7HO-style waterfall + PTT lamps, read-only |
| [APRSBox for Windows](https://github.com/SP9TKN/APRSBox) | the APRS application: iGate, digipeater, beacons, messages, map (Windows port of SQ9MDD's APRSBox) |
| [CYD-Monitor](https://github.com/SP9TKN/cyd-monitor) | 2.8" ESP32 display as a wireless monitor: the station screen over Wi-Fi, with touch |
| [Direwolf](https://github.com/wb2osz/direwolf) (WB2OSZ) | sound-card modem / TNC (AFSK 1200), used unchanged |

```mermaid
flowchart LR
    RIG["Yaesu FT-290R II<br/>2 m transceiver"]
    SC["USB sound card"]
    PTT["ESP32 PTT trigger<br/>PC818 optocoupler"]
    CYD["CYD-Monitor panel<br/>2.8 in, Wi-Fi"]
    NET(("APRS-IS"))
    subgraph PC["Windows PC"]
        DW["Direwolf<br/>AFSK 1200 modem"]
        AB["APRSBox for Windows"]
        AP["Audio PTT"]
    end
    RIG <-->|"audio"| SC
    SC <--> DW
    DW <-->|"KISS TCP"| AB
    AB <--> NET
    DW -->|"DTR on the USB COM port"| PTT
    PTT -->|"PTT, isolated"| RIG
    SC -.->|"same audio"| AP
    DW -.->|"DTR state, read-only"| AP
    PC -.->|"desktop over Wi-Fi"| CYD
```

**How it works**

1. **Receive.** Audio from the FT-290R II goes through a USB sound card to **Direwolf**, which decodes the AFSK 1200
   packets and hands them to **APRSBox** over KISS TCP. APRSBox stores them, draws them on the map, gates them to
   APRS-IS and digipeats them according to its routing rules.
2. **Transmit.** APRSBox queues a beacon, a message or a frame to digipeat and passes it to Direwolf over KISS.
   Direwolf raises **DTR** on the USB serial port of the **ESP32 PTT trigger** (`PTT COMx DTR`); the board closes the
   radio's PTT through the PC818 optocoupler and lights the red LED, and Direwolf plays the packet audio.
3. **Watch.** **Audio PTT** shows the same radio audio as a waterfall, with two lamps that follow the PTT line. It
   reads the DTR state from the port Direwolf holds, without opening the port, so it never gets in the way of
   transmitting.
4. **From any room.** **CYD-Monitor** mirrors the station desktop (APRSBox map, Audio PTT, Direwolf log) to a 2.8"
   ESP32 "Cheap Yellow Display" over Wi-Fi. The same panel is also the display of the
   [SP9TKN SDR scanner](https://github.com/SP9TKN/cyd-monitor#part-of-the-sp9tkn-sdr-scanner): it jumps to the scanner page when SDR Oxide hears something.
5. **Unattended.** Watchdog scripts (in the APRSBox repository) restart any program that stops and put the windows
   back in place after the monitor sleeps; a virtual display keeps the desktop at 1920×1080 while the physical
   monitor is off.

<!-- ecosystem:aprs:end -->

## Building

[PlatformIO](https://platformio.org/): `pio run -t upload --upload-port COMx`. A ready image is in
**[Releases](../../releases)** — write it at address **0x0**:
`python -m esptool --port COMx write_flash 0x0 <file>.bin`.

## License

[GPL-3.0](LICENSE) © 2026 SP9TKN.

**Tools:** developed with [Claude Code](https://claude.com/claude-code) (Anthropic) — AI assistant used for
coding, testing and documentation.
