set(sensorlib_src_dirs
    "SensorLib/src"
    "SensorLib/src/touch"
    "SensorLib/src/platform"
    "SensorLib/src/bosch"
    "SensorLib/src/bosch/BMM150"
    "SensorLib/src/bosch/common"
)

set(sensorlib_include_dirs
    "SensorLib/src"
    "SensorLib/src/REG"
    "SensorLib/src/touch"
    "SensorLib/src/platform"
    "SensorLib/src/bosch"
    "SensorLib/src/bosch/firmware"
    "SensorLib/src/bosch/BMM150"
    "SensorLib/src/bosch/common"
)

idf_component_register(
    SRC_DIRS
        "src"
        "codec_board"
        ${sensorlib_src_dirs}
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        "codec_board/include"
        ${sensorlib_include_dirs}
    REQUIRES
        driver
        esp_timer
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_i2s
        esp_driver_spi
        esp_driver_sdmmc
        esp_driver_sdspi
        espressif__esp_codec_dev
        esp_codec_dev
        esp_hw_support
        fatfs
        sdmmc
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_SENSORLIB_ESP_IDF_NEW_API=1)
