# libdoip

[![CI/CD Pipeline](https://github.com/Magolves/libdoip/actions/workflows/ci.yml/badge.svg)](https://github.com/Magolves/libdoip/actions/workflows/ci.yml)
[![Release](https://github.com/Magolves/libdoip/actions/workflows/release.yml/badge.svg)](https://github.com/Magolves/libdoip/actions/workflows/release.yml)
[![License](https://img.shields.io/github/license/Magolves/libdoip)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue.svg)](https://cmake.org/)
[![doctest](https://img.shields.io/badge/Tests-doctest-green.svg)](https://github.com/doctest/doctest)

C/C++ library for Diagnostics over IP (DoIP) (fork from https://github.com/GerritRiesch94/libdoip)

Despite the excellent work I saw some shortcomings in the design and refactored some parts - hopefully for the better.

The C++ standard is set to C++ 17. However, so far the code does not use C++ 17 features and could compile also with older standards like C++ 11 or 14.

Changes in particular are

- Introduced cmake build and test env
- Replace `gtest`with `doctest` (just a personal preference - IMHO `gtest` sucks)
- Introduced `DoIPAddress` struct, since the addresses had many different representations
- Introduced value semantics (WIP)
- ...

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

## References
- [ISO 13400-2:2019(en) Road vehicles — Diagnostic communication over Internet Protocol (DoIP) — Part 2: Transport protocol and network layer services](<https://www.iso.org/obp/ui/#iso:std:iso:13400:-2:ed-2:v1:en>)
- [Specification of Diagnostic over IP](<https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_DiagnosticOverIP.pdf>)
- [Diagnostics over Internet Protocol (DoIP)](<https://cdn.vector.com/cms/content/know-how/_application-notes/AN-IND-1-026_DoIP_in_CANoe.pdf>)