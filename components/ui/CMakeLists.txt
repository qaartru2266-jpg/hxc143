file(GLOB_RECURSE UI_SRCS "*.c" "*.cpp")
list(APPEND UI_SRCS "actions.c")

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "."
    REQUIRES lvgl
    PRIV_REQUIRES application
)
