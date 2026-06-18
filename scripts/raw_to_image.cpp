// Convert simulator raw RGB or grayscale bytes to PNG/JPEG.
//
// Build:
//   make raw_to_image_cpp

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace {

constexpr int kDefaultWidth = 1920;
constexpr int kDefaultHeight = 1080;
constexpr int kDefaultJpegQuality = 95;

enum class RawMode {
    Auto,
    Rgb,
    Gray,
};

struct Args {
    std::filesystem::path input;
    std::filesystem::path output;
    int width = kDefaultWidth;
    int height = kDefaultHeight;
    int quality = kDefaultJpegQuality;
    RawMode mode = RawMode::Auto;
};

void print_usage(const char* program)
{
    std::cerr
        << "Usage: " << program << " [--width WIDTH] [--height HEIGHT] "
        << "[--mode auto|rgb|gray] [--quality QUALITY] INPUT OUTPUT\n\n"
        << "Convert headerless raw image bytes from the SystemC RGB-to-gray "
        << "platform into a normal image file.\n\n"
        << "Arguments:\n"
        << "  INPUT             Source .raw file\n"
        << "  OUTPUT            Destination image, for example output.png\n\n"
        << "Options:\n"
        << "  --width WIDTH     Input width in pixels, default 1920\n"
        << "  --height HEIGHT   Input height in pixels, default 1080\n"
        << "  --mode MODE       Raw byte layout: auto, rgb, or gray; default auto\n"
        << "  --quality QUALITY JPEG quality for .jpg/.jpeg output, default 95\n"
        << "  -h, --help        Show this help message\n";
}

int parse_int_in_range(
    const std::string& option,
    const std::string& value,
    int minimum,
    int maximum)
{
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (*value.c_str() == '\0' || *end != '\0' || parsed < minimum || parsed > maximum) {
        throw std::runtime_error(
            option + " must be an integer from " + std::to_string(minimum) +
            " to " + std::to_string(maximum));
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

RawMode parse_mode(const std::string& value)
{
    if (value == "auto") {
        return RawMode::Auto;
    }
    if (value == "rgb") {
        return RawMode::Rgb;
    }
    if (value == "gray") {
        return RawMode::Gray;
    }
    throw std::runtime_error("--mode must be auto, rgb, or gray");
}

const char* mode_name(RawMode mode)
{
    switch (mode) {
    case RawMode::Auto:
        return "auto";
    case RawMode::Rgb:
        return "rgb";
    case RawMode::Gray:
        return "gray";
    }
    return "unknown";
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
        if (consume_value_arg(arg, "--width", argc, argv, i, value)) {
            args.width = parse_int_in_range("--width", value, 1, std::numeric_limits<int>::max());
        }
        else if (consume_value_arg(arg, "--height", argc, argv, i, value)) {
            args.height = parse_int_in_range("--height", value, 1, std::numeric_limits<int>::max());
        }
        else if (consume_value_arg(arg, "--mode", argc, argv, i, value)) {
            args.mode = parse_mode(value);
        }
        else if (consume_value_arg(arg, "--quality", argc, argv, i, value)) {
            args.quality = parse_int_in_range("--quality", value, 1, 100);
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

std::vector<unsigned char> read_file(const std::filesystem::path& input)
{
    std::ifstream file(input, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("cannot open input file: " + input.string());
    }

    const std::streamsize size = file.tellg();
    if (size < 0) {
        throw std::runtime_error("cannot determine input file size: " + input.string());
    }

    std::vector<unsigned char> bytes(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        file.read(reinterpret_cast<char*>(bytes.data()), size);
        if (!file) {
            throw std::runtime_error("failed while reading input file: " + input.string());
        }
    }
    return bytes;
}

RawMode infer_mode(size_t data_len, int width, int height, RawMode requested_mode)
{
    const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
    const size_t gray_size = pixel_count;
    const size_t rgb_size = pixel_count * 3;

    if (requested_mode == RawMode::Auto) {
        if (data_len == gray_size) {
            return RawMode::Gray;
        }
        if (data_len == rgb_size) {
            return RawMode::Rgb;
        }
        throw std::runtime_error(
            "cannot infer raw mode from " + std::to_string(data_len) +
            " bytes. Expected " + std::to_string(gray_size) +
            " bytes for gray or " + std::to_string(rgb_size) + " bytes for RGB.");
    }

    const size_t expected = requested_mode == RawMode::Rgb ? rgb_size : gray_size;
    if (data_len != expected) {
        throw std::runtime_error(
            std::string(mode_name(requested_mode)) + " raw for " + std::to_string(width) +
            "x" + std::to_string(height) + " must be " + std::to_string(expected) +
            " bytes, but " + std::to_string(data_len) + " bytes were read.");
    }

    return requested_mode;
}

std::string lowercase_extension(const std::filesystem::path& path)
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return ext;
}

void ensure_parent_directory(const std::filesystem::path& output)
{
    const std::filesystem::path parent = output.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

void write_image(
    const std::filesystem::path& output,
    const std::vector<unsigned char>& raw_bytes,
    int width,
    int height,
    RawMode mode,
    int quality)
{
    ensure_parent_directory(output);

    const int channels = mode == RawMode::Rgb ? 3 : 1;
    const int stride = width * channels;
    const std::string ext = lowercase_extension(output);
    int ok = 0;

    if (ext == ".png") {
        ok = stbi_write_png(output.string().c_str(), width, height, channels, raw_bytes.data(), stride);
    }
    else if (ext == ".jpg" || ext == ".jpeg") {
        ok = stbi_write_jpg(output.string().c_str(), width, height, channels, raw_bytes.data(), quality);
    }
    else if (ext == ".bmp") {
        ok = stbi_write_bmp(output.string().c_str(), width, height, channels, raw_bytes.data());
    }
    else if (ext == ".tga") {
        ok = stbi_write_tga(output.string().c_str(), width, height, channels, raw_bytes.data());
    }
    else {
        throw std::runtime_error("unsupported output extension: " + ext + " (use .png, .jpg, .bmp, or .tga)");
    }

    if (ok == 0) {
        throw std::runtime_error("failed to write output image: " + output.string());
    }
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const Args args = parse_args(argc, argv);
        const std::vector<unsigned char> raw_bytes = read_file(args.input);
        const RawMode mode = infer_mode(raw_bytes.size(), args.width, args.height, args.mode);

        write_image(args.output, raw_bytes, args.width, args.height, mode, args.quality);

        std::cout << "Wrote " << args.output.string() << " from " << raw_bytes.size()
                  << " bytes (" << args.width << "x" << args.height << ", "
                  << mode_name(mode) << ")\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
}
