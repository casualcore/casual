cmake_minimum_required(VERSION 3.26.5)

project(casual VERSION 1.9 )

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX $ENV{CASUAL_HOME})
set(CMAKE_INSTALL_RPATH $ENV{CASUAL_HOME}/lib)


# Dependencies
find_package(pugixml REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)
find_package(cppcodec REQUIRED)
find_package(tomlplusplus REQUIRED)

include(FetchContent)
FetchContent_Declare(
  casual_thirdparty
  GIT_REPOSITORY https://github.com/casualcore/casual-thirdparty.git
  GIT_TAG        b58283e47a12a9bddcf9a2bb22b81145200b4dca # release-1.9
  SOURCE_DIR     casual-thirdparty
)

FetchContent_MakeAvailable( 
    casual_thirdparty 
)

set(CASUAL_THIRDPARTY_SOURCE_DIR "${casual_thirdparty_SOURCE_DIR}")
set(CASUAL_THIRDPARTY_BINARY_DIR "${casual_thirdparty_BINARY_DIR}")

# Set compiler options 
if (APPLE)
    add_compile_options(
        -Wall
        -Wextra
        -Werror
        -Wsign-compare
        -Wuninitialized
        -Winit-self
        -Wno-unused-parameter
        -Wno-missing-declarations
        -Wno-noexcept-type
        -Wno-missing-field-initializers
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -Werror
        -Wsign-compare
        -Wuninitialized
        -Winit-self
        -Wno-unused-parameter
        -Wno-missing-declarations
        -Wno-noexcept-type
        -Wno-implicit-fallthrough
        -Wno-missing-field-initializers
        -Wno-dangling-reference
        -Wno-unused-but-set-variable
        -Wno-return-type
        -Wno-address
        -fPIC
    )
endif()


if(NOT DEFINED ENV{CMAKE_SOURCE_DIR})
    message( SEND_ERROR "CMAKE_SOURCE_DIR need to be set" )
endif()

if(NOT DEFINED ENV{CMAKE_BINARY_DIR})
    message( SEND_ERROR "CMAKE_BINARY_DIR need to be set" )
endif()

if(NOT DEFINED ENV{CASUAL_HOME})
    message( WARNING "CASUAL_HOME need to be set when using install target" )
endif()

include(casual.cmake)

if(EXISTS "$ENV{CMAKE_BINARY_DIR}/version.cmake")
    include($ENV{CMAKE_BINARY_DIR}/version.cmake)
endif()

# Information for Packing
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "casual-middleware")
set(CPACK_PACKAGE_VENDOR "laz.se")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_URL "https://github.com/casualcore/casual")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Casual Middleware Software")
set(CPACK_PACKAGE_RELOCATABLE OFF)

set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/casual")
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
set(CPACK_RPM_PACKAGE_DESCRIPTION "This is the core package for casual-middleware")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/etc/bash_completion.d")
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/opt")

set(CPACK_RPM_SPEC_MORE_DEFINE "%define _build_id_links none")

file(WRITE ${CMAKE_BINARY_DIR}/changelog.txt
"* Tue Sep  22 2015  Fredrik Eriksson <lazan@laz.se> 
- Some insightful comment about first version of casuals
")

set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_BINARY_DIR}/changelog.txt")

set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")

set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_PACKAGE_ARCHITECTURE})

include(CPack)

enable_testing()

# Build subdirectories
add_subdirectory(middleware)
add_subdirectory(test)
add_subdirectory(documentation)

# Handle documentation target and reference dependecies to subtargets
add_custom_target(documentation 
    DEPENDS 
        documentation-buffer
        documentation-adminstration
        documentation-configuration
        documentation-gateway
)

