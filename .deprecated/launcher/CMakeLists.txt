file(GLOB_RECURSE SOURCES
    "src/core/*.cpp"
    "src/hal/*.cpp"
    "src/osal/*.cpp"
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "src"
    REQUIRES "nix-wrapper-esp-idf"
)
