set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/server/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
)

add_library(casual-example-server-objects OBJECT)
target_sources(casual-example-server-objects
  PRIVATE
    server/source/example.cpp
    server/source/example/work.cpp
)

LinkServer(casual-example-server
  OBJECTS
    $<TARGET_OBJECTS:casual-example-server-objects>
  INCLUDE_PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  LIBRARY_PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  LIBRARIES
    casual-common
    casual-buffer
  SERVER_DEFINITION
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/source/example.services.yaml>
  CONFIGURATION_FILE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.yaml>
)

# make sure we got a resource property file available when we build with a resource 
#set(ENV{CASUAL_RESOURCE_CONFIGURATION_FILE} "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.yaml")


add_library(casual-example-resource-server-objects OBJECT)
target_sources(casual-example-resource-server-objects
  PRIVATE
    server/source/example/resource.cpp
)

LinkServer(casual-example-resource-server
  OBJECTS
    $<TARGET_OBJECTS:casual-example-resource-server-objects>
  INCLUDE_PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  LIBRARY_PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  LIBRARIES
    casual-common
    casual-buffer
  SERVER_DEFINITION
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/source/example/resource.services.yaml>
  CONFIGURATION_FILE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.yaml>
)

# same as above, but does not name the resource, hence we can configure multiple
# resources (of rm-mockup)
LinkServer(casual-example-dynamic-resource-server
  OBJECTS
    $<TARGET_OBJECTS:casual-example-resource-server-objects>
  INCLUDE_PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  LIBRARY_PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  LIBRARIES
    casual-common
    casual-buffer
  SERVER_DEFINITION
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/source/example/dynamic.resource.services.yaml>
  CONFIGURATION_FILE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.yaml>
)


add_executable(casual-example-error-server)

target_sources(casual-example-error-server
  PRIVATE
    server/source/example/error.cpp
)

target_link_libraries(casual-example-error-server PRIVATE
  casual-common
  casual-buffer
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/executable/bin)

add_library(dummy STATIC)

target_sources(dummy
  PRIVATE
    executable/source/example.cpp
)

target_include_directories(dummy PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unittest/bin)

add_executable(test-casual-example)

target_sources(test-casual-example
  PRIVATE
    server/unittest/source/test_example.cpp
)

target_include_directories(test-casual-example PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
)

target_link_directories(test-casual-example PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
)

target_link_libraries(test-casual-example PRIVATE
  casual-common
  casual-xatmi
  casual-unittest
  casual-domain-unittest
  casual-service-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_add_tests(test-casual-example "" AUTO)

install(
    TARGETS
      casual-example-error-server
    DESTINATION
      example/bin
)


# servers build with casual-build-server, require for some reason, to be installed as files
# TODO: Figure out why.
install(
 FILES    
   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/server/bin/$<TARGET_FILE_NAME:casual-example-server>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/server/bin/$<TARGET_FILE_NAME:casual-example-dynamic-resource-server>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/server/bin/$<TARGET_FILE_NAME:casual-example-resource-server>
 DESTINATION
    example/bin
 PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE
)