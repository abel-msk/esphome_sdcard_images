#pragma once

// #include "esphome/components/http_request/http_request.h"
// #define USE_ONLINE_IMAGE_JPEG_SUPPORT
// #define USE_ONLINE_IMAGE_BMP_SUPPORT
// #define USE_ONLINE_IMAGE_PNG_SUPPORT

#include "../sd_mmc_card/sd_mmc_card.h"
#include "esphome/components/image/image.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"

#include "image_decoder.h"

namespace esphome
{
    namespace local_image
    {

        // using t_http_codes = enum {
        //     HTTP_CODE_OK = 200,
        //     HTTP_CODE_NOT_MODIFIED = 304,
        //     HTTP_CODE_NOT_FOUND = 404,
        // };

        enum RetCodes : short {
            OK = 0,
            NO_MEM = 1,
            FILE_NOT_NOTFOUND,
            DECODER_UNKNOWN,
            DECODER_NOT_INIT,
            DECODER_NOT_PREPARE,
            DECODER_PROC_ERR
        };


        /**
         * @brief Format that the image is encoded with.
         */
        enum ImageFormat
        {
            /** Automatically detect from MIME type. Not supported yet. */
            AUTO,
            /** JPEG format. */
            JPEG,
            /** PNG format. */
            PNG,
            /** BMP format. */
            BMP,
        };

        /**
         * @brief Download an image from a given URL, and decode it using the specified decoder.
         * The image will then be stored in a buffer, so that it can be re-displayed without the
         * need to re-download or re-decode.
         */

        class LocalImage : public PollingComponent,
                           public image::Image
        {
        public:
            /**
             * @brief Construct a new LocalImage object.
             *
             * @param path
             * @param width Desired width of the target image area.
             * @param height Desired height of the target image area.
             * @param format Format that the image is encoded in (@see ImageFormat).
             * @param buffer_size Size of the buffer used to download the image.
             */
            LocalImage(sd_mmc_card::SdMmc *card, const std::string &path, int width, int height, ImageFormat format, image::ImageType type,
                       image::Transparency transparency);

            void draw(int x, int y, display::Display *display, Color color_on, Color color_off) override;

            void update() override;
            void loop() override;
            void map_chroma_key(Color &color);

            /** Set the URL to download the image from. */
            void set_path(const std::string &path)
            {
                if (this->validate_path_(path))
                {
                    this->path_ = path;
                }
            }

            void set_sd_mmc_card(sd_mmc_card::SdMmc *);
            
            /**
             * @brief Set the image that needs to be shown as long as the downloaded image
             *  is not available.
             *
             * @param placeholder Pointer to the (@link Image) to show as placeholder.
             */
            void set_placeholder(image::Image *placeholder) { this->placeholder_ = placeholder; }

            /**
             * Release the buffer storing the image. The image will need to be downloaded again
             * to be able to be displayed.
             */
            void release();

            /**
             * @brief  When loading finished release  buffers for used for prepare image 
             * 
             */
            void end_loading_();
            /**
             * Resize the download buffer
             *
             * @param size The new size for the download buffer.
             */
            // size_t resize_download_buffer(size_t size) { return this->download_buffer_.resize(size); }

            void add_on_finished_callback(std::function<void()> &&callback);
            void add_on_error_callback(std::function<void()> &&callback);

        protected:
            bool validate_path_(const std::string &path);

            RAMAllocator<uint8_t> allocator_{};

            uint32_t get_buffer_size_() const { return get_buffer_size_(this->buffer_width_, this->buffer_height_); }
            int get_buffer_size_(int width, int height) const { return (this->get_bpp() * width + 7u) / 8u * height; }

            int get_position_(int x, int y) const { return (x + y * this->buffer_width_) * this->get_bpp() / 8; }

            ESPHOME_ALWAYS_INLINE bool is_auto_resize_() const { return this->fixed_width_ == 0 || this->fixed_height_ == 0; }

            /**
             * @brief Resize the image buffer to the requested dimensions.
             *
             * The buffer will be allocated if not existing.
             * If the dimensions have been fixed in the yaml config, the buffer will be created
             * with those dimensions and not resized, even on request.
             * Otherwise, the old buffer will be deallocated and a new buffer with the requested
             * allocated
             *
             * @param width
             * @param height
             * @return 0 if no memory could be allocated, the size of the new buffer otherwise.
             */
            size_t resize_(int width, int height);

            /**
             * @brief Draw a pixel into the buffer.
             *
             * This is used by the decoder to fill the buffer that will later be displayed
             * by the `draw` method. This will internally convert the supplied 32 bit RGBA
             * color into the requested image storage format.
             *
             * @param x Horizontal pixel position.
             * @param y Vertical pixel position.
             * @param color 32 bit color to put into the pixel.
             */
            void draw_pixel_(int x, int y, Color color);

            // void end_connection_();

            CallbackManager<void()> load_finished_callback_{};
            CallbackManager<void()> download_error_callback_{};

            sd_mmc_card::SdMmc *sd_mmc_card_{nullptr};

            std::unique_ptr<ImageDecoder> decoder_{nullptr};

            uint8_t *source_buffer_;
            size_t source_size_ = 0;
            uint8_t *buffer_;
            bool loded = false;

            RetCodes last_error_code_;

            const ImageFormat format_;
            image::Image *placeholder_{nullptr};

            std::string path_{""};

            /** width requested on configuration, or 0 if non specified. */
            const int fixed_width_;
            /** height requested on configuration, or 0 if non specified. */
            const int fixed_height_;
            /**
             * Actual width of the current image. If fixed_width_ is specified,
             * this will be equal to it; otherwise it will be set once the decoding
             * starts and the original size is known.
             * This needs to be separate from "BaseImage::get_width()" because the latter
             * must return 0 until the image has been decoded (to avoid showing partially
             * decoded images).
             */
            int buffer_width_;
            /**
             * Actual height of the current image. If fixed_height_ is specified,
             * this will be equal to it; otherwise it will be set once the decoding
             * starts and the original size is known.
             * This needs to be separate from "BaseImage::get_height()" because the latter
             * must return 0 until the image has been decoded (to avoid showing partially
             * decoded images).
             */
            int buffer_height_;

            friend bool ImageDecoder::set_size(int width, int height);
            friend void ImageDecoder::draw(int x, int y, int w, int h, const Color &color);
        };

         /*   
            Set new Path
        */
        template <typename... Ts>
        class LocalImageSetPathAction : public Action<Ts...>
        {
        public:
            LocalImageSetPathAction(LocalImage *parent) : parent_(parent) {}
            TEMPLATABLE_VALUE(std::string, path)
            void play(Ts... x) override
            {
                this->parent_->set_path(this->path_.value(x...));
            }

        protected:
            LocalImage *parent_;
        };

        /*   
        Set new Path and call reload
        */
        template <typename... Ts>
        class LocalImageReloadAction : public Action<Ts...>
        {
        public:
            LocalImageReloadAction(LocalImage *parent) : parent_(parent) {}
            TEMPLATABLE_VALUE(std::string, path)
            void play(Ts... x) override
            {
                this->parent_->set_path(this->path_.value(x...));
                this->parent_->update();
            }

        protected:
            LocalImage *parent_;
        };

        /*   
         Free all buffers
        */
        template <typename... Ts>
        class LocalImageReleaseAction : public Action<Ts...>
        {
        public:
            LocalImageReleaseAction(LocalImage *parent) : parent_(parent) {}
            void play(Ts... x) override { this->parent_->release(); }

        protected:
            LocalImage *parent_;
        };

        template <typename... Ts>
        class LocalImageLoadAction : public Action<Ts...>
        {
        public:
        LocalImageLoadAction(LocalImage *parent) : parent_(parent) {}
            void play(Ts... x) override  { this->parent_->update(); }
        protected:
            LocalImage *parent_;
        };



        class LoadFinishedTrigger : public Trigger<>
        {
        public:
            explicit LoadFinishedTrigger(LocalImage *parent)
            {
                parent->add_on_finished_callback([this]()
                                                 { this->trigger(); });
            }
        };
        
        class LoadErrorTrigger : public Trigger<>
        {
        public:
            explicit LoadErrorTrigger(LocalImage *parent)
            {
                parent->add_on_error_callback([this]()
                                              { this->trigger(); });
            }
        };

    } // namespace online_image
} // namespace esphome
