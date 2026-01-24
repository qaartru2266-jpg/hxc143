file(GLOB_RECURSE UI_SRCS "*.c" "*.cpp")

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "."
    REQUIRES lvgl
)
