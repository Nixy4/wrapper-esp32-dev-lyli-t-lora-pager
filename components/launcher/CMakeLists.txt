set(INCLUDE_DIRS "include" "src")

file(GLOB_RECURSE SOURCES "src/*.cpp")

# Public interface deps (headers exposed by our include/)
set(REQUIRES
    wrapper-esp32
)

# Private deps (used only inside our src/ — not exposed in public headers)
set(PRIV_REQUIRES
    nvs_flash
    app_update
    bootloader_support
    esp_partition
    spi_flash
    sdmmc
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${REQUIRES}
    PRIV_REQUIRES ${PRIV_REQUIRES}
)
