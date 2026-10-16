if(CONFIG_ZMK_DISPLAY)
  zephyr_library()
  zephyr_library_sources(behaviors/behavior_display_theme.c)
endif()
