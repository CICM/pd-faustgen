# faust-pd
The FAUST compiler embedded in a Pd external

The pure-data counterpart of the Max external faustgen~. This project is mainly an testing and experimental ground to embed the FAUST compiler with LLVM inside another application. The final goal is to integrate a similar feature in the application KIWI.

## Dependencies
- [faust](https://github.com/grame-cncm/faust.git)
- [pure-data](https://github.com/pure-data/pure-data.git)
- [pd.build](https://github.com/pierreguillot/pd.build.git)

## Installing LLVM

The FAUST compiler requires LLVM 5.0 backend.

- macos:

The fastest solution is to download the precompiled binaries from the [LLVM website](http://releases.llvm.org). On the macos CI, we assume for example that the binaries are installed in the llvm folder at the root of the project:
```
- curl -o ./llvm.tar.gz http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-x86_64-apple-darwin.tar.xz
- tar zxvf ./llvm.tar.gz && mv clang+llvm-5.0.0-x86_64-apple-darwin llvm
```

You can also use HomeBrew or MacPorts but the compilation of the sources last around 50 minutes and in this case, you change the LLVM_DIR with the proper location.
```
- brew install llvm-5.0 +universal
```
or
```
- port install llvm-5.0 +universal
```

- linux:

## Installing pkg-config

The FAUST compiler requires pkg-config. can also use HomeBrew or MacPorts this application:
``
- brew install pkg-config
```
or
```
- port install pkgconfig
```

## Compilation

- macos:
```
- git submodule update --init --recursive
- mkdir faust/build/forpd && cd faust/build/forpd
- cmake .. -DINCLUDE_STATIC=on -DINCLUDE_OSC=off -DINCLUDE_HTTP=off -DUNIVERSAL=on -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -C../backends/backends.cmake
- make
- mkdir ../../../build && cd ../../../build
- cmake .. -GXcode "-DCMAKE_OSX_ARCHITECTURES=i386;x86_64" -DLLVM_DIR=./llvm/lib/cmake/llvm
```
- linux:
```
- git submodule update --init --recursive
- mkdir faust/build/forpd && cd faust/build/forpd
- cmake .. -DINCLUDE_STATIC=on -DINCLUDE_OSC=off -DINCLUDE_HTTP=off -DCMAKE_C_FLAGS=-m64 -C../backends/backends.cmake
- make --quiet
- mkdir ../../../build && cd ../../../build
- cmake .. -DCMAKE_C_FLAGS=-m64
- cmake --build . --config Release
```
