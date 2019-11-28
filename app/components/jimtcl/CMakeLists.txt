idf_component_register(REQUIRES newlib pthread vfs
                       INCLUDE_DIRS jimtcl)

idf_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
set(interface_include_directories $<TARGET_PROPERTY:idf::jimtcl,INTERFACE_INCLUDE_DIRECTORIES>)

set (opts "${CMAKE_C_FLAGS}")
string(APPEND opts " -I${COMPONENT_DIR}/platform_include")
string(APPEND opts " -I$<JOIN:${interface_include_directories}, -I>")
string(APPEND opts " $<JOIN:${compile_options}, >")
string(APPEND opts " $<JOIN:${c_compile_options}, >")
string(APPEND opts " $<JOIN:${compile_definitions}, >")
string(APPEND opts " -Wno-implicit-fallthrough -Wno-missing-field-initializers")
string(APPEND opts " -DHAVE_USLEEP=1")

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    include(ExternalProject)
    get_filename_component(gcc_name "${CMAKE_C_COMPILER}" NAME)
    string(REPLACE "-gcc" "" host_prefix "${gcc_name}")

    ExternalProject_Add(jimtcl_build
        PREFIX ${COMPONENT_DIR}
        SOURCE_DIR ${COMPONENT_DIR}/jimtcl
        CONFIGURE_COMMAND ./configure --host=${host_prefix} "CFLAGS=${opts}"
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make libjim.a initjimsh.o
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS ${COMPONENT_DIR}/jimtcl/libjim.a ${COMPONENT_DIR}/jimtcl/initjimsh.o
        )

    add_library(jim_imp STATIC IMPORTED)
    add_dependencies(jim_imp jimtcl_build)

    set_target_properties(jim_imp PROPERTIES IMPORTED_LOCATION
        ${COMPONENT_DIR}/jimtcl/libjim.a)
    set_target_properties(jim_imp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
        ${COMPONENT_DIR}/jimtcl)
    set_target_properties(jim_imp PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES
        "idf::newlib;idf::compat")
    set_target_properties(jim_imp PROPERTIES ADDITIONAL_CLEAN_FILES
        ${COMPONENT_DIR}/jimtcl/libjim.a)

    add_library(initjimsh STATIC IMPORTED)
    add_dependencies(initjimsh jimtcl_build)
    set_target_properties(initjimsh PROPERTIES IMPORTED_LOCATION
        ${COMPONENT_DIR}/jimtcl/initjimsh.o)
    set_target_properties(initjimsh PROPERTIES ADDITIONAL_CLEAN_FILES
        ${COMPONENT_DIR}/jimtcl/initjimsh.o)

    target_link_libraries(jim_imp INTERFACE idf::newlib idf::compat)

    target_link_libraries(${COMPONENT_TARGET} INTERFACE jim_imp initjimsh)
endif()

add_custom_target(debug_jimtcl_opts COMMAND ${CMAKE_COMMAND} -E echo "${opts}")
