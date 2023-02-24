set(sources
    main.c
    networking.c
    storage.c
)

set(dependencies
    fatfs
    driver
    nvs_flash
    esp_netif
    console
    esptool_py
    esp_gdbstub
    esp_psram
    esp_http_server
    esp_wifi
    jimtcl
    compat
    platform_include
)

idf_component_register(
    SRCS
        ${sources}
    INCLUDE_DIRS
        "."
    PRIV_REQUIRES
        ${dependencies}
    EMBED_FILES "web_page.html"
)

set(host "esp-idf")
set(OPENOCD_DIR ${CMAKE_CURRENT_LIST_DIR}/openocd)
set(JIMTCL_DIR ${CMAKE_SOURCE_DIR}/components/jimtcl/jimtcl)
set(PLATFORM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/components/platform_include)

include(${OPENOCD_DIR}/cmake/OpenOCDSetup.cmake)
include(${OPENOCD_DIR}/cmake/configure-esp32.cmake)

add_library(openocd INTERFACE)

# Propogate config.h which is auto-generated into the root directory.
# Normally this should be done in the openocd/CmakeLists.txt.
# But we wanted to keep it simple to make the upstream process easier.
target_include_directories(openocd INTERFACE ${OPENOCD_DIR})

add_subdirectory(${OPENOCD_DIR}/src)

# Propogate platform_include folder to all openocd targets
# Normally this should be done in the openocd/src/CmakeLists.txt.
# But we wanted to keep it simple to make the upstream process easier.
target_include_directories(openocd_main BEFORE INTERFACE ${PLATFORM_INCLUDE_DIR})
target_link_libraries(openocd INTERFACE openocd_main)

target_include_directories(${COMPONENT_LIB} PRIVATE $<TARGET_PROPERTY:openocd_main,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(${COMPONENT_LIB} PUBLIC openocd)

include(${OPENOCD_DIR}/cmake/CreateTCL-lite.cmake)

fatfs_create_rawflash_image(storage openocd/tcl-lite FLASH_IN_PROJECT)

# custom target to load binaries from jtag.
find_program(OPENOCD_EXECUTABLE "openocd" PATHS "openocd-esp32/bin")
if (NOT OPENOCD_EXECUTABLE)
    message(WARNING "openocd-esp32 not found!")
else()
    message(STATUS "openocd-esp32 found in " ${OPENOCD_EXECUTABLE})
    set(app_bin ${CMAKE_PROJECT_NAME}.bin)
    set(OPENOCD_CONFIG board/esp32s3-builtin.cfg)
    # run "cmake --build build -t jtag-flasher-full"
    add_custom_target(jtag-flasher-full
        DEPENDS ${CMAKE_BINARY_DIR}/${app_bin}
        COMMAND ${OPENOCD_EXECUTABLE} -s $ENV{OPENOCD_SCRIPTS} -f ${OPENOCD_CONFIG}
        --command "program_esp_bins ${CMAKE_BINARY_DIR} flasher_args.json compress verify reset exit"
    )
    # run "cmake --build build -t jtag-flasher"
    add_custom_target(jtag-flasher
        DEPENDS ${CMAKE_BINARY_DIR}/${app_bin}
        COMMAND ${OPENOCD_EXECUTABLE} -s $ENV{OPENOCD_SCRIPTS} -f ${OPENOCD_CONFIG}
        --command "program_esp ${CMAKE_BINARY_DIR}/${app_bin} 0x10000 compress verify reset exit"
    )
endif()
