#include "local_image.h"

#include "esphome/core/log.h"

static const char *const TAG = "local_image";

#include "image_decoder.h"

#ifdef USE_ONLINE_IMAGE_BMP_SUPPORT
#include "bmp_image.h"
#endif
#ifdef USE_ONLINE_IMAGE_JPEG_SUPPORT
#include "jpeg_image.h"
#endif
#ifdef USE_ONLINE_IMAGE_PNG_SUPPORT
#include "png_image.h"
#endif

namespace esphome {
namespace local_image {

using image::ImageType;
using storage::FileProvider;

inline bool is_color_on(const Color &color) {
  // This produces the most accurate monochrome conversion, but is slightly slower.
  //  return (0.2125 * color.r + 0.7154 * color.g + 0.0721 * color.b) > 127;

  // Approximation using fast integer computations; produces acceptable results
  // Equivalent to 0.25 * R + 0.5 * G + 0.25 * B
  return ((color.r >> 2) + (color.g >> 1) + (color.b >> 2)) & 0x80;
}

LocalImage::LocalImage(int width, int height, ImageFormat format, ImageType type, image::Transparency transparency)
    : Image(nullptr, 0, 0, type, transparency),
      buffer_(nullptr),
      format_(format),
      fixed_width_(width),
      fixed_height_(height),
      source_size_(0),
      source_buffer_(nullptr),
      image_loaded_(false),
      last_error_(ErrorCode::OK) {
  this->decoder_ = nullptr;
  this->source_size_ = 0;
  // this->source_buffer_ = {nullptr};
  // this->loded = false;
  // this->last_error_ = OK;
}

void LocalImage::dump_config() {
  ESP_LOGCONFIG(TAG, "LocalImage:");
  ESP_LOGCONFIG(TAG, "   Format: %s", LOG_STR_ARG(format_));
  ESP_LOGCONFIG(TAG, "   Type: %s", LOG_STR_ARG(this->get_type()));
  ESP_LOGCONFIG(TAG, "   Width: %d", this->get_width());
  ESP_LOGCONFIG(TAG, "   Height: %d", this->get_height());
  ESP_LOGCONFIG(TAG, "   Path: %d", this->path_.c_str());
};

void LocalImage::setup() {
  if (this->provider_->is_ready()) {
    this->load_image();
  }
};

void LocalImage::set_path(const std::string &path) { this->path_ = path; }

void LocalImage::set_storage(storage::FileProvider *file_provider) { this->provider_ = file_provider; }

void LocalImage::draw(int x, int y, display::Display *display, Color color_on, Color color_off) {
  ESP_LOGD(TAG, "Draw image.");

  if (this->buffer_) {
    Image::draw(x, y, display, color_on, color_off);
  } else if (this->placeholder_) {
    this->placeholder_->draw(x, y, display, color_on, color_off);
  }
}

void LocalImage::free_source_buffer_() {
  if (source_buffer_ != nullptr) {
    this->allocator_.deallocate(this->source_buffer_, this->source_size_);
    this->source_buffer_ == nullptr;
  }
  this->source_size_ = 0;

  if (this->decoder_ != nullptr) {
    this->decoder_.reset();
    this->decoder_ = nullptr;
  }
}

void LocalImage::free_image_buffer_() {
  if (this->buffer_) {
    ESP_LOGV(TAG, "Deallocating image buffer...");
    this->allocator_.deallocate(this->buffer_, this->get_buffer_size_());
    this->data_start_ = nullptr;
    this->buffer_ = nullptr;
    this->width_ = 0;
    this->height_ = 0;
    this->buffer_width_ = 0;
    this->buffer_height_ = 0;
  }
}

// size_t LocalImage::create_image_buffer_(size_t new_size) {
//   ESP_LOGD(TAG, "Allocating new buffer of %zu bytes", new_size);
//     this->buffer_ = this->allocator_.allocate(new_size);
//     if (this->buffer_ == nullptr) {
//       ESP_LOGE(TAG, "allocation of %zu bytes failed. Biggest block in heap: %zu Bytes", new_size,
//               this->allocator_.get_max_free_block_size());
//       this->last_error_ = ErrorCode::NO_MEM;
//       return 0;
//     }
//   return new_size;
// }

size_t LocalImage::create_image_buffer(int width_in, int height_in) {
  int width = this->fixed_width_;
  int height = this->fixed_height_;
  if (this->is_auto_resize_()) {
    width = width_in;
    height = height_in;
    if (this->width_ != width && this->height_ != height) {
      this->free_image_buffer_();
    }
  }

  size_t new_size = this->get_buffer_size_(width, height);
  if ((this->buffer_) && (new_size == this->get_buffer_size_())) {
    // Buffer already allocated => no need to resize
    return new_size;
  }

  ESP_LOGD(TAG, "Allocating new buffer of %zu bytes", new_size);
  this->buffer_ = this->allocator_.allocate(new_size);
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "allocation of %zu bytes failed. Biggest block in heap: %zu Bytes", new_size,
             this->allocator_.get_max_free_block_size());
    this->last_error_ = ErrorCode::NO_MEM;
    return 0;
  }

  this->buffer_width_ = width;
  this->buffer_height_ = height;
  this->width_ = width;
  this->height_ = height;
  ESP_LOGV(TAG, "New size: (%d, %d)", width, height);
  return new_size;
}

//------------------------------------------------------------------
//
void LocalImage::load_image() {
  size_t file_size = 0;
  size_t read_bytes = 0;
  this->last_error_ = ErrorCode::OK;

  //
  //  If free memory from previous loading. if any.
  //
  if (this->source_buffer_) {
    this->free_source_buffer_();
  }

  ESP_LOGD(TAG, "Loading image from file : %s", this->path_.c_str());

  file_size = this->provider_->get_size(this->path_);
  if ((file_size == 0) || (this->provider_->error() != 0)) {
    ESP_LOGE(TAG, "File %s check error: %s", path_.c_str(), this->provider_->error_str());
    this->last_error_ = ErrorCode::FILE_NOT_NOTFOUND;
    return;
  }

  //
  //    Get Memory for read file
  //
  this->source_buffer_ = this->allocator_.allocate(file_size);
  if (this->source_buffer_ == nullptr) {
    ESP_LOGE(TAG, "No memory. (Size: %zu)", file_size);
    this->last_error_ = ErrorCode::NO_MEM;
    return;
  }
  this->source_size_ = file_size;

  //
  //   Read file
  //
  ESP_LOGD(TAG, "Read file: %s. Size=%d", path_.c_str(), file_size);
  storage::FileObj *file = this->provider_->open_file(path_, storage::OPEN_READ);
  if (this->provider_->error() != 0) {
    ESP_LOGE(TAG, "Error open file %s : %s ", path_.c_str(), this->provider_->error_str());
  }
  read_bytes = file->read(this->source_buffer_, this->source_size_);
  if (file->error() != 0) {
    ESP_LOGE(TAG, "Error reading file %s : %s", path_.c_str(), this->provider_->error_str());
    this->free_source_buffer_();
    return;
  }
  delete file;  // ???
  ESP_LOGD(TAG, "Read %d bytes", read_bytes);
  if (read_bytes < this->source_size_) {
    ESP_LOGW(TAG, "Expect %d bytes, but read %d bytes.", this->source_size_, read_bytes);
  }

  //
  //  Prepare Decoder
  //
#ifdef USE_ONLINE_IMAGE_BMP_SUPPORT
  if (this->format_ == ImageFormat::BMP) {
    ESP_LOGD(TAG, "Allocating BMP decoder");
    this->decoder_ = make_unique<BmpDecoder>(this);
  }
#endif  // ONLINE_IMAGE_BMP_SUPPORT
#ifdef USE_ONLINE_IMAGE_JPEG_SUPPORT
  if (this->format_ == ImageFormat::JPEG) {
    ESP_LOGD(TAG, "Allocating JPEG decoder");
    this->decoder_ = esphome::make_unique<JpegDecoder>(this);
  }
#endif  // USE_ONLINE_IMAGE_JPEG_SUPPORT
#ifdef USE_ONLINE_IMAGE_PNG_SUPPORT
  if (this->format_ == ImageFormat::PNG) {
    ESP_LOGD(TAG, "Allocating PNG decoder");
    this->decoder_ = make_unique<PngDecoder>(this);
  }
#endif  // ONLINE_IMAGE_PNG_SUPPORT

  if (!this->decoder_) {
    ESP_LOGE(TAG, "Could not instantiate decoder. Image format unsupported: %d", this->format_);
    this->last_error_ = ErrorCode::DECODER_NOT_INIT;
    this->free_source_buffer_();
    return;
  }

  if (this->decoder_->prepare(read_bytes) < 0) {
    ESP_LOGE(TAG, "Error when prepare decoder.");
    this->last_error_ = ErrorCode::DECODER_NOT_PREPARE;
    this->free_source_buffer_();
    return;
  }

  auto fed = this->decoder_->decode(this->source_buffer_, this->source_size_);
  if (fed < 0) {
    ESP_LOGE(TAG, "Error decoding image %d", fed);
    this->last_error_ = ErrorCode::DECODER_PROC_ERR;

  } else {
    //
    // Pass prepared patas to parent Image class
    //
    this->data_start_ = this->buffer_;
    this->width_ = buffer_width_;
    this->height_ = buffer_height_;
    this->image_loaded_ = true;
    ESP_LOGD(TAG, "Image fully loaded, read %zu bytes, width/height = %d/%d", fed, this->width_, this->height_);
  }

  this->free_source_buffer_();
}

/**********************************************************************************************
 *
 * @brief Do load and decode image.
 *
 */
void LocalImage::loop() {
  if (this->image_loaded_) {
    this->load_finished_callback_.call();
    this->image_loaded_ = false;
  }

  if (this->last_error_ != ErrorCode::OK) {
    this->on_err_callback_.call(static_cast<uint8_t>(this->last_error_));
    this->last_error_ = ErrorCode::OK;
  }
  return;
}

void LocalImage::map_chroma_key(Color &color) {
  if (this->transparency_ == image::TRANSPARENCY_CHROMA_KEY) {
    if (color.g == 1 && color.r == 0 && color.b == 0) {
      color.g = 0;
    }
    if (color.w < 0x80) {
      color.r = 0;
      color.g = this->type_ == ImageType::IMAGE_TYPE_RGB565 ? 4 : 1;
      color.b = 0;
    }
  }
}

void LocalImage::draw_pixel_(int x, int y, Color color) {
  if (!this->buffer_) {
    ESP_LOGE(TAG, "Buffer not allocated!");
    return;
  }
  if (x < 0 || y < 0 || x >= this->buffer_width_ || y >= this->buffer_height_) {
    ESP_LOGE(TAG, "Tried to paint a pixel (%d,%d) outside the image!", x, y);
    return;
  }
  uint32_t pos = this->get_position_(x, y);
  switch (this->type_) {
    case ImageType::IMAGE_TYPE_BINARY: {
      const uint32_t width_8 = ((this->width_ + 7u) / 8u) * 8u;
      pos = x + y * width_8;
      auto bitno = 0x80 >> (pos % 8u);
      pos /= 8u;
      auto on = is_color_on(color);
      if (this->has_transparency() && color.w < 0x80)
        on = false;
      if (on) {
        this->buffer_[pos] |= bitno;
      } else {
        this->buffer_[pos] &= ~bitno;
      }
      break;
    }
    case ImageType::IMAGE_TYPE_GRAYSCALE: {
      uint8_t gray = static_cast<uint8_t>(0.2125 * color.r + 0.7154 * color.g + 0.0721 * color.b);
      if (this->transparency_ == image::TRANSPARENCY_CHROMA_KEY) {
        if (gray == 1) {
          gray = 0;
        }
        if (color.w < 0x80) {
          gray = 1;
        }
      } else if (this->transparency_ == image::TRANSPARENCY_ALPHA_CHANNEL) {
        if (color.w != 0xFF)
          gray = color.w;
      }
      this->buffer_[pos] = gray;
      break;
    }
    case ImageType::IMAGE_TYPE_RGB565: {
      this->map_chroma_key(color);
      uint16_t col565 = display::ColorUtil::color_to_565(color);
      this->buffer_[pos + 0] = static_cast<uint8_t>((col565 >> 8) & 0xFF);
      this->buffer_[pos + 1] = static_cast<uint8_t>(col565 & 0xFF);
      if (this->transparency_ == image::TRANSPARENCY_ALPHA_CHANNEL) {
        this->buffer_[pos + 2] = color.w;
      }
      break;
    }
    case ImageType::IMAGE_TYPE_RGB: {
      this->map_chroma_key(color);
      this->buffer_[pos + 0] = color.r;
      this->buffer_[pos + 1] = color.g;
      this->buffer_[pos + 2] = color.b;
      if (this->transparency_ == image::TRANSPARENCY_ALPHA_CHANNEL) {
        this->buffer_[pos + 3] = color.w;
      }
      break;
    }
  }
}

void LocalImage::add_on_finished_callback(std::function<void()> &&callback) {
  this->load_finished_callback_.add(std::move(callback));
}

void LocalImage::add_on_error_callback(std::function<void(uint8_t)> &&callback) {
  // this->on_err_callback_.add(std::move(callback));
  this->on_err_callback_.add(std::move(callback));
}

}  // namespace local_image
}  // namespace esphome
