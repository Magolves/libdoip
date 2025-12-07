# libdoip

[![CI/CD Pipeline](https://github.com/Magolves/libdoip/actions/workflows/ci.yml/badge.svg)](https://github.com/Magolves/libdoip/actions/workflows/ci.yml)
[![Release](https://github.com/Magolves/libdoip/actions/workflows/release.yml/badge.svg)](https://github.com/Magolves/libdoip/actions/workflows/release.yml)
[![License](https://img.shields.io/github/license/Magolves/libdoip)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue.svg)](https://cmake.org/)
[![doctest](https://img.shields.io/badge/Tests-doctest-green.svg)](https://github.com/doctest/doctest)

C/C++ library for Diagnostics over IP (DoIP) (fork from https://github.com/AVL-DiTEST-DiagDev/libdoip)

**CAUTION** The current API is under construction any may change at any time.

## Dependencies

`libdoip` uses `spdlog`. The lib is downloaded automatically. Or you may install it locally via

```bash
# Install Doxygen and Graphviz
sudo apt install libspdlog-dev
```

See [Logging](./doc/LOGGING.md) for details.

### Getting started

Quick start — read the generated tutorial for the example server:

- Online (published): https://magolves.github.io/libdoip/index.html
- Local example page: see `doc/ExampleDoIPServer.md` (included in the Doxygen HTML under "Example DoIP Server Tutorial").
- Example tutorial (direct): https://magolves.github.io/libdoip/ExampleDoIPServer.html

If you want to generate the docs locally, install Doxygen and Graphviz and
run:

```bash
sudo apt install doxygen graphviz
doxygen Doxyfile
xdg-open docs/html/index.html
```

### Installing library for Diagnostics over IP

1. To install the library on the system, first get the source files with:

```bash
git clone https://github.com/Magolves/libdoip.git
```

2. Enter the directory 'libdoip' and build the library with:

```bash
cmake . -Bbuild
cd build
make
```

1. To install the library into `/usr/lib/libdoip` use:

```bash
sudo make install
```

### Installing doctest

```bash
sudo apt install doctest
```

## Debugging

### Dump UDP

```bash
sudo tcpdump -i any udp port 13400 -X
```

## Examples

The project includes a small example DoIP server demonstrating how to
use the `DoIPServer` and `DoIPServerModel` APIs and how to register UDS
handlers.

- Example source files: `examples/exampleDoIPServer.cpp`,
  `examples/ExampleDoIPServerModel.h`
- Example tutorial (published): https://magolves.github.io/libdoip/ExampleDoIPServer.html

See the "Examples" section in the generated Doxygen main page for
additional annotated links to these files.


## References

- [ISO 13400-2:2019(en) Road vehicles — Diagnostic communication over Internet Protocol (DoIP) — Part 2: Transport protocol and network layer services](<https://www.iso.org/obp/ui/#iso:std:iso:13400:-2:ed-2:v1:en>)
- [Specification of Diagnostic over IP](<https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_DiagnosticOverIP.pdf>)
- [Diagnostics over Internet Protocol (DoIP)](<https://cdn.vector.com/cms/content/know-how/_application-notes/AN-IND-1-026_DoIP_in_CANoe.pdf>)