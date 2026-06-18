// Convert a PNG/JPEG image to the simulator's headerless RGB888 raw format.
//
// Build:
//   make image_to_raw_cpp

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

namespace {

constexpr int kDefaultWidth = 1920;
constexpr int kDefaultHeight = 1080;
constexpr int kRgbChannels = 3;

struct Args {
    std::filesystem::path input;
    std::filesystem::path output;
    int width = kDefaultWidth;
    int height = kDefaultHeight;
    bool resize = false;
};

void print_usage(const char* program)
{
    std::cerr
        << "Usage: " << program << " [--width WIDTH] [--height HEIGHT] [--resize] "
        << "INPUT OUTPUT\n\n"
        << "Convert PNG/JPEG/etc. to headerless 24-bit RGB raw bytes for the "
        << "SystemC RGB-to-gray platform.\n\n"
        << "Arguments:\n"
        << "  INPUT           Source image, for example pictures/jpg/input.jpg\n"
        << "  OUTPUT          Destination .raw file\n\n"
        << "Options:\n"
        << "  --width WIDTH   Output width in pixels, default 1920\n"
        << "  --height HEIGHT Output height in pixels, default 1080\n"
        << "  --resize        Resize to --width x --height instead of rejecting mismatches\n"
        << "  -h, --help      Show this help message\n";
}

int parse_positive_int(const std::string& option, const std::string& value)
{
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (*value.c_str() == '\0' || *end != '\0' || parsed <= 0 ||
        parsed > std::numeric_limits<int>::max()) {
        throw std::runtime_error(option + " must be a positive integer");
    }
    return static_cast<int>(parsed);
}

bool consume_value_arg(
    const std::string& arg,
    const std::string& option,
    int argc,
    char** argv,
    int& index,
    std::string& value)
{
    const std::string prefix = option + "=";
    if (arg == option) {
        if (index + 1 >= argc) {
            throw std::runtime_error(option + " requires a value");
        }
        value = argv[++index];
        return true;
    }
    if (arg.rfind(prefix, 0) == 0) {
        value = arg.substr(prefix.size());
        return true;
    }
    return false;
}

Args parse_args(int argc, char** argv)
{
    Args args;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value;

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(EXIT_SUCCESS);
        }
        if (arg == "--resize") {
            args.resize = true;
        }
        else if (consume_value_arg(arg, "--width", argc, argv, i, value)) {
            args.width = parse_positive_int("--width", value);
        }
        else if (consume_value_arg(arg, "--height", argc, argv, i, value)) {
            args.height = parse_positive_int("--height", value);
        }
        else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("unknown option: " + arg);
        }
        else {
            positional.push_back(arg);
        }
    }

    if (positional.size() != 2) {
        throw std::runtime_error("expected INPUT and OUTPUT paths");
    }

    args.input = positional[0];
    args.output = positional[1];
    return args;
}

void write_file(const std::filesystem::path& output, const std::vector<unsigned char>& bytes)
{
    const std::filesystem::path parent = output.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream file(output, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot create output file: " + output.string());
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        throw std::runtime_error("failed while writing output file: " + output.string());
    }
}

std::vector<unsigned char> resize_rgb(
    const unsigned char* input,
    int input_width,
    int input_height,
    int output_width,
    int output_height)
{
    std::vector<unsigned char> output(
        static_cast<size_t>(output_width) * static_cast<size_t>(output_height) * kRgbChannels);

    unsigned char* resized = stbir_resize_uint8_linear(
        input,
        input_width,
        input_height,
        0,
        output.data(),
        output_width,
        output_height,
        0,
        STBIR_RGB);

    if (resized == nullptr) {
        throw std::runtime_error("stb_image_resize2 failed to resize the image");
    }

    return output;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const Args args = parse_args(argc, argv);

        int input_width = 0;
        int input_height = 0;
        int original_channels = 0;
        std::unique_ptr<unsigned char, decltype(&stbi_image_free)> image(
            stbi_load(
                args.input.string().c_str(),
                &input_width,
                &input_height,
                &original_channels,
                kRgbChannels),
            stbi_image_free);

        if (image == nullptr) {
            throw std::runtime_error(
                "cannot load " + args.input.string() + ": " + stbi_failure_reason());
        }

        std::vector<unsigned char> raw_bytes;
        if (input_width != args.width || input_height != args.height) {
            if (!args.resize) {
                throw std::runtime_error(
                    args.input.string() + " is " + std::to_string(input_width) + "x" +
                    std::to_string(input_height) + ", expected " + std::to_string(args.width) +
                    "x" + std::to_string(args.height) + ". Use --resize to scale it.");
            }

            raw_bytes = resize_rgb(image.get(), input_width, input_height, args.width, args.height);
        }
        else {
            const size_t byte_count =
                static_cast<size_t>(args.width) * static_cast<size_t>(args.height) * kRgbChannels;
            raw_bytes.assign(image.get(), image.get() + byte_count);
        }

        write_file(args.output, raw_bytes);

        std::cout << "Wrote " << raw_bytes.size() << " bytes to " << args.output.string()
                  << " (" << args.width << "x" << args.height << ", RGB888)\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
}
