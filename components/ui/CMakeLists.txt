if(CMAKE_SCRIPT_MODE_FILE)
    # component_get_requirements.cmake runs in script mode; CONFIGURE_DEPENDS is invalid there.
    file(GLOB_RECURSE UI_SRCS "ui_font_alibaba_pu_hui_ti_3_55_regular.c" "eez-flow.cpp" "ui_image_qd.c" "*.c" "*.cpp")
else()
    file(GLOB_RECURSE UI_SRCS CONFIGURE_DEPENDS "*.c" "*.cpp")
endif()
list(APPEND UI_SRCS "actions.c")

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "."
    REQUIRES lvgl
    PRIV_REQUIRES application
)
