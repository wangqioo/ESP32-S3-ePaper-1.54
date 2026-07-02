idf_component_register(
    SRCS
    "main.cpp"
    PRIV_REQUIRES
    app
    esp_timer
    lvgl__lvgl
    nvs_flash
    port_bsp
    INCLUDE_DIRS
    "./")

