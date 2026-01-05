file(GLOB_RECURSE UI_SRCS "*.c")

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "."
    REQUIRES lvgl
)
