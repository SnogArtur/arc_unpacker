// SOTES image
//
// Company:   Lizsoft
// Engine:    FOR-D System
// Extension: - (embedded in executables)
// Archives:  .dll
//
// Known games:
// - Fortune Summoners: Secret Of The Elemental Stone (SOTES)

#include <cstring>
#include "formats/gfx/sotes_converter.h"
#include "formats/image.h"

namespace
{
    size_t guess_image_dimension(
        const std::vector<uint32_t> candidates,
        ssize_t main_delta,
        ssize_t max_delta_correction,
        size_t pixels_size)
    {
        for (auto &base : candidates)
        {
            for (ssize_t delta = 0; delta <= max_delta_correction; delta ++)
            {
                size_t possible_dimension = base + main_delta + delta;
                if (possible_dimension > pixels_size)
                    continue;
                if (pixels_size % possible_dimension == 0)
                    return possible_dimension;
            }
        }
        throw std::runtime_error("Cannot figure out the image dimensions");
    }

    void mirror(char *pixel_data, size_t pixel_data_size, size_t scanline_width)
    {
        size_t height = pixel_data_size / scanline_width;
        std::unique_ptr<char> old_line(new char[scanline_width]);
        for (size_t y = 0; y < height / 2; y ++)
        {
            memcpy(
                old_line.get(),
                &pixel_data[y * scanline_width],
                scanline_width);

            memcpy(
                &pixel_data[y * scanline_width],
                &pixel_data[(height - 1 - y) * scanline_width],
                scanline_width);

            memcpy(
                &pixel_data[(height - 1 - y) * scanline_width],
                old_line.get(),
                scanline_width);
        }
    }
}

void SotesConverter::decode_internal(VirtualFile &file) const
{
    if (file.io.size() < 1112)
        throw std::runtime_error("Not a SOTES image");

    uint32_t weird_data1[8];
    uint32_t palette[256];
    uint32_t weird_data2[14];
    file.io.read(weird_data1, 8 * 4);
    file.io.read(palette, 256 * 4);
    file.io.read(weird_data2, 14 * 4);

    size_t pixel_data_offset = weird_data2[12] - weird_data2[10];
    if (pixel_data_offset >= file.io.size())
        throw std::runtime_error("Not a SOTES image");
    file.io.skip(pixel_data_offset);

    size_t raw_data_size = file.io.size() - file.io.tell();

    size_t width = guess_image_dimension(
        std::vector<uint32_t>(&weird_data1[1], &weird_data1[5]),
        - weird_data1[6],
        3,
        raw_data_size);

    size_t height = guess_image_dimension(
        std::vector<uint32_t>(&weird_data2[0], &weird_data2[5]),
        - weird_data2[10],
        0,
        raw_data_size);

    bool use_palette = width * height * 3 != raw_data_size;

    size_t pixel_data_size;
    std::unique_ptr<char> pixel_data(nullptr);
    if (use_palette)
    {
        pixel_data_size = width * height * 3;
        pixel_data.reset(new char[pixel_data_size]);

        char *pixels_ptr = pixel_data.get();
        for (size_t i = 0; i < raw_data_size; i ++)
        {
            if (pixels_ptr >= pixel_data.get() + width * height * 3)
                throw std::runtime_error("Trying to write pixels beyond EOF");
            size_t palette_index = static_cast<size_t>(file.io.read_u8());
            uint32_t rgba = palette[palette_index];
            *pixels_ptr ++ = rgba;
            *pixels_ptr ++ = rgba >> 8;
            *pixels_ptr ++ = rgba >> 16;
        }
    }
    else
    {
        pixel_data_size = raw_data_size;
        pixel_data.reset(new char[pixel_data_size]);
        file.io.read(pixel_data.get(), pixel_data_size);
    }

    mirror(pixel_data.get(), pixel_data_size, 3 * width);

    std::unique_ptr<Image> image = Image::from_pixels(
        width,
        height,
        std::string(
            pixel_data.get(),
            pixel_data_size),
        IMAGE_PIXEL_FORMAT_BGR);
    image->update_file(file);
}
