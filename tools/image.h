#pragma once

#include <ranges>
#include <cassert>
#include <filesystem>
#include <cstdio>
#include <span>

#include <png.h>

struct RGB {
  int r, g, b;
};

class Image {
public:
  Image(int width, int height) {
    assert(width > 0);
    assert(height > 0);
    PrepareImage(width, height);
  }

  explicit Image(const std::filesystem::path& path) {
    ReadPng(path);
  }

  Image(Image&& other) : width_{other.width_}, height_{other.height_}, bytes_{other.bytes_} {
    other.width_ = 0;
    other.height_ = 0;
    other.bytes_ = nullptr;
  }

  ~Image() {
    for (auto row : std::span{bytes_, bytes_ + height_}) {
      delete[] row;
    }
    delete[] bytes_;
  }

  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;
  Image& operator=(Image&&) = delete;

  void Write(const std::filesystem::path& path) {
    if (!width_) {
      throw std::runtime_error{"Image is empty"};
    }

    auto* fp = std::fopen(path.c_str(), "wb");
    if (!fp) {
      throw std::runtime_error{"Can't open file " + path.string()};
    }

    auto png = png_create_write_struct("1.6.44", nullptr, nullptr, nullptr);
    if (!png) {
      throw std::runtime_error{"Can't create png write struct"};
    }

    auto info = png_create_info_struct(png);
    if (!info) {
      throw std::runtime_error{"Can't create png info struct"};
    }

    if (setjmp(png_jmpbuf(png))) {
      abort();
    }

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(png, info, width_, height_, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    // png_set_filler(png, 0, PNG_FILLER_AFTER);

    png_write_image(png, bytes_);
    png_write_end(png, nullptr);

    std::fclose(fp);
    png_destroy_write_struct(&png, &info);
  }

  RGB GetPixel(int y, int x) const {
    auto px = &bytes_[y][4 * x];
    return {px[0], px[1], px[2]};
  }

  void SetPixel(const RGB& pixel, int y, int x) {
    auto px = &bytes_[y][4 * x];
    px[0] = pixel.r;
    px[1] = pixel.g;
    px[2] = pixel.b;
  }

  int Height() const {
    return height_;
  }

  int Width() const {
    return width_;
  }

private:
  void PrepareImage(int width, int height) {
    height_ = height;
    width_ = width;
    bytes_ = new png_bytep[height_];
    for (auto*& row : std::span{bytes_, bytes_ + height_}) {
      row = new png_byte[4 * width_]{};
      for (auto x : std::views::iota(0, width_)) {
        row[4 * x + 3] = 255;
      }
    }
  }

  void ReadPng(const std::filesystem::path& path) {
    auto* fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
      throw std::runtime_error{"Can't open file " + path.string()};
    }

    auto png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
      throw std::runtime_error{"Can't create png read struct"};
    }

    auto info = png_create_info_struct(png);
    if (!info) {
      throw std::runtime_error{"Can't create png info struct"};
    }
    if (setjmp(png_jmpbuf(png))) {
      abort();
    }

    png_init_io(png, fp);

    png_read_info(png, info);

    width_ = png_get_image_width(png, info);
    height_ = png_get_image_height(png, info);
    auto color_type = png_get_color_type(png, info);
    auto bit_depth = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if (bit_depth == 16) {
      png_set_strip_16(png);
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
      png_set_palette_to_rgb(png);
    }

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
      png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
      png_set_tRNS_to_alpha(png);
    }

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) {
      png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
      png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    bytes_ = new png_bytep[height_];
    for (auto y = 0; y < height_; y++) {
      bytes_[y] = new png_byte[png_get_rowbytes(png, info)];
    }

    png_read_image(png, bytes_);
    png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(fp);
  }

  int width_, height_;
  png_bytepp bytes_;
};
