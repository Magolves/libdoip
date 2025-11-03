# libdoip
C/C++ library for Diagnostics over IP (DoIP) (fork from https://github.com/GerritRiesch94/libdoip)

Despite the excellent work I saw some shortcomings in the design and refactored some parts - hopefully for the better.

The C++ standard is set to C++ 17. However, so far the code does not use C++ 17 features and could compile also with older standards like C++ 11 or 14.

Changes in particular

- Introduced cmake build and test env
- Replace `gtest`with `doctest` (just a personal preference - IMHO `gtest` sucks)
- Introduced `Address` struct, since the addresses had many different representations
- Introduced value semantics (WIP)

### Installing library for Diagnostics over IP

1. To install the library on the system, first get the source files with:
```
git clone https://github.com/Magolves/libdoip.git
[git clone https://github.com/GerritRiesch94/libdoip](https://github.com/Magolves/libdoip.git)
```

2. Enter the directory 'libdoip' and build the library with:
```
make
```

3. To install the builded library into `/usr/lib/libdoip` use:

```
sudo make install
```

### Installing doctest

```
sudo apt install doctest
```