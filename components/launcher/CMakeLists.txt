file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/platform/*.cpp"
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "src"
    REQUIRES "wrapper-esp32"
)
