idf_component_register(
  SRCS "user_app.cpp"
  PRIV_REQUIRES
  port_bsp
  ui
  driver
  my_button
  codec_board
  waveshare__pcf85063a
  REQUIRES
  INCLUDE_DIRS ".")
