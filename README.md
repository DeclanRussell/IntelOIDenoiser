# Intel Open Image Denoiser command line tool

This is a simple implementation of Intels [Open Image AI denoiser](https://github.com/OpenImageDenoise/oidn). This is essentially an implmentation of the example executable provided in the original repository but instead uses OIIO so that a larger variety of image formats are supports. You can find a pre-built windows distribution in the releases tab of this repro.

Building the source code is pretty simple. The build uses scons so all you need to do is install scons and run it with the Sconstruct in the root of the directory.

## Usage
Command line parameters
* -i [string]     : path to input image
* -o [string]     : path to output image
* -a [string]     : path to input albedo AOV (optional)
* -n [string]     : path to input normal AOV (optional, requires albedo AOV)
* -hdr [int]      : Image is a HDR image. Disabling with will assume the image is in sRGB (default 1 i.e. enabled)
* -t [int]        : number of threads to use (defualt is all)
* -affinity [int] : Enable affinity. This pins vertual threads to physical cores and can improve performance (default 0 i.e. disabled)
* -repeat [int]   : Execute the denoiser N times. Useful for profiling.
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
