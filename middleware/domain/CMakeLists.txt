set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../buffer/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
)

add_library(casual-domain-manager-archive STATIC)

target_sources(casual-domain-manager-archive
  PRIVATE
    source/manager/transform.cpp
    source/manager/task/create.cpp
    source/manager/state.cpp
    source/manager/state/order.cpp
    source/manager/handle.cpp
    source/manager/admin/server.cpp
    source/manager/configuration.cpp
)

add_library(casual-domain-utility SHARED)

target_sources(casual-domain-utility
  PRIVATE
    source/configuration.cpp
)

target_link_libraries(casual-domain-utility PRIVATE
  casual-common
)


add_executable(casual-domain-manager)

target_sources(casual-domain-manager
  PRIVATE
    source/manager/main.cpp
)

target_link_libraries(casual-domain-manager PRIVATE
  casual-domain-manager-archive  
  casual-domain-utility
  casual-common
  casual-manager
  casual-configuration
)


add_library(casual-domain-manager-internal-api STATIC)

target_sources(casual-domain-manager-internal-api
  PRIVATE
    source/manager/api/internal/transform.cpp
)


add_library(casual-domain-discovery-archive STATIC)

target_sources(casual-domain-discovery-archive
  PRIVATE
    source/discovery/state.cpp
    source/discovery/instance.cpp
)


add_executable(casual-domain-discovery)

target_sources(casual-domain-discovery
  PRIVATE
    source/discovery/main.cpp
    source/discovery/handle.cpp
    source/discovery/admin/server.cpp
    source/discovery/admin/transform.cpp
)

target_link_libraries(casual-domain-discovery PRIVATE
  casual-domain-discovery-archive
  casual-domain-utility
  casual-common
  casual-manager
)


add_library(casual-domain-discovery-library SHARED)

target_sources(casual-domain-discovery-library
  PRIVATE
    source/discovery/api.cpp
)


target_link_libraries(casual-domain-discovery-library PRIVATE
  casual-domain-discovery-archive
  casual-common
)


add_library(casual-domain-unittest SHARED)

target_sources(casual-domain-unittest
  PRIVATE
    unittest/source/manager.cpp
    unittest/source/configuration.cpp
    unittest/source/discover.cpp
    unittest/source/utility.cpp
)


target_link_libraries(casual-domain-unittest PRIVATE
  casual-domain-discovery-library
  casual-common
  casual-unittest
  gtest::gtest
)


add_executable(test-simple-server)

target_sources(test-simple-server
  PRIVATE
    unittest/source/simple_server.cpp
)

target_link_libraries(test-simple-server PRIVATE
  casual-common
  casual-unittest
)


add_executable(test-casual-domain)

target_sources(test-casual-domain
  PRIVATE
    unittest/source/manager/test_state.cpp
    unittest/source/manager/test_manager.cpp
    unittest/source/manager/test_api.cpp
    unittest/source/test_discovery.cpp
)

target_link_libraries(test-casual-domain PRIVATE
  casual-domain-manager-archive
  casual-domain-discovery-library
  casual-domain-unittest
  casual-domain-utility
  casual-domain-manager-internal-api
  casual-common
  casual-configuration
  casual-unittest
  casual-configuration-example
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_add_tests(test-casual-domain "" AUTO)

add_library(casual-domain-admin-cli SHARED)

target_sources(casual-domain-admin-cli
  PRIVATE
    source/manager/admin/cli.cpp
)


target_link_libraries(casual-domain-admin-cli PRIVATE
  casual-domain-manager-archive
  casual-common
  casual-configuration
  casual-configuration-cli
  casual-service
  casual-transaction
)


add_library(casual-domain-api SHARED)

target_sources(casual-domain-api
  PRIVATE
    source/manager/api/state.cpp
)

target_link_libraries(casual-domain-api PRIVATE
  casual-domain-manager-internal-api
  casual-common
)


add_library(casual-domain-discovery-admin-cli SHARED)

target_sources(casual-domain-discovery-admin-cli
  PRIVATE
    source/discovery/admin/cli.cpp
)

target_link_libraries(casual-domain-discovery-admin-cli PRIVATE
  casual-domain-discovery-library
  casual-domain-discovery-archive
  casual-common
  casual-service
  casual-transaction
)

install(
    TARGETS
      casual-domain-manager
      casual-domain-discovery
    DESTINATION
      bin
)

install(
    TARGETS
      casual-domain-utility
      casual-domain-discovery-library
    DESTINATION
      lib
)

install(
    TARGETS
      casual-domain-admin-cli
      casual-domain-api
      casual-domain-discovery-admin-cli
    DESTINATION
      lib
)

install(
    FILES
      include/casual/domain/manager/api/model.h
      include/casual/domain/manager/api/state.h
    DESTINATION
      include/casual/domain/manager/api
)

