set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
)

add_library(casual-service-manager-archive STATIC)

target_sources(casual-service-manager-archive
  PRIVATE
    source/manager/state.cpp
    source/manager/admin/server.cpp
    source/manager/transform.cpp
)

add_executable(casual-service-manager)

target_sources(casual-service-manager
  PRIVATE
    source/manager/main.cpp
    source/manager/handle.cpp
    source/manager/configuration.cpp
)

target_link_libraries(casual-service-manager PRIVATE
  casual-service-manager-archive
  casual-common
  casual-manager
  casual-domain-utility
  casual-domain-discovery-library
  casual-configuration
)


add_executable(casual-service-forward)

target_sources(casual-service-forward
  PRIVATE
    source/forward/main.cpp
    source/forward/handle.cpp
)

target_link_libraries(casual-service-forward PRIVATE
  casual-domain-utility
  casual-common
)


add_library(casual-service SHARED)

target_sources(casual-service
  PRIVATE
    source/call/context.cpp
    source/call/state.cpp
    source/conversation/context.cpp
    source/call.cpp
    source/lookup.cpp
)

target_link_libraries(casual-service PRIVATE
  casual-common
  casual-transaction
)


add_library(casual-service-admin-api SHARED)

target_sources(casual-service-admin-api
  PRIVATE
    source/manager/admin/api.cpp
)

target_link_libraries(casual-service-admin-api PRIVATE
  casual-service
  casual-common
  casual-transaction
)


add_library(casual-service-admin-cli SHARED)

target_sources(casual-service-admin-cli
  PRIVATE
    source/manager/admin/cli.cpp
)

target_link_libraries(casual-service-admin-cli PRIVATE
  casual-service-admin-api
  casual-service
  casual-common
)

add_library(casual-service-api SHARED)

target_sources(casual-service-api
  PRIVATE
    source/manager/api/state.cpp
    source/manager/api/model.cpp
)

target_link_libraries(casual-service-api PRIVATE
  casual-service-admin-api
  casual-common
)


add_library(casual-service-unittest SHARED)

target_sources(casual-service-unittest
  PRIVATE
    unittest/source/utility.cpp
)

target_link_libraries(casual-service-unittest PRIVATE
  casual-service
  casual-unittest
  casual-common
  casual-transaction
  gtest::gtest
)


add_executable(test-casual-service)

target_sources(test-casual-service
  PRIVATE
    unittest/source/test_forward.cpp
    unittest/source/manager/test_manager.cpp
    unittest/source/manager/test_state.cpp
    unittest/source/manager/test_api.cpp
)

target_link_libraries(test-casual-service PRIVATE
  casual-service-manager-archive
  casual-service
  casual-service-api
  casual-service-unittest
  casual-transaction
  casual-domain-utility
  casual-domain-discovery-library
  casual-common
  casual-manager
  casual-configuration
  casual-unittest
  casual-domain-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-casual-service "" AUTO)

install(
    TARGETS
      casual-service-manager
      casual-service-forward
    DESTINATION
      bin
)

install(
    TARGETS
      casual-service
      casual-service-admin-api
      casual-service-admin-cli
      casual-service-api
    DESTINATION
      lib
)

install(
    FILES
      include/casual/service/manager/api/model.h
      include/casual/service/manager/api/state.h
    DESTINATION
      include/casual/service/manager/api
)

