### v0.1.2
- Update Faust (2.72.2)
- Update LLVM (>= 9.0.0)

### v0.1.1
- Fix the method to open the FAUST file on Windows when the path contains a space  

### v0.1.0
- Add support to preserve parameters values during recompilation
- Fix autocompilation when DSP instance not initialized
- Fix support for the double-float-precision option
- Add support to look for the DSP file in all Pd's search paths
- Add support to open the FAUST file in the default text editor when object is clicked
- Add a default locked FAUST file used when no argument is provided

### v0.0.5
- Fix when no argument is provided
- Update deken script
- Fix sources distribution

### v0.0.4
- Move repository to CICM
- Improve/Secure code
- Use repository local LLVM pre-built

### v0.0.3
- Change name from faust~ to faustgen~
- Remove abstraction faust.watcher
- Add support for autocompile method
- Add support for passive parameter
- Add support for list of active parameters
- Add support for ui glue long names
- Add support for dynamic compile options
- Improve print method

### v0.0.2
- Fix Linux dependencies
- Add abstraction faust.watcher

### v0.0.1
- Integration of FAUST lib inside the faust~ external (Linux/MacOS/Windows)
