cmake_minimum_required (VERSION 3.8)

project(euicc
        HOMEPAGE_URL "https://github.com/estkme-group/lpac"
        DESCRIPTION "libeuicc - eUICC Profile Manager Library"
        LANGUAGES C CXX)

set(FRAMEWORK_NAME "euicc")

enable_language(C)
enable_language(CXX)

add_subdirectory(${CMAKE_SOURCE_DIR}/../cjson ${CMAKE_BINARY_DIR}/cjson)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../euicc LIB_EUICC_SRCS)

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_SYSROOT "iphoneos")
set(CMAKE_OSX_ARCHITECTURES "x86_64" "arm64")
set(CMAKE_XCODE_ATTRIBUTE_SDKROOT "iphoneos")
set(CMAKE_XCODE_ATTRIBUTE_ARCHS "x86_64" "arm64")

add_library(${FRAMEWORK_NAME} SHARED ${LIB_EUICC_SRCS})

target_link_libraries(${FRAMEWORK_NAME} cjson-static)
target_include_directories(${FRAMEWORK_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

# Install headers
file(GLOB ALL_HEADERS "../euicc/*.h")

foreach(header ${ALL_HEADERS})
    if(${header} MATCHES "^.*\.private\.h$")
        list(REMOVE_ITEM ALL_HEADERS ${header})
    endif()
endforeach()

# Configure libeuicc iOS frmework
set(BUNDLE_RESOURCES
        ../euicc/LICENSE
        libeuicc.pc)

target_sources(${FRAMEWORK_NAME} PRIVATE "${ALL_HEADERS}")
target_sources(${FRAMEWORK_NAME} PRIVATE "${BUNDLE_RESOURCES}")

set_target_properties(${FRAMEWORK_NAME} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER me.estk.libeuicc
        RESOURCE "${BUNDLE_RESOURCES}"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${ALL_HEADERS}"
        RESOURCE "${BUNDLE_RESOURCES}"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")

# Install a pkg-config file
configure_file(../euicc/libeuicc.pc.in libeuicc.pc @ONLY)

set_target_properties(${FRAMEWORK_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Frameworks
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Frameworks
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Frameworks)

install(TARGETS ${FRAMEWORK_NAME}
        FRAMEWORK DESTINATION Frameworks)