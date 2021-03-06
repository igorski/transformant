# TRANSFORMANT

Transformant is a VST/AU plug-in which provides a stereo formant filter effect, driven by oscillators and obliterated to gravel through bit crushing or wave shaping.

You can hear it being the leading effect on Drosophelia's [Detractor](https://songwhip.com/drosophelia/detractor).

## On compatibility

### Build as VST 2.4

VST3 is great and all, but support across DAW's is poor (looking at a certain popular German product). You can however build as a VST2.4 plugin and enjoy it on a wider range of host platforms.

However: as of SDK 3.6.11, Steinberg no longer packages the required _./pluginterfaces/vst2.x_-folder inside the VST3_SDK folder.
If you wish to build a VST2 plugin, copying the folder from an older SDK version _could_ work (verified 3.6.9. _vst2.x_ folders to work with SDK 3.7.0), though be aware
that you _need a license to target VST2_. You can view [Steinbergs rationale on this decision here](https://www.steinberg.net/en/newsandevents/news/newsdetail/article/vst-2-coming-to-an-end-4727.html).

Once your SDK is "setup" for VST2, simply uncomment the following line in _CMakeLists.txt_:

```
set(SMTG_CREATE_VST2_VERSION "Use VST2" ON)
```

And rename the generated plugin extension from _.vst3_ to _.vst_ (or _.dll_ on Windows).

## Build instructions

The project uses [CMake](https://cmake.org) to generate the Makefiles and has been built and tested on macOS, Windows 10 and Linux (Ubuntu).

### Environment setup

Apart from requiring _CMake_ and a C(++) compiler such as _Clang_ or _MSVC_, the only other dependency is the [VST SDK from Steinberg](https://www.steinberg.net/en/company/developers.html) (the projects latest update requires SDK version 3.7.0).

Be aware that prior to building the plugin, the Steinberg SDK needs to be built from source as well. Following Steinbergs guidelines, the build target should be a _/build_-subfolder of the _/VST3_SDK_-folder.
To generate a release build of the library, execute the following commands from the root of the Steinberg SDK folder:

```
cd VST3_SDK
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

The result being that _{VST3_SDK_ROOT}/VST3_SDK/build/lib/Release/_ will contain the Steinberg VST libraries required to build the plugin.
NOTE: Windows users need to append _--config Release_ to the last cmake (build) call as the build type must be defined during this step.

If you intend to build VST2 versions as well, run the following from the root of the Steinberg SDK folder (run the _.bat_ version instead of the _.sh_ version on Windows) prior to building the library:

```
./copy_vst2_to_vst3_sdk.sh
```

And if you are running Linux, you can easily resolve all dependencies by first running the following from the root of the Steinberg SDK folder:

```
./tools/setup_linux_packages_for_vst3sdk.sh
```

### Building the plugin

Run CMake to generate the Makefile for your environment, after which you can compile the plugin using make. The build output will be stored in _./build/VST3/transformant.vst3_ as well as symbolically linked to your systems VST-plugin folder (on Unix).

You must provide the path to the Steinberg SDK by providing _VST3_SDK_ROOT_ to CMake like so:

```
cmake -DVST3_SDK_ROOT=/path/to/VST_SDK/VST3_SDK/ ..
```

#### Compiling on Unix systems:

```
mkdir build
cd build
cmake -DVST3_SDK_ROOT=/path/to/VST_SDK/VST3_SDK/ ..
make .
```

#### Compiling on Windows:

Assuming the Visual Studio Build Tools have been installed:

```
mkdir build
cd build
cmake.exe -G"Visual Studio 16 2019" -DVST3_SDK_ROOT=/path/to/VST_SDK/VST3_SDK/ ..
cmake.exe --build .
```

### Running the plugin

You can copy the build output into your system VST(3) folder and run it directly in a VST host / DAW of your choice.

When debugging, you can also choose to run the plugin against Steinbergs validator and editor host utilities:

    {VST3_SDK_ROOT}/build/bin/validator  build/VST3/transformant.vst3
    {VST3_SDK_ROOT}/build/bin/editorhost build/VST3/transformant.vst3

### Build as Audio Unit (macOS only)

Is aided by the excellent [Jamba framework](https://github.com/pongasoft/jamba) by Pongasoft, which provides a toolchain around Steinbergs SDK. Execute the following instructions to build Transformant as an Audio Unit:

* Build the AUWrapper Project in the Steinberg SDK folder
* Create a Release build of the Xcode project generated in step 1, this creates _VST3_SDK/public.sdk/source/vst/auwrapper/build/lib/Release/libauwrapper.a_
* Run _sh build_au.sh_ from the repository root, providing the path to _VST3_SDK_ROOT_ as before:

```
VST3_SDK_ROOT=/path/to/VST_SDK/VST3_SDK sh build_au.sh
```

The subsequent Audio Unit component will be located in _./build/VST3/transformant.component_ as well as linked
in _~/Library/Audio/Plug-Ins/Components/_

You can validate the Audio Unit using Apple's _auval_ utility, by running _auval -v aufx dely IGOR_ on the command line. Note that there is the curious behaviour that you might need to reboot before the plugin shows up, though you can force a flush of the Audio Unit cache at runtime by running _killall -9 AudioComponentRegistrar_.
