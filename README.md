
<p align="center">
  <h1 align="center">
  <img width="50" alt="FaustLogo" img src="https://user-images.githubusercontent.com/1409918/64951909-41544a00-d87f-11e9-87dd-720e0f8e1570.png"/> faustgen~ <img width="40" alt="PdLogo" img src="https://user-images.githubusercontent.com/1409918/64951943-5335ed00-d87f-11e9-9b52-b4b6af47d7ba.png"/>
  </h1>
  <p align="center">
    The FAUST compiler embedded in a Pd external
  </p>
  <p align="center">
    <a href="https://travis-ci.org/CICM/pd-faustgen/builds"><img src="https://img.shields.io/travis/CICM/pd-faustgen.svg?label=travis" alt="Travis CI"></a>
    <a href="https://ci.appveyor.com/project/CICM/pd-faustgen/history"><img src="https://img.shields.io/appveyor/ci/CICM/pd-faustgen.svg?label=appveyor" alt="Appveyor CI"></a>
    <a href="https://app.codacy.com/project/CICM/pd-faustgen/dashboard"><img src="https://api.codacy.com/project/badge/Grade/a89aaf703bf045a383ff4a28d6d4b173"/></a>
  </p>
</p>

## Presentation

The **faustgen~** object is an external with the [FAUST](http://faust.grame.fr/about/) just-in-time (JIT) compiler embedded that allows to load, compile and play FAUST files within the audio programming environment [Pure Data](http://msp.ucsd.edu/software.html). FAUST (Functional Audio Stream) is a functional programming language specifically designed for real-time signal processing and synthesis developed by the [GRAME](http://www.grame.fr/). The FAUST JIT compiler - built with [LLVM](https://llvm.org/) - brings together the convenience of a standalone interpreted language with the efficiency of a compiled language. The **faustgen~** object is a very first version with elementary features, any help and any contribution are welcome.

<p align="center">
<a href="https://vimeo.com/282672255"><img width="440" alt="video1" src="https://user-images.githubusercontent.com/1409918/44655622-1be76a80-a9f6-11e8-90dc-ce01d6734a28.png"></a> <a href="https://vimeo.com/286662395"><img width="440" alt="video2" src="https://user-images.githubusercontent.com/1409918/44655623-1be76a80-a9f6-11e8-86e0-4519609f2e4c.png"></a>
</p>


**Dependencies:**

- [LLVM](http://llvm.org)
- [FAUST](https://github.com/grame-cncm/faust.git)
- [Pure Data](https://github.com/pure-data/pure-data.git)
- [CMake](https://cmake.org/)
- [pd.build](https://github.com/pierreguillot/pd.build.git)

## Compilation

The FAUST compiler requires LLVM 9.0.0 backend (or higher). Once LLVM is installed on your machine, you can use CMake to generate a project that will compile both the FAUST library and the Pure Data external. Then you can use Deken to release the external.

#### Installing LLVM

The fastest solution to install LLVM is to download the precompiled binaries from the [LLVM website](http://releases.llvm.org). For example, on the Travis CI for MacOS, we assume that the binaries are installed in the llvm folder at the root of the project:

```
curl -o ./llvm.tar.xz -L https://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-darwin-apple.tar.xz
tar xzf ./llvm.tar.xz && mv clang+llvm-9.0.0-x86_64-darwin-apple llvm
```
or a for a linux system
```
curl -o ./llvm.tar.gz https://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz
tar xvf ./llvm.tar.gz && mv clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-14.04 llvm
```
You can also use HomeBrew or MacPorts on macOS or APT on Linux the compilation of the sources last around 50 minutes and in this case, you change the LLVM_DIR with the proper location.

On Windows, you must compile from sources using the static runtime library. Compiling LLVM with the Microsoft Visual Compiler requires to use the static runtime library, for example:
```
cd llvm-9.0.0.src && mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -DLLVM_USE_CRT_DEBUG=MTd -DLLVM_USE_CRT_RELEASE=MT -DLLVM_BUILD_TESTS=Off -DCMAKE_INSTALL_PREFIX="./llvm" -Thost=x64
cmake --build . --target ALL_BUILD (--config Debug/Release)
cmake --build . --target INSTALL (optional)
```
You can also use the pre-compiled libraries used on the Appveyor CI.

#### Compiling the FAUST library and the Pd external

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

#### Publishing with Deken

Once the binaries are compiled or uploaded with Travis and Appveyor to the releases section of GitHub, the external can be published using [Deken](https://github.com/pure-data/deken). First of all, you must have an account on the website https://puredata.info and the [Deken plugin for developers](https://github.com/pure-data/deken/blob/master/developer/README.md) installed. On Windows run the script FaustDeken.bat located in the deken subdirectory with the version of the external, for example: `FaustDeken 0.0.1`. On Unix systems, run the script FaustDeken.sh the deken subdirectory with the version of the external, for example: `FaustDeken.sh 0.0.1`. Once the deken files are prepared, you can upload the files to puredata.info using  `FaustDeken.sh upload 0.0.1`.

## Credits

**FAUST institution**: GRAME  
**FAUST website**: faust.grame.fr  
**FAUST developers**: Yann Orlarey, Stéphane Letz, Dominique Fober and others  

**faustgen~ institutions**: CICM - ANR MUSICOLL  
**faustgen~ website**: github.com/CICM/pd-faustgen  
**faustgen~ developer**: Pierre Guillot

## Legacy

This **faustgen~** object for Pd is inspired by the **faustgen~** object for Max developed by Martin Di Rollo and Stéphane Letz.

Another **faust~** object has been developed by Albert Graef using the programming language [Pure](https://github.com/agraef/pure-lang).
