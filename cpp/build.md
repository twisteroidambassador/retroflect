# Building C++ version of retroflect

## Requirement

* Visual Studio. I used VS 2017 Community; other versions may also work.
* [WinDivert 1.4.3](https://reqrypt.org/windivert.html). Download
  either the -A or -B binary package.


## Choosing appropriate WinDivert files

Use the following files from WinDivert binary package, according
to the build target:

* x64 build: `WinDivert.dll`, `WinDivert.lib` and
  `WinDivert64.sys` from the `x86_64` directory. This will only run
  on 64-bit Windows.
* x86 build: `WinDivert.dll`,
  `WinDivert.lib`, `WinDivert32.sys` and `WinDivert64.sys` from the
  `x86` directory. This will run on both 32-bit and 64-bit Windows.

For both builds `windivert.h` from the `include` directory is required.

See [WinDivert documentation](https://reqrypt.org/windivert-doc.html)
for details.

## Steps

* Put the appropriate `windivert.h` and `WinDivert.lib` in
  `retroflect\windivert`.
* Open the solution file `retroflect.sln` and build.
* Put `WinDivert.dll` and `WinDivert*.sys` in the same directory as
  the built executable file `retroflect.exe`.
