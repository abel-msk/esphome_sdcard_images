from esphome import automation
import esphome.codegen as cg

# from esphome.components.http_request import CONF_HTTP_REQUEST_ID, HttpRequestComponent
from esphome.components.image import (
    CONF_INVERT_ALPHA,
    CONF_TRANSPARENCY,
    IMAGE_SCHEMA,
    Image_,
    get_image_type_enum,
    get_transparency_enum,
)
from esphome.components.storage import FileProvider
import esphome.config_validation as cv
from esphome.const import (
    CONF_DITHER,
    CONF_FILE,
    CONF_FORMAT,
    CONF_ID,
    CONF_ON_ERROR,
    CONF_PATH,
    CONF_RESIZE,
    CONF_TRIGGER_ID,
    CONF_TYPE,
)

AUTO_LOAD = ["image", "storage"]
DEPENDENCIES = ["display"]
CODEOWNERS = ["@abel-msk"]
MULTI_CONF = True


# AUTO_LOAD = ["image"]
# DEPENDENCIES = ["display", "http_request"]


CONF_ON_LOAD_FINISHED = "on_load_finished"
# CONF_ON_ERROR = "on_error"
CONF_PLACEHOLDER = "placeholder"
CONF_STORAGE_FS_ID = "storage_id"
CONF_IMAGE_PATH = "path"

# _LOGGER = logging.getLogger(__name__)

local_image_ns = cg.esphome_ns.namespace("local_image")
ImageFormat = local_image_ns.enum("ImageFormat")
LocalImage = local_image_ns.class_("LocalImage", cg.Component, Image_)


class Format:
    def __init__(self, image_type):
        self.image_type = image_type

    @property
    def enum(self):
        return getattr(ImageFormat, self.image_type)

    def actions(self):
        pass


class BMPFormat(Format):
    def __init__(self):
        super().__init__("BMP")

    def actions(self):
        cg.add_define("USE_ONLINE_IMAGE_BMP_SUPPORT")


class JPEGFormat(Format):
    def __init__(self):
        super().__init__("JPEG")

    def actions(self):
        cg.add_define("USE_ONLINE_IMAGE_JPEG_SUPPORT")
        cg.add_library("JPEGDEC", None, "https://github.com/bitbank2/JPEGDEC#ca1e0f2")


class PNGFormat(Format):
    def __init__(self):
        super().__init__("PNG")

    def actions(self):
        cg.add_define("USE_ONLINE_IMAGE_PNG_SUPPORT")
        cg.add_library("pngle", "1.0.2")


IMAGE_FORMATS = {
    x.image_type: x
    for x in (
        BMPFormat(),
        JPEGFormat(),
        PNGFormat(),
    )
}
IMAGE_FORMATS.update({"JPG": IMAGE_FORMATS["JPEG"]})

# Actions
SetPathAction = local_image_ns.class_(
    "LocalImageSetPathAction", automation.Action, cg.Parented.template(LocalImage)
)
ReleaseImageAction = local_image_ns.class_(
    "LocalImageReleaseAction", automation.Action, cg.Parented.template(LocalImage)
)

LocalImageReloadAction = local_image_ns.class_(
    "LocalImageReloadAction", automation.Action, cg.Parented.template(LocalImage)
)

LocalImageLoadAction = local_image_ns.class_(
    "LocalImageLoadAction", automation.Action, cg.Parented.template(LocalImage)
)

# Triggers
LoadFinishedTrigger = local_image_ns.class_(
    "LoadFinishedTrigger", automation.Trigger.template()
)
LoadErrorTrigger = local_image_ns.class_(
    "LoadErrorTrigger", automation.Trigger.template()
)


def remove_options(*options):
    return {
        cv.Optional(option): cv.invalid(
            f"{option} is an invalid option for local_image"
        )
        for option in options
    }


LOCAL_IMAGE_SCHEMA = IMAGE_SCHEMA.extend(
    remove_options(CONF_FILE, CONF_INVERT_ALPHA, CONF_DITHER)
).extend(
    {
        cv.Required(CONF_ID): cv.declare_id(LocalImage),
        cv.Required(CONF_STORAGE_FS_ID): cv.use_id(FileProvider),
        cv.Required(CONF_IMAGE_PATH): cv.string,
        cv.Required(CONF_FORMAT): cv.one_of(*IMAGE_FORMATS, upper=True),
        cv.Optional(CONF_PLACEHOLDER): cv.use_id(Image_),
        cv.Optional(CONF_ON_LOAD_FINISHED): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LoadFinishedTrigger),
            }
        ),
        cv.Optional(CONF_ON_ERROR): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LoadErrorTrigger),
            }
        ),
    }
)

CONFIG_SCHEMA = cv.Schema(
    cv.All(
        LOCAL_IMAGE_SCHEMA,
        cv.require_framework_version(
            # esp8266 not supported yet; if enabled in the future, minimum version of 2.7.0 is needed
            # esp8266_arduino=cv.Version(2, 7, 0),
            esp32_arduino=cv.Version(0, 0, 0),
            esp_idf=cv.Version(4, 0, 0),
            rp2040_arduino=cv.Version(0, 0, 0),
            host=cv.Version(0, 0, 0),
        ),
    )
)

SET_PATH_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(LocalImage),
        cv.Required(CONF_PATH): cv.templatable(cv.string),
    }
)

RELEASE_IMAGE_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(LocalImage),
    }
)

LOAD_IMAGE_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(LocalImage),
    }
)


@automation.register_action("local_image.load", LocalImageLoadAction, LOAD_IMAGE_SCHEMA)
@automation.register_action(
    "local_image.release", ReleaseImageAction, RELEASE_IMAGE_SCHEMA
)
@automation.register_action("local_image.set_path", SetPathAction, SET_PATH_SCHEMA)
async def local_image_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    if CONF_PATH in config:
        template_ = await cg.templatable(config[CONF_PATH], args, cg.std_string)
        cg.add(var.set_path(template_))
    return var


@automation.register_action(
    "local_image.reload", LocalImageReloadAction, SET_PATH_SCHEMA
)
async def sd_mmc_append_file_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    path_ = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    cg.add(var.set_path(path_))
    return var


async def to_code(config):
    image_format = IMAGE_FORMATS[config[CONF_FORMAT]]
    image_format.actions()

    width, height = config.get(CONF_RESIZE, (0, 0))
    transparent = get_transparency_enum(config[CONF_TRANSPARENCY])

    var = cg.new_Pvariable(
        config[CONF_ID],
        width,
        height,
        image_format.enum,
        get_image_type_enum(config[CONF_TYPE]),
        transparent,
    )
    await cg.register_component(var, config)

    # fp_id = config.get(CONF_STORAGE_FS_ID)
    # fp = await cg.get_variable(fp_id)
    # cg.add(var.set_storage(fp))

    file_provider = await cg.get_variable(config[CONF_STORAGE_FS_ID])
    cg.add(var.set_storage(file_provider))

    # path = await cg.get_variable(config[CONF_IMAGE_PATH])
    cg.add(var.set_path(config[CONF_IMAGE_PATH]))

    if placeholder_id := config.get(CONF_PLACEHOLDER):
        placeholder = await cg.get_variable(placeholder_id)
        cg.add(var.set_placeholder(placeholder))

    for conf in config.get(CONF_ON_LOAD_FINISHED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_ERROR, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint8, "x")], conf)
