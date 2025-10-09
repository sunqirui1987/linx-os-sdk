set(FONTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/third/fonts")

# Collect fonts source files
file(GLOB_RECURSE FONTS_SRCS "${FONTS_ROOT}/src/*.c")

# Exclude test files
list(FILTER FONTS_SRCS EXCLUDE REGEX ".*test.*\\.c$")

idf_component_register(
    SRCS ${FONTS_SRCS}
    INCLUDE_DIRS "${FONTS_ROOT}/include"
    REQUIRES liblvgl
)