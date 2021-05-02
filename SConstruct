
import os
PLATFORM = Platform()
DEBUG = ARGUMENTS.get("debug", 0)

CUDA_INCLUDE_PATH = os.environ['CUDA_PATH'] + "/include"

ENV = Environment(CPPPATH = ['.', "./contrib/oidn/include", "./contrib/OpenImageIO/include", CUDA_INCLUDE_PATH
],
                  CCFLAGS="-std=c++11 /EHsc")

# Used for debbugging
if int(DEBUG):
    ENV.Append(CCFLAGS=' -ggdb3')

SOURCES = Glob("src/*.cpp")

LIBPATH = []
LIBPATH.append("./contrib/oidn/lib")
LIBPATH.append("./contrib/OpenImageIO/lib")

LIBS = []
LIBS.append("OpenImageDenoise")
LIBS.append("OpenImageIO")

LINKFLAGS = []

if PLATFORM.name == "win32":
    ENV.Append(CPPDEFINES = "NOMINMAX")

# Copy over the OIDN dlls to the bin directory
ENV.Command("bin/OpenImageDenoise.dll", "./contrib/oidn/bin/OpenImageDenoise.dll", Copy("$TARGET", "$SOURCE"))
ENV.Command("bin/tbb12.dll", "./contrib/oidn/bin/tbb12.dll", Copy("$TARGET", "$SOURCE"))

# Copy all of OIIO many dependancies!
ENV.Command("bin/boost_atomic-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_atomic-vc141-mt-x64-1_67.dll", Copy("$TARGET", "$SOURCE"))
ENV.Command("bin/boost_chrono-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_chrono-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_container-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_container-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_context-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_context-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_coroutine-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_coroutine-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_date_time-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_date_time-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_filesystem-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_filesystem-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_math_c99-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_math_c99-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_math_c99f-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_math_c99f-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_math_c99l-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_math_c99l-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_math_tr1-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_math_tr1-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_math_tr1f-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_math_tr1f-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_math_tr1l-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_math_tr1l-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_random-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_random-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_regex-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_regex-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_system-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_system-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/boost_thread-vc141-mt-x64-1_67.dll", "./contrib/OpenImageIO/bin/boost_thread-vc141-mt-x64-1_67.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/Half.dll", "./contrib/OpenImageIO/bin/Half.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/Iex-2_2.dll", "./contrib/OpenImageIO/bin/Iex-2_2.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/IexMath-2_2.dll", "./contrib/OpenImageIO/bin/IexMath-2_2.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/IlmImf-2_2.dll", "./contrib/OpenImageIO/bin/IlmImf-2_2.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/IlmImfUtil-2_2.dll", "./contrib/OpenImageIO/bin/IlmImfUtil-2_2.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/IlmThread-2_2.dll", "./contrib/OpenImageIO/bin/IlmThread-2_2.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/Imath-2_2.dll", "./contrib/OpenImageIO/bin/Imath-2_2.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/jpeg62.dll", "./contrib/OpenImageIO/bin/jpeg62.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/libeay32.dll", "./contrib/OpenImageIO/bin/libeay32.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/libpng16.dll", "./contrib/OpenImageIO/bin/libpng16.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/lzma.dll", "./contrib/OpenImageIO/bin/lzma.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/OpenImageIO.dll", "./contrib/OpenImageIO/bin/OpenImageIO.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/OpenImageIO_Util.dll", "./contrib/OpenImageIO/bin/OpenImageIO_Util.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/ssleay32.dll", "./contrib/OpenImageIO/bin/ssleay32.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/tiff.dll", "./contrib/OpenImageIO/bin/tiff.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/tiffxx.dll", "./contrib/OpenImageIO/bin/tiffxx.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/turbojpeg.dll", "./contrib/OpenImageIO/bin/turbojpeg.dll", Copy("$TARGET","$SOURCE"))
ENV.Command("bin/zlib1.dll", "./contrib/OpenImageIO/bin/zlib1.dll", Copy("$TARGET","$SOURCE"))

program = ENV.Program(target="bin/Denoiser", source=SOURCES,
                              LIBPATH=LIBPATH, LIBS=LIBS, LINKFLAGS=LINKFLAGS)

