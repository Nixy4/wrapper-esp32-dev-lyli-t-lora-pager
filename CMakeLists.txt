cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wrapper-esp32-dev-lyli-t-lora-pager)

# set(SPIFFS_ELF_SRC "${CMAKE_SOURCE_DIR}/test/hello_world.app.elf")
# set(SPIFFS_IMAGE_DIR "${CMAKE_BINARY_DIR}/spiffs_image")
# set(SPIFFS_ELF_DST "${SPIFFS_IMAGE_DIR}/hello_world.app.elf")

# add_custom_command(
# 	OUTPUT ${SPIFFS_ELF_DST}
# 	COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIFFS_IMAGE_DIR}
# 	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPIFFS_ELF_SRC} ${SPIFFS_ELF_DST}
# 	DEPENDS ${SPIFFS_ELF_SRC}
# 	COMMENT "Copy hello_world.app.elf into SPIFFS image directory"
# )

# add_custom_target(copy_elf_to_spiffs ALL DEPENDS ${SPIFFS_ELF_DST})

# spiffs_create_partition_image(storage ${SPIFFS_IMAGE_DIR} FLASH_IN_PROJECT DEPENDS copy_elf_to_spiffs)
