
file(GLOB_RECURSE SOURCES_C *.c)
file(GLOB_RECURSE SOURCES_CPP *.cpp)

idf_component_register(
    SRCS ${SOURCES_C} ${SOURCES_CPP} 
    INCLUDE_DIRS "." 
    REQUIRES nvs_flash esp_wifi esp_netif esp_event esp_http_client json esp-tls mbedtls lwip lvgl
    )
