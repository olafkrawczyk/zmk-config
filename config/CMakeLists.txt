# Source the custom status screen and user modules
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# Force compile custom status screen if Display and LVGL are enabled.
# The Kconfig choice symbol ZMK_DISPLAY_STATUS_SCREEN_CUSTOM seems to be unreliable
# or overridden by the shield's default choice in some contexts.
if(CONFIG_ZMK_DISPLAY AND CONFIG_LVGL)
  zephyr_library_sources(custom_status_screen.c)
endif()
zephyr_library_sources(layer_display_state.c)

if(CONFIG_ZMK_KEYMAP)
  zephyr_library_sources(behavior_layer_display.c)
endif()

zephyr_library_sources_ifdef(CONFIG_ZMK_LAYER_DISPLAY_RELAY layer_display_relay.c)
