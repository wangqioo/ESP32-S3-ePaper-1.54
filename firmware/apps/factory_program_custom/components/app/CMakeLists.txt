idf_component_register(
  SRCS
  "user_app.cpp"
  "memo_utils.cpp"
  "memo_storage.cpp"
  "memo_recorder.cpp"
  "memo_player.cpp"
  "voice_ui.cpp"
  PRIV_REQUIRES
  port_bsp
  ui
  driver
  esp_timer
  my_button
  codec_board
  waveshare__pcf85063a
  REQUIRES
  INCLUDE_DIRS ".")
