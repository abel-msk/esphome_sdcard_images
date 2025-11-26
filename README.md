# About
This repository contains ESPhome component for loading and display image file from locally accessed storage device.
For interaction with sd card used [this component](https://github.com/abel-msk/esphome_sd_card).
Copy of sd card component also included un this repository for debug purposes.

# Install
Just add in yours esphome config file next lines:

```
external_components:
  - source:
        type: git
        url: https://github.com/abel-msk/esphome_sdcard_images
    components: [local_image]
```

## Base local_image Configuration
local_image component load image from local accessed storage, convert to bitmap and save in memory.
Component use [`image::Image`](https://esphome.io/components/image/) interface, so it can be accessed from other components witch diaplay images. For example [lgvl](https://esphome.io/components/lvgl/)



```yaml

local_image:
  - id: varImage
    path:  "/bgwf/day_rain.png"
    storage_id: sdcard1      #  Line to sd_mmc_carsd component configuration with id sdcard1
    format: png
    resize: 480x320
    type: RGB565     
    
```

**Configuration variables:**

- **id** (**Required**, [ID](https://esphome.io/guides/configuration-types/#id))): The ID with which you will be able to reference the image later in your display code.
- **storage_id** (**Required**, The ID of dtorage component. For storage access component require storage drivers with [storage::FileProvider] (https://github.com/esphome/esphome/pull/11390) interface.

Other options are the same as in [online_image](https://esphome.io/components/online_image/#online_image) component except URL and update_interval


**Access storage interface**

```yaml

external_components:
  - source: github://pr#11390
    components: [storage]
    refresh: 1h

```
local_image:
  - id: varImage
    path:  "/bgwf/day_rain.png"
    sd_mmc_card_id: sdcard1      #  Line to sd_mmc_carsd component configuration with id sdcard1
    format: png
    resize: 480x320
    type: RGB565     
    on_load_finished:            # Callback called when image loaded
      - lvgl.image.update:       # draw image in place of lvgl image with id bgImage
          id: bgImage
          src: varImage
    
    on_error:                    # Callback called if got error during locaing image
      - logger.log: 
          format: "Loading image Error %d"
          args: [x]
```

For start load image with pointes url use automation like this:

```
then:
      - local_image.reload: 
          id: varImage
          path:  !lambda |-
              ...
              return "/gd/imahe1.png"
```

This action will read  image '/gd/imahe1.png' from sd card  and load into memory.
When load finished this will call `on_load_finished` callbask for drawing (see loacal_image initializing).
---

Because storage interface not in release yet, i planing for make localy available component sdfs for accessing sdcards. Late something like this will migate to esphome project.
