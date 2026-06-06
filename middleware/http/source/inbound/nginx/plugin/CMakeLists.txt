set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/nginx/sbin)

include(ExternalProject)
find_program(MAKE_EXECUTABLE NAMES gmake make mingw32-make REQUIRED)

set(EXTERNAL_PROJ_NAME "nginx")
set(EXTERNAL_INSTALL_DIR $ENV{CASUAL_HOME}/nginx)
set(NGINX_VERSION "nginx-1.28.0")

ExternalProject_Add(
    ${EXTERNAL_PROJ_NAME}
    
    # Källkodshantering (Antingen en URL, Git-repo, eller befintlig källkatalog)
    SOURCE_DIR          ${CASUAL_THIRDPARTY_SOURCE_DIR}/nginx/${NGINX_VERSION}
    
    # 1. Konfigurera (Kör ./configure i byggkatalogen)
    CONFIGURE_COMMAND   cd ${CASUAL_THIRDPARTY_SOURCE_DIR}/nginx/${NGINX_VERSION} && ./configure 
                        --with-debug
                        --builddir=build
                        --prefix=${CMAKE_CURRENT_BINARY_DIR}/nginx-build/
                        --with-cc-opt=-Wno-deprecated
                        --add-module=$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
                        --without-http_rewrite_module
    
    # 2. Bygg (Kör make i byggkatalogen)
    BUILD_COMMAND       cd ${CASUAL_THIRDPARTY_SOURCE_DIR}/nginx/${NGINX_VERSION} && ${MAKE_EXECUTABLE}
    
    # 3. Installera (Kör make install)
    INSTALL_COMMAND     cd ${CASUAL_THIRDPARTY_SOURCE_DIR}/nginx/${NGINX_VERSION} && ${MAKE_EXECUTABLE} install
    INSTALL_DIR         ${EXTERNAL_INSTALL_DIR}
    
    # Hoppa över standardteststeget
    TEST_COMMAND        ""

    DEPENDS
        casual-http-inbound-common
)

install(
    FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/nginx-build/sbin/nginx
    DESTINATION 
        nginx/sbin
    PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
)

install(
    DIRECTORY 
        ${CMAKE_CURRENT_BINARY_DIR}/nginx-build/html
    DESTINATION 
        nginx/
)

install(
    DIRECTORY 
        ${CMAKE_CURRENT_BINARY_DIR}/nginx-build/logs
    DESTINATION 
        nginx/
)

install(
    FILES 
        ${CMAKE_SOURCE_DIR}/thirdparty/nginx/nginx.conf
    DESTINATION nginx/conf
)