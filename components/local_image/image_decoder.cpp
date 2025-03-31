#include "image_decoder.h"
#include "local_image.h"

#include "esphome/core/log.h"

namespace esphome
{
    namespace local_image
    {

        static const char *const TAG = "online_image.decoder";

        bool ImageDecoder::set_size(int width, int height)
        {
            bool success = this->image_->resize_(width, height) > 0;
            this->x_scale_ = static_cast<double>(this->image_->buffer_width_) / width;
            this->y_scale_ = static_cast<double>(this->image_->buffer_height_) / height;
            return success;
        }

        void ImageDecoder::draw(int x, int y, int w, int h, const Color &color)
        {
            auto width = std::min(this->image_->buffer_width_, static_cast<int>(std::ceil((x + w) * this->x_scale_)));
            auto height = std::min(this->image_->buffer_height_, static_cast<int>(std::ceil((y + h) * this->y_scale_)));
            for (int i = x * this->x_scale_; i < width; i++)
            {
                for (int j = y * this->y_scale_; j < height; j++)
                {
                    this->image_->draw_pixel_(i, j, color);
                }
            }
        }
    } // namespace online_image
} // namespace esphome
