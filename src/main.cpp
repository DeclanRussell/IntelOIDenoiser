
#include <OpenImageDenoise/oidn.hpp>
#include <iostream>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <stdio.h>
#include <exception>
#include <time.h>
#ifdef _WIN32
#include <thread>
#include <chrono>
#include <windows.h>
#include <winternl.h>
#endif

#define DENOISER_MAJOR_VERSION 1
#define DENOISER_MINOR_VERSION 7

// Our global image handles
OIIO::ImageBuf* input_beauty = nullptr;
OIIO::ImageBuf* input_albedo = nullptr;
OIIO::ImageBuf* input_normal = nullptr;

// Logging verbosity level
int verbosity = 2;

// Application start time
std::chrono::high_resolution_clock::time_point app_start_time;

std::string getTime()
{
    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - app_start_time;
    double milliseconds = time_span.count();
    int seconds = floor(milliseconds / 1000.0);
    int minutes = floor((float(seconds) / 60.f));
    milliseconds -= seconds * 1000.0;
    seconds -= minutes * 60;
    char s[10];
    sprintf(s, "%02d:%02d:%03d", minutes, seconds, (int)milliseconds);
    return std::string(s);
}

template<typename... Args>
void PrintInfo(const char *c, Args... args)
{
    if (!verbosity)
        return;
    char buffer[256];
    sprintf(buffer, c, args...);
    std::cout<<getTime()<<"       | "<<buffer<<std::endl;
}

template<typename... Args>
void PrintError(const char *c, Args... args)
{
    char buffer[256];
    sprintf(buffer, c, args...);
    std::cerr<<getTime()<<" ERROR | "<<buffer<<std::endl;
}

#ifdef _WIN32
int getSysOpType()
{
    int ret = 0;
    NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
    OSVERSIONINFOEXW osInfo;

    *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

    if (NULL != RtlGetVersion)
    {
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        RtlGetVersion(&osInfo);
        ret = osInfo.dwMajorVersion;
    }
    return ret;
}
#endif

void exitfunc(int exit_code)
{
#ifdef _WIN32
    if (getSysOpType() < 10)
    {
        HANDLE tmpHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId());
        if (tmpHandle != NULL)
        {
            PrintInfo("terminating...");
            std::this_thread::sleep_for(std::chrono::seconds(1)); // delay 1s
            TerminateProcess(tmpHandle, 0);
        }
    }
#endif
	exit(exit_code);
}

void cleanup()
{
    if (input_beauty) delete input_beauty;
    if (input_albedo) delete input_albedo;
    if (input_normal) delete input_normal;
}

bool convertToFormat(void* in_ptr, void* out_ptr, unsigned int in_channels, unsigned int out_channels)
{
    switch (in_channels)
    {
        case(1):
        {
            switch (out_channels)
            {
                case(1):
                case(2):
                case(3):
                case(4): memcpy(out_ptr, in_ptr, sizeof(float)); return true;
                default: return false; // How has this happened?
            }
        }
        case(2):
        {
            switch (out_channels)
            {
                case(1): memcpy(out_ptr, in_ptr, sizeof(float)); return true;
                case(2):
                case(3):
                case(4): memcpy(out_ptr, in_ptr, 2 * sizeof(float));return true;
                default: return false; // How has this happened?
            }
        }
        case(3):
        {
            switch (out_channels)
            {
                case(1): memcpy(out_ptr, in_ptr, 1 * sizeof(float));return true;
                case(2): memcpy(out_ptr, in_ptr, 2 * sizeof(float)); return true;
                case(3):
                case(4): memcpy(out_ptr, in_ptr, 3 * sizeof(float)); return true;
                default: return false; // How has this happened?
            }
        }
        case(4):
        {
            switch (out_channels)
            {
                case(1): memcpy(out_ptr, in_ptr, 1 * sizeof(float)); return true;
                case(2): memcpy(out_ptr, in_ptr, 2 * sizeof(float)); return true;
                case(3): memcpy(out_ptr, in_ptr, 3 * sizeof(float)); return true;
                case(4): memcpy(out_ptr, in_ptr, 4 * sizeof(float)); return true;
                default: return false; // How has this happened?
            }
        }
        default: return false; // How has this happened?
    }
    return false; // some unsupported conversion
}

void errorCallback(void* userPtr, oidn::Error error, const char* message)
{
    throw std::runtime_error(message);
}

bool progressCallback(void* userPtr, double n)
{
    if (verbosity >= 2)
        PrintInfo("%d%% complete", (int)(n*100.0));
    return true;
}

void printParams()
{
    // Always print parameters if needed
    int old_verbosity = verbosity;
    verbosity = 1;
    PrintInfo("Command line parameters");
    PrintInfo("-v [int]        : log verbosity level 0:disabled 1:simple 2:full (default 2)");
    PrintInfo("-i [string]     : path to input image");
    PrintInfo("-o [string]     : path to output image");
    PrintInfo("-a [string]     : path to input albedo AOV (optional)");
    PrintInfo("-n [string]     : path to input normal AOV (optional, requires albedo AOV)");
    PrintInfo("-hdr [int]      : Image is a HDR image. Disabling with will assume the image is in sRGB (default 1 i.e. enabled)");
    PrintInfo("-srgb [int]     : whether the main input image is encoded with the sRGB (or 2.2 gamma) curve (LDR only) or is linear (default 0 i.e. disabled)");
    PrintInfo("-t [int]        : number of threads to use (defualt is all)");
    PrintInfo("-affinity [int] : Enable affinity. This pins virtual threads to physical cores and can improve performance (default 0 i.e. disabled)");
    PrintInfo("-repeat [int]   : Execute the denoiser N times. Useful for profiling.");
    PrintInfo("-maxmem [int]   : Maximum memory size used by the denoiser in MB");
    PrintInfo("-clean_aux [int]: Whether the auxiliary feature (albedo, normal) images are noise-free; recommended for highest quality but should *not* be enabled for noisy auxiliary images to avoid residual noise (default 0 i.e. disabled)");
    verbosity = old_verbosity;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        for (int i=1; i<argc; i++)
        {
            if (strcmp(argv[i], "-v"))
                continue;
            i++;
            if (i >= argc)
            {
               PrintError("incorrect number of arguments for flag -v");
            }
            verbosity = std::stoi(std::string ( argv[i] ));
            break;
        }
    }
    app_start_time = std::chrono::high_resolution_clock::now();
    PrintInfo("Launching OIDN AI Denoiser command line app v%d.%d", DENOISER_MAJOR_VERSION, DENOISER_MINOR_VERSION);
    PrintInfo("Created by Declan Russell (01/03/2019)");

    bool b_loaded, n_loaded, a_loaded;
    b_loaded = n_loaded = a_loaded = false;

    // Pass our command line args
    std::string out_path;
    bool affinity = false;
    bool hdr = true;
    bool srgb = false;
    unsigned int num_runs = 1;
    int num_threads = 0;
    int maxmem = -1;
    bool clean_aux = false;
    if (argc == 1)
    {
        printParams();
        exitfunc(EXIT_SUCCESS);
    }
    for (int i=1; i<argc; i++)
    {
        const std::string arg( argv[i] );
        if (arg == "-i")
        {
            i++;
            std::string path( argv[i] );
            if (verbosity >= 2)
                PrintInfo("Input image: %s", path.c_str());
            input_beauty = new OIIO::ImageBuf(path);
            if (input_beauty->init_spec(path, 0, 0))
            {
                if (verbosity >= 2)
                    PrintInfo("Loaded successfully");
                b_loaded = true;
            }
            else
            {
                PrintError("Failed to load input image");
                PrintError("[OIIO]: %s", input_beauty->geterror().c_str());
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
        }
        else if (arg == "-n")
        {
            i++;
            std::string path( argv[i] );
            if (verbosity >= 2)
                PrintInfo("Normal image: %s", path.c_str());
            input_normal = new OIIO::ImageBuf(path);
            if (input_normal->init_spec(path, 0, 0))
            {
                if (verbosity >= 2)
                    PrintInfo("Loaded successfully");
                n_loaded = true;
            }
            else
            {
                PrintError("Failed to load normal image");
                PrintError("[OIIO]: %s", input_normal->geterror().c_str());
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
        }
        else if (arg == "-a")
        {
            i++;
            std::string path( argv[i] );
            if (verbosity >= 2)
                PrintInfo("Albedo image: %s", path.c_str());
            input_albedo = new OIIO::ImageBuf(path);
            if (input_albedo->init_spec(path, 0, 0))
            {
                if (verbosity >= 2)
                    PrintInfo("Loaded successfully");
                a_loaded = true;
            }
            else
            {
                PrintError("Failed to load albedo image");
                PrintError("[OIIO]: %s", input_albedo->geterror().c_str());
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
        }
        else if(arg == "-o")
        {
            i++;
            out_path = std::string( argv[i] );
            if (verbosity >= 2)
                PrintInfo("Output image: %s", out_path.c_str());
        }
        else if (arg == "-affinity")
        {
            i++;
            std::string affinity_string( argv[i] );
            affinity = std::stoi(affinity_string);
            if (verbosity >= 2)
                PrintInfo((affinity) ? "Affinity enabled" : "Affinity disabled");
        }
        else if (arg == "-hdr")
        {
            i++;
            std::string hdr_string( argv[i] );
            hdr = bool(std::stoi(hdr_string));
            if (verbosity >= 2)
                PrintInfo((hdr) ? "HDR training data enabled" : "HDR training data disabled");
            if (!hdr)
            {
                PrintInfo("Enabling sRGB mode due to LDR");
                srgb = true;
            }
        }
        else if (arg == "-srgb")
        {
            i++;
            std::string srgb_string( argv[i] );
            srgb = bool(std::stoi(srgb_string));
            if (verbosity >= 2)
                PrintInfo((srgb) ? "sRGB mode enabled" : "sRGB mode disabled");
        }
        else if (arg == "-t")
        {
            i++;
            std::string num_threads_string( argv[i] );
            num_threads = std::stoi(num_threads_string);
            if (verbosity >= 2)
                PrintInfo("Number of threads set to %d", num_threads);
        }
        else if (arg == "-repeat")
        {
            i++;
            std::string repeat_string( argv[i] );
            num_runs = std::max(std::stoi(repeat_string), 1);
            if (verbosity >= 2)
                PrintInfo("Number of repeats set to %d", num_runs);
        }
        else if (arg == "-maxmem")
        {
            i++;
            std::string maxmem_string( argv[i] );
            maxmem = float(std::stoi(maxmem_string));
            if (verbosity >= 2)
                PrintInfo("Maximum denoiser memory set to %dMB", maxmem);
        }
        else if (arg == "-clean_aux")
        {
            i++;
            std::string clean_aux_string( argv[i] );
            clean_aux = bool(std::stoi(clean_aux_string));
            if (verbosity >= 2)
                PrintInfo((clean_aux) ? "cleanAux enabled" : "cleanAux disabled");
        }
        else if (arg == "-h" || arg == "--help")
        {
            printParams();
        }
    }

    if (verbosity && srgb && hdr)
    {
        PrintInfo("Disbaling sRGB, incompatble with HDR input");
        srgb = false;
    }

    // Check if a beauty has been loaded
    if (!b_loaded)
    {
        PrintError("No input image could be loaded");
        cleanup();
        exitfunc(EXIT_FAILURE);
    }

    // If a normal AOV is loaded then we also require an albedo AOV
    if (n_loaded && !a_loaded)
    {
        PrintError("You cannot use a normal AOV without an albedo");
        cleanup();
        exitfunc(EXIT_FAILURE);
    }

    // Check for a file extension
    int x = (int)out_path.find_last_of(".");
    x++;
    const char* ext_c = out_path.c_str()+x;
    std::string ext(ext_c);
    if (!ext.size())
    {
        PrintError("No output file extension");
        cleanup();
        exitfunc(EXIT_FAILURE);
    }

    OIIO::ROI beauty_roi, albedo_roi, normal_roi;
    beauty_roi = OIIO::get_roi_full(input_beauty->spec());
    int b_width = beauty_roi.width();
    int b_height = beauty_roi.height();
    if (a_loaded)
    {
        albedo_roi = OIIO::get_roi_full(input_albedo->spec());
        if (n_loaded)
            normal_roi = OIIO::get_roi_full(input_normal->spec());
    }

    // Check that our feature buffers are the same resolution as our beauty
    int a_width = (a_loaded) ? albedo_roi.width() : 0;
    int a_height = (a_loaded) ? albedo_roi.height() : 0;
    if (a_loaded)
    {
        if (a_width != b_width || a_height != b_height)
        {
            PrintError("Aldedo image not same resolution as beauty");
            cleanup();
            exitfunc(EXIT_FAILURE);
        }
    }

    int n_width = (n_loaded) ? normal_roi.width() : 0;
    int n_height = (n_loaded) ? normal_roi.height() : 0;
    if (n_loaded)
    {
        if (n_width != b_width || n_height != b_height)
        {
            PrintError("Normal image not same resolution as beauty");
            cleanup();
            exitfunc(EXIT_FAILURE);
        }
    }

    // Create our output buffer
    std::vector<float> output_pixels(b_width * b_height * 3);

    // Get our pixel data
    std::vector<float> beauty_pixels(b_width * b_height * beauty_roi.nchannels());
    input_beauty->get_pixels(beauty_roi, OIIO::TypeDesc::FLOAT, &beauty_pixels[0]);
    std::vector<float> beauty_pixels_float3(b_width * b_height * 3);
    // Convert buffer to float3
    float* in = (float*)beauty_pixels.data();
    float* out = (float*)beauty_pixels_float3.data();
    for (unsigned int i = 0; i < beauty_pixels.size(); i+=beauty_roi.nchannels())
    {
        if (!convertToFormat(in, out, beauty_roi.nchannels(), 3))
        {
            PrintError("Failed to convert beauty to float3");
            cleanup();
            exitfunc(EXIT_FAILURE);
        }
        in += beauty_roi.nchannels();
        out += 3;
    }


    std::vector<float> albedo_pixels;
    if (a_loaded)
    {
        std::vector<float> albedo_pixels_temp(a_width * a_height * albedo_roi.nchannels());
        input_albedo->get_pixels(albedo_roi, OIIO::TypeDesc::FLOAT, &albedo_pixels_temp[0]);
        albedo_pixels.resize(a_width * a_height * 3);
        in = (float*)albedo_pixels_temp.data();
        out = (float*)albedo_pixels.data();
        // Convert buffer to float3
        for (unsigned int i = 0; i < albedo_pixels_temp.size(); i+=albedo_roi.nchannels())
        {
            if (!convertToFormat(in, out, albedo_roi.nchannels(), 3))
            {
                PrintError("Failed to convert albedo to float3");
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
            in += albedo_roi.nchannels();
            out += 3;
        }
    }

    std::vector<float> normal_pixels;
    if (n_loaded)
    {
        std::vector<float> normal_pixels_temp(n_width * n_height * normal_roi.nchannels());
        input_normal->get_pixels(normal_roi, OIIO::TypeDesc::FLOAT, &normal_pixels_temp[0]);
        normal_pixels.resize(n_width * n_height * 3);
        in = (float*)normal_pixels_temp.data();
        out = (float*)normal_pixels.data();
        // Convert buffer to float3
        for (unsigned int i = 0; i < normal_pixels_temp.size(); i+=normal_roi.nchannels())
        {
            if (!convertToFormat(in, out, normal_roi.nchannels(), 3))
            {
                PrintError("Failed to convert normal to float3");
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
            in += normal_roi.nchannels();
            out += 3;
        }
    }

    // Catch exceptions
    try
    {
        PrintInfo("Initializing OIDN");
        // Create our device
        oidn::DeviceRef device = oidn::newDevice();
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
           throw std::runtime_error(errorMessage);
        device.setErrorFunction(errorCallback);
        // Set our device parameters
        if (num_threads)
            device.set("numThreads", num_threads);
        if (affinity)
            device.set("setAffinity", affinity);

        // Commit the changes to the device
        device.commit();
        const int versionMajor = device.get<int>("versionMajor");
        const int versionMinor = device.get<int>("versionMinor");
        const int versionPatch = device.get<int>("versionPatch");

        PrintInfo("Using OIDN version %d.%d.%d", versionMajor, versionMinor, versionPatch);

        // Create the AI filter
        oidn::FilterRef filter = device.newFilter("RT");

        // Set our progress callback
        filter.setProgressMonitorFunction((oidn::ProgressMonitorFunction)progressCallback);

        // Set our filter paramaters

        // Set our the filter images
        filter.setImage("color", (void*)&beauty_pixels_float3[0], oidn::Format::Float3, b_width, b_height);
        if (a_loaded)
            filter.setImage("albedo", (void*)&albedo_pixels[0], oidn::Format::Float3, a_width, a_height);
        if (n_loaded)
            filter.setImage("normal", (void*)&normal_pixels[0], oidn::Format::Float3, n_width, n_height);
        filter.setImage("output", (void*)&output_pixels[0], oidn::Format::Float3, b_width, b_height);

        filter.set("hdr", hdr);
        filter.set("srgb", srgb);
        if (maxmem >= 0)
            filter.set("maxMemoryMB", maxmem);
        filter.set("cleanAux", clean_aux);

        // Commit changes to the filter
        filter.commit();

        // Execute denoise
        int sum = 0;
        for (unsigned int i = 0; i < num_runs; i++)
        {
            PrintInfo("Denoising...");
            clock_t start = clock(), diff;
            filter.execute();
            diff = clock() - start;
            int msec = diff * 1000 / CLOCKS_PER_SEC;
            if (num_runs > 1)
                PrintInfo("Denoising run %d complete in %d.%03d seconds", i+1, msec/1000, msec%1000);
            else
                PrintInfo("Denoising complete in %d.%03d seconds", msec/1000, msec%1000);
            sum += msec;
        }
        if (num_runs > 1)
        {
            sum /= num_runs;
            PrintInfo("Denoising avg of %d complete in %d.%03d seconds", num_runs, sum/1000, sum%1000);
        }

    }
    catch (const std::exception& e)
    {
        PrintError("[OIDN]: %s", e.what());
        cleanup();
        exitfunc(EXIT_FAILURE);
    }


    // If the image already exists delete it
    remove(out_path.c_str());

    // Convert the image back to the original format
    in = (float*)output_pixels.data();
    out = (float*)beauty_pixels.data();
    // Convert buffer to float3
    for (unsigned int i = 0; i < output_pixels.size(); i+=3)
    {
        if (!convertToFormat(in, out, 3, beauty_roi.nchannels()))
        {
            PrintError("Failed to convert output to original format");
            cleanup();
            exitfunc(EXIT_FAILURE);
        }
        in += 3;
        out += beauty_roi.nchannels();
    }

    // Set our OIIO pixels
    if (!input_beauty->set_pixels(beauty_roi, OIIO::TypeDesc::FLOAT, &beauty_pixels[0]))
        PrintError("Something went wrong setting pixels");

    // Save the output image
    PrintInfo("Saving to: %s", out_path.c_str());
    if (input_beauty->write(out_path))
        PrintInfo("Done!");
    else
    {
        PrintError("Could not save file %s", out_path.c_str());
        PrintError("[OIIO]: %s", input_beauty->geterror().c_str());
    }

    cleanup();
    exitfunc(EXIT_SUCCESS);
}

