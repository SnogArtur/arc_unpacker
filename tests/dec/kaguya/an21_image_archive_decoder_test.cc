﻿#include "dec/kaguya/an21_image_archive_decoder.h"
#include "algo/ptr.h"
#include "algo/range.h"
#include "io/memory_stream.h"
#include "test_support/catch.h"
#include "test_support/decoder_support.h"
#include "test_support/file_support.h"
#include "test_support/image_support.h"

using namespace au;
using namespace au::dec::kaguya;

static void mutate_image(res::Image &image)
{
    for (const auto x : algo::range(image.width()))
    for (const auto y : algo::range(image.height()))
    {
        image.at(x, y).r ^= 43;
        image.at(x, y).g ^= 21;
        image.at(x, y).b ^= 99;
    }
}

static bstr compress(const bstr &input, const size_t band = 4)
{
    io::MemoryStream output_stream;
    for (const auto i : algo::range(band))
    {
        bstr deinterleaved;
        for (const auto j : algo::range(i, input.size() & ~3, band))
            deinterleaved += input[j];
        auto input_ptr = algo::make_ptr(deinterleaved);
        const auto init_b = *input_ptr++;
        auto last_b = init_b;
        output_stream.write<u8>(init_b);
        while (input_ptr.left())
        {
            auto b = *input_ptr++;
            output_stream.write<u8>(b);
            if (last_b == b)
            {
                auto repetitions = 0;
                while (input_ptr.left() && repetitions < 0x7FFF)
                {
                    if (*input_ptr != b)
                        break;
                    input_ptr++;
                    repetitions++;
                }

                if (repetitions < 0x80)
                {
                    output_stream.write<u8>(repetitions);
                }
                else
                {
                    output_stream.write<u8>(((repetitions - 0x80) >> 8) | 0x80);
                    output_stream.write<u8>((repetitions - 0x80));
                }

                if (input_ptr.left())
                {
                    b = *input_ptr++;
                    output_stream.write<u8>(b);
                }
            }
            last_b = b;
        }
    }
    return output_stream.seek(0).read_to_eof();
}

TEST_CASE("Atelier Kaguya AN21 archives", "[dec]")
{
    std::vector<res::Image> expected_images =
    {
        tests::get_transparent_test_image(),
        tests::get_transparent_test_image(),
        tests::get_transparent_test_image(),
    };
    mutate_image(expected_images[1]);
    mutate_image(expected_images[2]);

    io::File input_file;

    const std::vector<std::vector<u32>> unk = {
        {0},
        {1, '?', '?'},
        {2, '?'},
        {3, '?'},
        {4, '?'},
        {5, '?'},
    };
    input_file.stream.write("AN21"_b);
    input_file.stream.write_le<u16>(unk.size());
    input_file.stream.write_le<u16>('?');
    for (const auto x : unk)
    {
        input_file.stream.write<u8>(x[0]);
        for (const auto i : algo::range(1, x.size()))
            input_file.stream.write_le<u32>(x[i]);
    }

    const std::vector<std::pair<u32, u32>> unk2 = {{1, 2}, {3, 4}};
    input_file.stream.write_le<u16>(unk2.size());
    for (const auto x : unk2)
    {
        input_file.stream.write_le<u32>(x.first);
        input_file.stream.write_le<u32>(x.second);
    }
    input_file.stream.write("[PIC]10"_b);

    input_file.stream.write_le<u16>(expected_images.size());
    input_file.stream.write_le<u32>(0);
    input_file.stream.write_le<u32>(0);
    input_file.stream.write_le<u32>(expected_images.at(0).width());
    input_file.stream.write_le<u32>(expected_images.at(0).height());

    bstr last_data;

    {
        const auto &base_image = expected_images.at(0);
        input_file.stream.write_le<u32>(0);
        input_file.stream.write_le<u32>(0);
        input_file.stream.write_le<u32>(base_image.width());
        input_file.stream.write_le<u32>(base_image.height());
        input_file.stream.write_le<u32>(4);

        res::Image flipped_image(base_image);
        flipped_image.flip_vertically();
        tests::write_32_bit_image(input_file.stream, flipped_image);

        io::MemoryStream tmp_stream;
        tests::write_32_bit_image(tmp_stream, flipped_image);
        last_data = tmp_stream.seek(0).read_to_eof();
    }

    for (const auto i : algo::range(1, expected_images.size()))
    {
        res::Image flipped_image(expected_images[i]);
        flipped_image.flip_vertically();

        io::MemoryStream tmp_stream;
        tests::write_32_bit_image(tmp_stream, flipped_image);
        auto data = tmp_stream.seek(0).read_to_eof();

        auto diff_data = data;
        for (const auto j : algo::range(data.size()))
            diff_data[j] -= last_data[j];

        const auto compressed_data = compress(diff_data);
        input_file.stream.write<u8>(4);
        input_file.stream.write_le<u32>(compressed_data.size());
        input_file.stream.write(compressed_data);
        last_data = data;
    }

    const auto decoder = An21ImageArchiveDecoder();
    const auto actual_files = tests::unpack(decoder, input_file);
    tests::compare_images(actual_files, expected_images);
}