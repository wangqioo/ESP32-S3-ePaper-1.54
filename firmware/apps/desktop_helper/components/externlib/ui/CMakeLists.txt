idf_component_register(
  SRCS "FacTest_ui.c"
  INCLUDE_DIRS "./"
  REQUIRES lvgl ui_res
)
#add_compile_definitions(XPOWERS_CHIP_AXP2101 CONFIG_XPOWERS_ESP_IDF_NEW_API)
##REQUIRES