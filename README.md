# About
This repository contains ESPhome component for loading and display image file from scmmc card.
For interaction with sd card used [this component](https://github.com/abel-msk/esphome_sd_card).
Copy of sd card component also included un this repository for debug purposes.

# Install
Just add in yours esphome config file next lines:

```
external_components:
  - source:
        type: git
        url: https://github.com/abel-msk/esphome-works
    components: [local_image, sd_mms_card]
```
# Usage

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

