
file(GLOB_RECURSE SOURCES_C *.c)
file(GLOB_RECURSE SOURCES_CPP *.cpp)

idf_component_register(
    SRCS ${SOURCES_C} ${SOURCES_CPP} 
    INCLUDE_DIRS "." 
    REQUIRES esp_lcd driver lvgl_esp32_drivers spiffs esp_wifi esp_netif esp_event nvs_flash
    )

# spiffs_create_partition_image(storage ../qr_data FLASH_IN_PROJECT)
