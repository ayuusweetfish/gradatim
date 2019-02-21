### Building

#### \*nix

* Install [SDL](http://libsdl.org/), [SDL_image](https://www.libsdl.org/projects/SDL_image/) and [SDL_ttf](https://www.libsdl.org/projects/SDL_ttf/). Prebuilt packages will work fine.
* Install [libvorbis](https://www.xiph.org/vorbis/) and [PortAudio](http://portaudio.com/). It's likely that your package manager provides them.
* Install [SoundTouch](https://www.surina.net/soundtouch/index.html). **Note:** build the library with `SOUNDTOUCH_INTEGER_SAMPLES` defined (see its documentation).
* Install [IIR1](https://github.com/berndporr/iir1). **Note:** it's likely that you'll need to build and install it yourself. The code may require a little tweaking to compile on some platforms.

By now you are ready to build the game with CMake.

```
cd src
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
make
```

The executable will be named `gradatim`. Note that you'll need to copy the assets into the same folder so that the program can find them.

#### Windows

The following building process is tested under MinGW with MSYS (2013072300). It's rather difficult to build with VS toolchain as the project relies on a few GNU language features.

In the MSYS environment, install the libraries listed above.

Replace the `cmake` line in the \*nix step with the following:

```
cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=RELEASE -DSDL2_BUILDING_LIBRARY=1 ..
```

The executable will be named `gradatim.exe`.
