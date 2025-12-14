#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "local_image.h"

namespace esphome {
namespace local_image {

/*
   Set new Path
   local_image.setpath:
       id:
       path:
*/
template<typename... Ts> class LocalImageSetPathAction : public Action<Ts...> {
 public:
  LocalImageSetPathAction(LocalImage *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, path)
  void play(Ts... x) override { this->parent_->set_path(this->path_.value(x...)); }

 protected:
  LocalImage *parent_;
};

/*
     Set new Path and call reload
     local_image.reload:
         id:
         path:
*/
template<typename... Ts> class LocalImageReloadAction : public Action<Ts...> {
 public:
  LocalImageReloadAction(LocalImage *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, path)
  void play(Ts... x) override {
    this->parent_->set_path(this->path_.value(x...));
    this->parent_->load_image();
  }

 protected:
  LocalImage *parent_;
};

/*
     Free all buffers
     local_image.release:
         id:
*/
template<typename... Ts> class LocalImageReleaseAction : public Action<Ts...> {
 public:
  LocalImageReleaseAction(LocalImage *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->free_image_buffer_(); }

 protected:
  LocalImage *parent_;
};

/*
     Reload image from old URL
     local_image.load:
         id:
*/
template<typename... Ts> class LocalImageLoadAction : public Action<Ts...> {
 public:
  LocalImageLoadAction(LocalImage *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->load_image(); }

 protected:
  LocalImage *parent_;
};

class LoadFinishedTrigger : public Trigger<> {
 public:
  explicit LoadFinishedTrigger(LocalImage *parent) {
    parent->add_on_finished_callback([this]() { this->trigger(); });
  }
};

class LoadErrorTrigger : public Trigger<uint8_t> {
 public:
  explicit LoadErrorTrigger(LocalImage *parent) {
    // parent->add_on_error_callback([this](int value) { this->trigger(value); });
    parent->add_on_error_callback([this](const uint8_t rc) { this->trigger(rc); });
  }
};

}  // namespace local_image
}  // namespace esphome
