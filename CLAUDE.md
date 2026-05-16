# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository purpose

Recording and analysis tooling for two SDR platforms: Ettus USRP b200mini (UHD) and Nuand bladeRF 2.0 micro (xA5/xA9, libbladeRF). The C++ side records raw I/Q to a custom binary file format; the MATLAB side parses those files and runs analyses (spectrograms, channelization, PDW extraction, event prediction).

## Build (cpp)

CMake-based build, out-of-source under `cpp/build/`:

```
cd cpp && cmake -S . -B build && cmake --build build
```

Produces `*.out` executables in `cpp/build/`. Notes:

- bladeRF targets resolve libbladeRF via `pkg_check_modules` (`PkgConfig::LIBBLADERF`), so `pkg-config` (or `pkgconf`) must be on PATH and libbladeRF's `.pc` file must be discoverable. On macOS Apple Silicon: `brew install pkgconf libbladerf`.
- USRP targets need UHD ≥ 4.5.0 and Boost ≥ 1.65 found via `find_package`. If only one toolchain is installed, the missing `find_package` will fail the whole configure — comment out the unrelated targets if needed.
- C++ standard varies per target (11/17/20) — preserve the per-target `set_property(... CXX_STANDARD ...)` when adding new sources.

## bladeRF FPGA/firmware loading

Before running bladeRF recorders, the device must have its FPGA bitstream loaded (it's volatile). The two helper scripts are not interchangeable — pick by hardware variant:

- `cpp/loadFpgaA5` — for bladeRF 2.0 micro **xA5**
- `cpp/loadFpgaA9` — for bladeRF 2.0 micro **xA9**

Both expect the `.rbf` and `.img` files at `~/workarea/`.

## Recording binary file format (`cpp/IqPacket.h`)

All recorders write a fixed-size `IqPacket` header followed by interleaved I/Q samples (`int8` if `bitWidth ≤ 8`, otherwise `int16`). Critical detail: **the first 4 bytes (`endianness`) double as a format-version sentinel**, and MATLAB readers branch on it:

| Sentinel       | Meaning                                                     |
|----------------|-------------------------------------------------------------|
| `0x00000000`   | Big endian (format assumed v2 — gap, can't disambiguate)    |
| `0x01010101`   | Little endian, format v1 (frequency stored as uint32 — broken above 2^32 Hz) |
| `0x02020202`   | Little endian, format v2 (frequency widened to uint64, adds `spare0`)         |
| `0x03030303`   | Little endian, format v3 (`rxGainDb` becomes float32 instead of uint32)       |

`IQ_FILE_FORMAT` in `IqPacket.h` is currently `3`. **If you change the struct layout, bump this AND add a new endianness sentinel AND update every MATLAB reader's `switch endianness` block** — `convert_my_iq_to_mat.m`, `plot_my_iq.m`, and `spectrogram_my_iq.m` all duplicate the parsing logic.

The recorders compensate for filter startup transients by allocating `dwellDuration*Fs + FILTER_DELAY` samples and writing only the trailing `dwellDuration*Fs` to disk; `sampleStartTime` is offset accordingly. The CLI argument order for all four recorders is `<freqMhz> <bwMhz> <sampleRateMsps> <gainDb> <dwellSec> <durationSec> <filterDelay>`.

## MATLAB pipeline

The MATLAB scripts are not functions — they're top-level scripts that `clearvars` / `clear all` and prompt with `uigetdir`/`uigetfile`. Run them from the MATLAB IDE; there is no test harness. The expected pipeline is:

1. `convert_my_iq_to_mat.m` — reads a directory of `.iq` files and emits `.mat` files containing the variables `iq, fs, fc, bw, gain, bitWidth, sampleStartTime, linkSpeed, boardName, serialNo, fpgaVersion, fwVersion`. Every downstream script assumes this schema.
2. Analysis: `plot_my_iq.m`, `spectrogram_my_iq.m`, `create_pdws.m`, `create_pdws_channelized.m`, `predict_event.m`, `channelizer_example.m`. Several of these (`plot_my_iq`, `spectrogram_my_iq`) also accept raw `.iq` and re-implement the header parser inline.

### PDW generation algorithm

Both `create_pdws.m` and `create_pdws_channelized.m` use the same shape: noise floor = `median(abs(iq))`, pulse threshold = `noise_floor * 10^(SNR_THRESHOLD/10)`, leading/trailing edge state machine, frequency estimated from `median(diff(unwrapped phase))` over the pulse, saturation flag set if any in-pulse sample exceeds `±0.9999`. The non-channelized version uses a separate (lower) trailing-edge threshold so pulses are closed once they fall back near the noise floor; the channelized version uses the same threshold for both edges. Keep these consistent with the comments — the pulse-width and frequency math relies on `fs` being the per-bin decimated rate after channelization.

## Things not to break

- `IQ_FILE_FORMAT`, the endianness sentinel, and the MATLAB readers must move together.
- Don't change the CLI argument order of the recorders; downstream scripts and notes assume it.
- The `FILTER_DELAY` discard logic appears in every recorder — if you refactor it, refactor all four.
