
<p align="center">
  <h1 align="center">
    faust~
  </h1>
  <p align="center">
    The FAUST compiler embedded in a Pd external
  </p>
  <p align="center">
    <a href="https://travis-ci.org/pierreguillot/faust-pd"><img src="https://img.shields.io/travis/pierreguillot/faust-pd.svg?label=travis" alt="Travis CI"></a>
    <a href="https://ci.appveyor.com/project/pierreguillot/faust-pd/history"><img src="https://img.shields.io/appveyor/ci/pierreguillot/faust-pd.svg?label=appveyor" alt="Appveyor CI"></a>
  </p>
</p>

### Presentation

The pure-data counterpart of the Max external faustgen~. This project is mainly a testing and experimental ground to embed the FAUST compiler with LLVM inside another application. The final goal is to integrate a similar feature in the application KIWI.

### Dependencies

- [llvm](http://llvm.org)
- [faust](https://github.com/grame-cncm/faust.git)
- [pure-data](https://github.com/pure-data/pure-data.git)
- [pd.build](https://github.com/pierreguillot/pd.build.git)

## Installing LLVM

The FAUST compiler requires LLVM 5.0.0 backend (or higher - 6.0.0). The fastest solution is to download the precompiled binaries from the [LLVM website](http://releases.llvm.org). For example, on the Travis CI for MacOS, we assume for example that the binaries are installed in the llvm folder at the root of the project:
```
curl -o ./llvm.tar.gz http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-x86_64-apple-darwin.tar.xz
tar zxvf ./llvm.tar.gz && mv clang+llvm-5.0.0-x86_64-apple-darwin llvm
```
You can also use HomeBrew or MacPorts on MacOS or APT on Linux the compilation of the sources last around 50 minutes and in this case, you change the LLVM_DIR with the proper location. On Windows, you must compile from sources using the static runtime library. You can also use the pre-compiled libraries used on the Appveyor CI.   

## Compilation

```
git submodule update --init --recursive
mkdir build && cd build
cmake ..
cmake --build .
```
Useful CMake options:
- `USE_LLVM_CONFIG` to disable default LLVM location for FAUST (for example: `-DUSE_LLVM_CONFIG=off`).
- `LLVM_DIR` to define LLVM location for FAUST and the Pd external (for example: `-DLLVM_DIR=./llvm/lib/cmake/llvm`).

see also the files `.travis.yml` and `appveyor.yml`.
