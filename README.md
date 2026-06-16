# Intel Open Image Denoiser command line tool

This is a simple implementation of Intels [Open Image AI denoiser](https://github.com/OpenImageDenoise/oidn). This is essentially an implmentation of the example executable provided in the original repository but instead uses OIIO so that a larger variety of image formats are supports. You can find a pre-built windows distribution in the releases tab of this repro.

## Building
The build uses CMake and [Conan](https://conan.io/) to pull in the OpenImageIO dependency. The Intel Open Image Denoise (OIDN) library is downloaded automatically from its [GitHub releases](https://github.com/RenderKit/oidn/releases) during the CMake configure step (the version is controlled by the `OIDN_VERSION` CMake cache variable, default `2.5.0`).
```
python3 -m pip install conan
conan profile detect --force
conan install . --build=missing -s:h compiler.cppstd=20 -s:b compiler.cppstd=20
cmake -S . -B build -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./out
cmake --build build --config Release
cmake --install build --config Release
```
OpenImageIO 3.x (which Conan resolves for the `>=2.4 <4` range) requires **C++17 or newer** and a reasonably modern compiler, so `compiler.cppstd=20` above is mandatory — an older default profile (e.g. `gnu14`) makes Conan reject `openimageio`, `opencolorio` and friends as invalid.

### Linux (RHEL 8 / older system GCC)
The system GCC 8 cannot build the OpenEXR/OpenImageIO stack (its `std::filesystem` is incomplete). Build with a newer toolchain, e.g. `gcc-toolset-11`:
```
sudo dnf install gcc-toolset-11 gcc-toolset-11-libatomic-devel libatomic
source /opt/rh/gcc-toolset-11/enable
conan profile detect --force          # now detects gcc 11
conan install . --build=missing -s compiler.cppstd=20
```
The `gcc-toolset-11-libatomic-devel` package is needed because OpenImageIO's command line tools link `-latomic`; without it the build fails at the final link with `cannot find -latomic`. If you cannot install that package but do have the base `libatomic` runtime, point the linker at a directory containing only a `libatomic.so` symlink (do **not** add a whole system GCC lib dir to `-L`, or it will shadow the toolset's `libstdc++` and break the link with errors like `undefined reference to std::__throw_bad_array_new_length()`):
```
mkdir -p ~/.local/lib/oiio-atomic
ln -sf /usr/lib64/libatomic.so.1.2.0 ~/.local/lib/oiio-atomic/libatomic.so
conan install . --build=missing -s compiler.cppstd=20 \
  -c tools.build:exelinkflags="['-L$HOME/.local/lib/oiio-atomic']" \
  -c tools.build:sharedlinkflags="['-L$HOME/.local/lib/oiio-atomic']"
```

## Usage
Command line parameters
* -v [int]        : log verbosity level 0:disabled 1:simple 2:full (default 2)
* -i [string]     : path to input image
* -o [string]     : path to output image
* -a [string]     : path to input albedo AOV (optional)
* -n [string]     : path to input normal AOV (optional, requires albedo AOV)
* -hdr [int]      : Image is a HDR image. Disabling with will assume the image is in sRGB (default 1 i.e. enabled)
* -srgb [int]     : whether the main input image is encoded with the sRGB (or 2.2 gamma) curve (LDR only) or is linear (default 0 i.e. disabled)
* -t [int]        : number of threads to use (defualt is all)
* -affinity [int] : Enable affinity. This pins virtual threads to physical cores and can improve performance (default 0 i.e. disabled)
* -repeat [int]   : Execute the denoiser N times. Useful for profiling.
* -maxmem [int]   : Maximum memory size used by the denoiser in MB
* -clean_aux [int]: Whether the auxiliary feature (albedo, normal) images are noise-free; recommended for highest quality but should *not* be enabled for noisy auxiliary images to avoid residual noise (default 0 i.e. disabled)
* -h/--help : Lists command line parameters

You need to at least have an input and output for the app to run. If you also have them, you can add an albedo AOV or albedo and normal AOVs to improve the denoising. All images should be the same resolutions, not meeting this requirement will lead to unexpected results (likely a crash).

For best results provide as many of the AOVs as possible to the denoiser. Generally the more information the denoiser has to work with the better. The denoiser also prefers images rendered with a box filter or by using FIS.

Please refer to the originial OIDN repository [here](https://github.com/OpenImageDenoise/oidn) for more information.

## Examples
Here is a quick example scene that uses the image from the original OIDN repository. These can can also be found in the image folder of this repository.

### Noisy image
<p align="center">
  <img src="https://github.com/DeclanRussell/IntelOIDenoiser/blob/master/images/car_beauty.jpg" alt="test"/>
</p>

### Denoised output
<p align="center">
  <img src="https://github.com/DeclanRussell/IntelOIDenoiser/blob/master/images/car_test_intel.jpg" alt="denoise_test"/>
</p>

# Simple sequence batch script
As it has been widely requested here is a very simple batch script for denoising sequences until I have time to implement something proper into the application itself. It will do the most simple denoising without any feature AOVs. Save the following code into a file named Sequence.bat and place it into the directory where your images are saved. Running this script will denoise all files image files that match the chosen file extension in the folder. There are three parameters that you will need to edit in the script,

* FILE_EXTENSION – the file extension of your image
* PATH_TO_DENOISER – the full directory of the Denoiser.exe
* OUTPUT_PREFIX – a prefix which is prepended to the name of the image to create the output name. I.e. with the prefix denoised_ the image test.jpg will become denoised_test.jpg

```
SET FILE_EXTENSION=jpg
SET PATH_TO_DENOISER=D:\Projects\IntelOIDenoiser\Denoiser_v1.0
SET OUTPUT_PREFIX=denoised_

for /r %%v in (*.%FILE_EXTENSION%) do %PATH_TO_DENOISER%\Denoiser.exe -i "%%~nv.%FILE_EXTENSION%" -o "%OUTPUT_PREFIX%%%~nv.%FILE_EXTENSION%"

cmd /k
```

# Licence info
This licence has an MIT licence.
