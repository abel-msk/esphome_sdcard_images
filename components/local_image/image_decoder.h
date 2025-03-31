#pragma once
#include "esphome/core/color.h"

namespace esphome
{
    namespace local_image
    {

        enum DecodeError : int
        {
            DECODE_ERROR_INVALID_TYPE = -1,
            DECODE_ERROR_UNSUPPORTED_FORMAT = -2,
            DECODE_ERROR_OUT_OF_MEMORY = -3,
        };

        class LocalImage;

        /**
         * @brief Class to abstract decoding different image formats.
         */
        class ImageDecoder
        {
        public:
            /**
             * @brief Construct a new Image Decoder object
             *
             * @param image The image to decode the stream into.
             */
            ImageDecoder(LocalImage *image) : image_(image) {}
            virtual ~ImageDecoder() = default;

            /**
             * @brief Initialize the decoder.
             *
             * @param download_size The total number of bytes that need to be downloaded for the image.
             * @return int          Returns 0 on success, a {@see DecodeError} value in case of an error.
             */
            virtual int prepare(size_t download_size)
            {
                this->download_size_ = download_size;
                return 0;
            }

            /**
             * @brief Decode a part of the image. It will try reading from the buffer.
             * There is no guarantee that the whole available buffer will be read/decoded;
             * the method will return the amount of bytes actually decoded, so that the
             * unread content can be moved to the beginning.
             *
             * @param buffer The buffer to read from.
             * @param size   The maximum amount of bytes that can be read from the buffer.
             * @return int   The amount of bytes read. It can be 0 if the buffer does not have enough content to meaningfully
             *               decode anything, or negative in case of a decoding error.
             */
            virtual int decode(uint8_t *buffer, size_t size) = 0;

            /**
             * @brief Request the image to be resized once the actual dimensions are known.
             * Called by the callback functions, to be able to access the parent Image class.
             *
             * @param width The image's width.
             * @param height The image's height.
             * @return true if the image was resized, false otherwise.
             */
            bool set_size(int width, int height);

            /**
             * @brief Fill a rectangle on the display_buffer using the defined color.
             * Will check the given coordinates for out-of-bounds, and clip the rectangle accordingly.
             * In case of binary displays, the color will be converted to binary as well.
             * Called by the callback functions, to be able to access the parent Image class.
             *
             * @param x The left-most coordinate of the rectangle.
             * @param y The top-most coordinate of the rectangle.
             * @param w The width of the rectangle.
             * @param h The height of the rectangle.
             * @param color The fill color
             */
            void draw(int x, int y, int w, int h, const Color &color);

            bool is_finished() const { return this->decoded_bytes_ == this->download_size_; }

        protected:
            LocalImage *image_;
            // Initializing to 1, to ensure it is distinguishable from initial "decoded_bytes_".
            // Will be overwritten anyway once the download size is known.
            size_t download_size_ = 1;
            size_t decoded_bytes_ = 0;
            double x_scale_ = 1.0;
            double y_scale_ = 1.0;
        };
    } // namespace online_image
} // namespace esphome
