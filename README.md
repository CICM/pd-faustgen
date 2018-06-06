
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

The **faust~** object is an external with the [FAUST](http://faust.grame.fr/about/) just-in-time (JIT) compiler embedded that allows to load, compile and play FAUST files within the audio programming environment [Pure Data](http://msp.ucsd.edu/software.html). FAUST (Functional Audio Stream) is a functional programming language specifically designed for real-time signal processing and synthesis developed by the [GRAME](http://www.grame.fr/). The FAUST JIT compiler - built with [LLVM](https://llvm.org/) - brings together the convenience of a standalone interpreted language with the efficiency of a compiled language. The **faust~** object is a very first version with elementary features, any help and any contribution are welcome.

This **faust~** object for Pd is inspired by the **faustgen~** object for Max developed by Martin Di Rollo and Stéphane Letz.

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
You can also use HomeBrew or MacPorts on MacOS or APT on Linux the compilation of the sources last around 50 minutes and in this case, you change the LLVM_DIR with the proper location. On Windows, you must compile from sources using the static runtime library. You can also use the pre-compiled libraries used on the Appveyor CI. Compiling LLVM with the Microsoft Visual Compiler requires to use the static runtime library, for example:
```
cd llvm-6.0.0.src && mkdir build && cd build
cmake .. -G "Visual Studio 14 2015 Win64" -DLLVM_USE_CRT_DEBUG=MTd -DLLVM_USE_CRT_RELEASE=MT -DLLVM_BUILD_TESTS=Off -DCMAKE_INSTALL_PREFIX="./llvm" -Thost=x64
cmake --build . --target ALL_BUILD (--config Debug/Release)
cmake --build . --target INSTALL (optional)
```

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

## Publishing with Deken

Once the binaries are uploaded with Travis and Appveyor to the releases section of GitHub, the external can be published using [Deken](https://github.com/pure-data/deken). First of all, you must have an account on the website https://puredata.info and the [Deken plugin for developers](https://github.com/pure-data/deken/blob/master/developer/README.md) installed. On Windows run the script FaustDeken.bat with the version of the external, for example: `FaustDeken 0.0.1`.

## Credits

- FAUST institution: GRAME
- FAUST website: faust.grame.fr
- FAUST developers: Yann Orlarey, Stéphane Letz, Dominique Fober and others

- faust~ institutions: CICM - ANR MUSICOLL
- fauts~ website: github.com/grame-cncm/faust-pd
- faust~ developer: Pierre Guillot
