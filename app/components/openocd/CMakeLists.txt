idf_component_register(REQUIRES newlib pthread vfs lwip jimtcl
                       INCLUDE_DIRS openocd/src)

idf_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
set(interface_include_directories $<TARGET_PROPERTY:idf::openocd,INTERFACE_INCLUDE_DIRECTORIES>)

get_filename_component(gcc_name "${CMAKE_C_COMPILER}" NAME)
string(REPLACE "-gcc" "" host_prefix "${gcc_name}")


set(configure_opts "--host=${host_prefix}"
    "--disable-internal-libjaylink"
    "--disable-internal-jimtcl"
    "--enable-dummy"
    "--disable-ftdi"
    "--disable-stlink"
    "--disable-ti-icdi"
    "--disable-ulink"
    "--disable-usb-blaster-2"
    "--disable-ft232r"
    "--disable-vsllink"
    "--disable-xds110"
    "--disable-osbdm"
    "--disable-opendous"
    "--disable-aice"
    "--disable-usbprog"
    "--disable-armjtagew"
    "--disable-cmsis-dap"
    "--disable-usb-blaster"
    "--disable-openjtag"
    "--disable-presto"
    "--disable-kitprog"
    "--disable-rlink"
    "--disable-parport"
    "--disable-jlink"
    "--disable-target64")

string(CONCAT cflags "${CMAKE_C_FLAGS}"
                     " -I${COMPONENT_DIR}/../jimtcl/platform_include"
                     " -I$<JOIN:${interface_include_directories}, -I>"
                     " $<JOIN:${compile_options}, >"
                     " $<JOIN:${c_compile_options}, >"
                     " $<JOIN:${compile_definitions}, >"
                     " -Wno-implicit-fallthrough -Wno-missing-field-initializers"
                     " -Wno-redundant-decls -Wno-cast-function-type"
                     " -DHAVE_USLEEP=1")

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    include(ExternalProject)

    ExternalProject_Add(openocd_build
        PREFIX ${COMPONENT_DIR}
        SOURCE_DIR ${COMPONENT_DIR}/openocd
        CONFIGURE_COMMAND ./configure ${configure_opts} "CFLAGS=${cflags}"
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make src/libopenocd.la
        INSTALL_COMMAND ""
        )

    add_library(openocd_imp STATIC IMPORTED)
    add_dependencies(openocd_imp openocd_build)

    set_target_properties(openocd_imp PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/openocd/src/.libs/libopenocd.a)
    set_target_properties(openocd_imp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
        ${COMPONENT_DIR}/openocd/src)

     target_link_libraries(openocd_imp INTERFACE idf::newlib idf::compat idf::lwip idf::vfs idf::jimtcl)

    target_link_libraries(${COMPONENT_TARGET} INTERFACE openocd_imp)
endif()

add_custom_target(debug_openocd_opts COMMAND ${CMAKE_COMMAND} -E echo "${opts}")

spiffs_create_partition_image(storage openocd/tcl-lite FLASH_IN_PROJECT)
