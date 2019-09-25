
#include <OpenImageDenoise/oidn.hpp>
#include <iostream>
#include <OpenImageIO\imageio.h>
#include <OpenImageIO\imagebuf.h>
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
#define DENOISER_MINOR_VERSION 2

// Our global image handles
OIIO::ImageBuf* input_beauty = nullptr;
OIIO::ImageBuf* input_albedo = nullptr;
OIIO::ImageBuf* input_normal = nullptr;

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
            std::cout<<"terminating..."<<std::endl;
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
                case(1):  memcpy(out_ptr, in_ptr, 1 * sizeof(float));return true;
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

void printParams()
{
    std::cout<<"Command line parameters"<<std::endl;
    std::cout<<"-i [string]     : path to input image"<<std::endl;
    std::cout<<"-o [string]     : path to output image"<<std::endl;
    std::cout<<"-a [string]     : path to input albedo AOV (optional)"<<std::endl;
    std::cout<<"-n [string]     : path to input normal AOV (optional, requires albedo AOV)"<<std::endl;
    std::cout<<"-hdr [int]      : Image is a HDR image. Disabling with will assume the image is in sRGB (default 1 i.e. enabled)"<<std::endl;
    std::cout<<"-t [int]        : number of threads to use (defualt is all)"<<std::endl;
    std::cout<<"-affinity [int] : Enable affinity. This pins vertual threads to physical cores and can improve performance (default 0 i.e. disabled)"<<std::endl;
    std::cout<<"-repeat [int]   : Execute the denoiser N times. Useful for profiling."<<std::endl;
    std::cout<<"-maxmem [int]   : Maximum memory size used by the denoiser in MB"<<std::endl;
}

int main(int argc, char *argv[])
{
    std::cout<<"Launching OIDN AI Denoiser command line app v"<<DENOISER_MAJOR_VERSION<<"."<<DENOISER_MINOR_VERSION<<std::endl;
    std::cout<<"Created by Declan Russell (01/03/2019)"<<std::endl;

    bool b_loaded, n_loaded, a_loaded;
    b_loaded = n_loaded = a_loaded = false;

    // Pass our command line args
    std::string out_path;
    bool affinity = false;
    bool hdr = true;
    unsigned int num_runs = 1;
    int num_threads = 0;
    int maxmem = -1;
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
            std::cout<<"Input image: "<<path<<std::endl;
            input_beauty = new OIIO::ImageBuf(path);
            if (input_beauty->init_spec(path, 0, 0))
            {
                std::cout<<"Loaded successfully"<<std::endl;
                b_loaded = true;
            }
            else
            {
                std::cout<<"Failed to load input image"<<std::endl;
                std::cout<<"[OIIO]: "<<input_beauty->geterror()<<std::endl;
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
        }
        else if (arg == "-n")
        {
            i++;
            std::string path( argv[i] );
            std::cout<<"Normal image: "<<path<<std::endl;
            input_normal = new OIIO::ImageBuf(path);
            if (input_normal->init_spec(path, 0, 0))
            {
                std::cout<<"Loaded successfully"<<std::endl;
                n_loaded = true;
            }
            else
            {
                std::cout<<"Failed to load normal image"<<std::endl;
                std::cout<<"[OIIO]: "<<input_normal->geterror()<<std::endl;
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
        }
        else if (arg == "-a")
        {
            i++;
            std::string path( argv[i] );
            std::cout<<"Albedo image: "<<path<<std::endl;
            input_albedo = new OIIO::ImageBuf(path);
            if (input_albedo->init_spec(path, 0, 0))
            {
                std::cout<<"Loaded successfully"<<std::endl;
                a_loaded = true;
            }
            else
            {
                std::cout<<"Failed to load albedo image"<<std::endl;
                std::cout<<"[OIIO]: "<<input_albedo->geterror()<<std::endl;
                cleanup();
                exitfunc(EXIT_FAILURE);
            }
        }
        else if(arg == "-o")
        {
            i++;
            out_path = std::string( argv[i] );
            std::cout<<"Output image: "<<out_path<<std::endl;
        }
        else if (arg == "-affinity")
        {
            i++;
            std::string affinity_string( argv[i] );
            affinity = std::stoi(affinity_string);
            std::cout<<((affinity) ? "Affinity enabled" : "Affinity disabled")<<std::endl;
        }
        else if (arg == "-hdr")
        {
            i++;
            std::string hdr_string( argv[i] );
            hdr = bool(std::stoi(hdr_string));
            std::cout<<((hdr) ? "HDR training data enabled" : "HDR training data disabled")<<std::endl;
        }
        else if (arg == "-t")
        {
            i++;
            std::string num_threads_string( argv[i] );
            num_threads = std::stoi(num_threads_string);
            std::cout<<"Number of threads set to "<<num_threads<<std::endl;
        }
        else if (arg == "-repeat")
        {
            i++;
            std::string repeat_string( argv[i] );
            num_runs = std::max(std::stoi(repeat_string), 1);
            std::cout<<"Number of repeats set to "<<num_runs<<std::endl;
        }
        else if (arg == "-maxmem")
        {
            i++;
            std::string maxmem_string( argv[i] );
            maxmem = float(std::stoi(maxmem_string));
            std::cout<<"Maximum denoiser memory set to "<<maxmem<<"MB"<<std::endl;
        }
        else if (arg == "-h" || arg == "--help")
        {
            printParams();
        }
    }

    // Check if a beauty has been loaded
    if (!b_loaded)
    {
        std::cerr<<"No input image could be loaded"<<std::endl;
        cleanup();
        exitfunc(EXIT_FAILURE);
    }

    // If a normal AOV is loaded then we also require an albedo AOV
    if (n_loaded && !a_loaded)
    {
        std::cerr<<"You cannot use a normal AOV without an albedo"<<std::endl;
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
        std::cerr<<"No output file extension"<<std::endl;
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
            std::cerr<<"Aldedo image not same resolution as beauty"<<std::endl;
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
            std::cerr<<"Normal image not same resolution as beauty"<<std::endl;
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
            std::cerr<<"Failed to convert beauty to float3"<<std::endl;
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
                std::cerr<<"Failed to convert albedo to float3"<<std::endl;
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
                std::cerr<<"Failed to convert normal to float3"<<std::endl;
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
        std::cout<<"Initializing OIDN"<<std::endl;
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

        std::cout<< "Using OIDN version "<<versionMajor<<"."<<versionMinor<<"."<<versionPatch<<std::endl;

        oidn::FilterRef filter = device.newFilter("RT");

        // Set our filter paramaters

        // Set our the filter images
        filter.setImage("color", (void*)&beauty_pixels_float3[0], oidn::Format::Float3, b_width, b_height);
        if (a_loaded)
            filter.setImage("albedo", (void*)&albedo_pixels[0], oidn::Format::Float3, a_width, a_height);
        if (n_loaded)
            filter.setImage("normal", (void*)&normal_pixels[0], oidn::Format::Float3, n_width, n_height);
        filter.setImage("output", (void*)&output_pixels[0], oidn::Format::Float3, b_width, b_height);

        filter.set("hdr", hdr);
        if (maxmem >= 0)
            filter.set("maxMemoryMB", maxmem);

        // Commit changes to the filter
        filter.commit();

        // Execute denoise
        int sum = 0;
        for (unsigned int i = 0; i < num_runs; i++)
        {
            std::cout<<"Denoising..."<<std::endl;
            clock_t start = clock(), diff;
            filter.execute();
            diff = clock() - start;
            int msec = diff * 1000 / CLOCKS_PER_SEC;
            if (num_runs > 1)
                std::cout<<"Denoising run "<<i<<" complete in "<<msec/1000<<"."<<std::setfill('0')<<std::setw(3)<<msec%1000<<" seconds"<<std::endl;
            else
                std::cout<<"Denoising complete in "<<msec/1000<<"."<<std::setfill('0')<<std::setw(3)<<msec%1000<<" seconds"<<std::endl;
            sum += msec;
        }
        if (num_runs > 1)
        {
            sum /= num_runs;
            std::cout<<"Denoising avg of "<<num_runs<<" complete in "<<sum/1000<<"."<<std::setfill('0')<<std::setw(3)<<sum%1000<<" seconds"<<std::endl;    
        }

    }
    catch (const std::exception& e)
    {
        std::cerr<<"[OIDN]: "<<e.what()<<std::endl;
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
            std::cerr<<"Failed to convert output to original format"<<std::endl;
            cleanup();
            exitfunc(EXIT_FAILURE);
        }
        in += 3;
        out += beauty_roi.nchannels();
    }

    // Set our OIIO pixels
    if (!input_beauty->set_pixels(beauty_roi, OIIO::TypeDesc::FLOAT, &beauty_pixels[0]))
        std::cerr<<"Something went wrong setting pixels"<<std::endl;

    // Save the output image
    std::cout<<"Saving to: "<<out_path<<std::endl;
    if (input_beauty->write(out_path))
        std::cout<<"Done!"<<std::endl;
    else
    {
        std::cerr<<"Could not save file "<<out_path<<std::endl;
        std::cerr<<"[OIIO]: "<<input_beauty->geterror()<<std::endl;
    }

    cleanup();
    exitfunc(EXIT_SUCCESS);
}

