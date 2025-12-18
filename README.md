# About
This repository contains ESPhome component for loading and display image file from locally accessed storage device.
The device driver should imlement storage component.  For accessing sdcard is possible to use sdmmc component. 

# Install
All code was moved under my own esphome fork and can be accessed via external component.
Just add in yours esphome config file next lines:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/abel-msk/esphome
      ref: local_image
    components: [ storage, sdmmc, local_image ]
```

Or it steel can be used from this repository (Significantly fewer files to download). In this case use:

```yaml
external_components:
  - source: github://pr#11390
    components: [ storage ]
    refresh: 1h
  - source: github://abel-msk/sdmmc
    components: [ sdmmc ]
  - source: github://abel-msk/local_image
    components: [ local_image ]

```

# Sdmmc

## Base sdmmc Configuration

Sdmmc component is my simple implementation of storage component. Used for access file system on sdcard.
Component get access to file and directoryes manipulations and fire read and wtite operations.
It can be used independently of local_image for you own projects.
sdmmc uses FATLib for filesystems and MMC card inetrface implemented on ESP32 series soc.

```yaml
sdmmc:
  id: storage_1
  cmd_pin: GPIO12
  clk_pin: GPIO13
  mode_1bit: true
  data0_pin: GPIO16

```

**Configuration variables:**

- **id** (**Required**, [ID](https://esphome.io/guides/configuration-types/#id))): The ID with which you will be able to storage acces later in your code.
- **cmd_pin** (**Required** [Pin](https://esphome.io/guides/configuration-types/#pin)): Controll pin  
- **clk_pin** (**Required** [Pin](https://esphome.io/guides/configuration-types/#pin)): Clock rate pin. Requiter PWM ready pin.
- **mode_1bit** (**Required**) Set data bus width. Can bee 1bit witdth (True) or 4Bit width (False).
- **data0_pin** (**Required** [Pin](https://esphome.io/guides/configuration-types/#pin)): Bud bit 0  
- **data1_pin** (**Optional** [Pin](https://esphome.io/guides/configuration-types/#pin)): Bud bit 1  used if mode_1bit set to `False`
- **data2_pin** (**Optional** [Pin](https://esphome.io/guides/configuration-types/#pin)): Bud bit 2  used if mode_1bit set to `False`
- **data3_pin** (**Optional** [Pin](https://esphome.io/guides/configuration-types/#pin)): Bud bit 3  used if mode_1bit set to `False`


Of cource You can create you own imoplementation os storage access  for local_image component.
All you need in this way is to inherit yours class from FileProvider calss inside storage.

Storage and FileProvider interface for study is available [here](https://github.com/abel-msk/esphome/blob/storage/esphome/components/storage/file_provider.h)

For use storage  uou configuration use 

```yaml

external_components:
  - source: github://pr#11390
    components: [storage]
    refresh: 1h

```

# Local_image

## Base local_image Configuration
local_image component load image from local accessed storage, convert to bitmap and save in memory.
Component use [`image::Image`](https://esphome.io/components/image/) interface, so it can be accessed from other components witch diaplay images. For example [lvgl](https://esphome.io/components/lvgl/) component

```yaml

local_image:
  - id: varImage
    path:  "/bgwf/day_rain.png"
    storage_id: storage_1      #  Line to sd_mmc_carsd component configuration with id sdcard1
    format: png
    resize: 480x320
    type: RGB565     
    
```

**Configuration variables:**

- **id** (**Required**, [ID](https://esphome.io/guides/configuration-types/#id))): The ID with which you will be able to reference the image later in your display code.
- **storage_id** (**Required**) The ID of dtorage component. For storage access component require storage drivers with [storage::FileProvider] (https://github.com/esphome/esphome/pull/11390) interface.
- **path** (**Required**) The path to file on loacaly accessed storage device.
- 
Other options are the same as in the [online_image](https://esphome.io/components/online_image/#online_image) component except URL.

**Access storage interface**

```yaml

external_components:
  - source: github://pr#11390
    components: [storage]
    refresh: 1h
```

More detailed example

```yaml
local_image:
  - id: varImage
    path:  "/bgwf/day_rain.png"
    storage_id: sdcard1         #  id of storage component configuration with id sdcard1
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

For start load image or reload image from path:

```yaml
then:
      - local_image.reload:    #  Action for re-read image from storage (in case image was changed )
          id: varImage
          path:  !lambda |-
              ...
              return "/bgwf/day_rain.png"


lvgl:
   . . .
  pages:
    - id: main_page
      widgets: 
        - image:    
            id: bgImage  
            align: CENTER
            src: varImage
            y: 0
            x: 0
```


The action local_image.reload will read  image '/bgwf/day_rain.png' and load into memory.
When load finished this will call `on_load_finished` callbask for drawing (see loacal_image initialising).


