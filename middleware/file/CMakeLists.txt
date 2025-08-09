set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../queue/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../file/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../cli/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../queue/bin>
)

add_library(casual-file-common STATIC)

target_sources(casual-file-common
  PRIVATE
    source/instance.cpp
)

add_executable(casual-file-manager)

target_sources(casual-file-manager
  PRIVATE
    source/manager/main.cpp
    source/manager/handle.cpp
    source/manager/resource.cpp
    source/manager/state.cpp
    source/manager/admin/server.cpp
)

target_link_libraries(casual-file-manager PRIVATE
  casual-common
  casual-manager
)


add_library(casual-file-api SHARED)

target_sources(casual-file-api
  PRIVATE
    source/code.cpp
    source/api/file.cpp
)

target_link_libraries(casual-file-api PRIVATE
  casual-file-common
  casual-common
  casual-transaction
)


add_library(casual-file-c-api SHARED)

target_sources(casual-file-c-api
  PRIVATE
    source/api/c/file.cpp
)


target_link_libraries(casual-file-c-api PRIVATE
  casual-file-api
  casual-common
)


add_library(casual-file-admin-cli SHARED)

target_sources(casual-file-admin-cli
  PRIVATE
    source/manager/admin/cli.cpp
)

target_link_libraries(casual-file-admin-cli PRIVATE
  casual-file-api
  casual-cli
  casual-common
  casual-service
)


add_executable(test-casual-file)

target_sources(test-casual-file
  PRIVATE
    unittest/source/resource/utility.cpp
    unittest/source/resource/test_resource.cpp
)

target_link_libraries(test-casual-file PRIVATE
  casual-file-api
  casual-common
  casual-unittest
  casual-domain-unittest
  casual-xatmi
  casual-service
  casual-service-unittest
  casual-transaction
  casual-queue-api
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_add_tests(test-casual-file "" AUTO)

install(
    TARGETS
      casual-file-api
      casual-file-c-api
      casual-file-admin-cli
    DESTINATION
      lib
)

install(
    TARGETS
      casual-file-manager
    DESTINATION
      bin
)

