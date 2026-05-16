set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../server/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/database/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/unittest/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../queue/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../cli/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
)

add_library(casual-queue-common SHARED)

target_sources(casual-queue-common
  PRIVATE
    source/common/log.cpp
    source/common/queue.cpp
)

target_link_libraries(casual-queue-common PRIVATE
  casual-common
)


add_library(casual-queue-group-archive STATIC)

target_sources(casual-queue-group-archive
  PRIVATE
    source/group/state.cpp
    source/group/queuebase.cpp
    source/group/queuebase/statement.cpp
    source/group/handle.cpp
)

add_executable(casual-queue-group)

target_sources(casual-queue-group
  PRIVATE
    source/group/main.cpp
)

target_link_libraries(casual-queue-group PRIVATE
  casual-queue-group-archive
  casual-queue-common
  casual-domain-utility
  casual-common
  casual-domain-discovery-library
  sqlite3
)


add_library(casual-queue-manager-archive STATIC)

target_sources(casual-queue-manager-archive
  PRIVATE
    source/manager/transform.cpp
    source/manager/configuration.cpp
    source/manager/handle.cpp
    source/manager/state.cpp
    source/manager/admin/server.cpp
)

add_executable(casual-queue-manager)

target_sources(casual-queue-manager
  PRIVATE
    source/manager/main.cpp
)

target_link_libraries(casual-queue-manager PRIVATE
  casual-queue-manager-archive
  casual-queue-common
  casual-common
  casual-manager
  casual-domain-utility
  casual-domain-discovery-library
  casual-configuration
)


add_library(casual-queue-api SHARED)

target_sources(casual-queue-api
  PRIVATE
    source/api/queue.cpp
)

target_link_libraries(casual-queue-api PRIVATE
  casual-queue-common
  casual-transaction
  casual-service
  casual-common
)


add_library(casual-queue-c-api SHARED)

target_sources(casual-queue-c-api
  PRIVATE
    source/c/queue.cpp
)

target_link_libraries(casual-queue-c-api PRIVATE
  casual-queue-api
  casual-queue-common
  casual-common
)


add_executable(casual-queue-forward-group)

target_sources(casual-queue-forward-group
  PRIVATE
    source/forward/handle.cpp
    source/forward/state.cpp
    source/forward/main.cpp
)

target_link_libraries(casual-queue-forward-group PRIVATE
  casual-queue-common
  casual-common
  casual-buffer
  casual-configuration
)

# fanout
add_executable(casual-queue-fanout-group)

target_sources(casual-queue-fanout-group
  PRIVATE
    source/fanout/handle.cpp
    source/fanout/state.cpp
    source/fanout/main.cpp
)

target_link_libraries(casual-queue-fanout-group PRIVATE
  casual-queue-common
  casual-common
  casual-configuration
)

add_executable(casual-queue-example-server)

target_sources(casual-queue-example-server
  PRIVATE
    source/example/main.cpp
)

target_link_libraries(casual-queue-example-server PRIVATE
  casual-queue-common
  casual-queue-api
  casual-queue-c-api
  casual-service
  casual-server
  casual-transaction
  casual-common
  casual-buffer
)


add_library(casual-queue-admin-cli SHARED)

target_sources(casual-queue-admin-cli
  PRIVATE
    source/manager/admin/cli.cpp
)

target_link_libraries(casual-queue-admin-cli PRIVATE
  casual-queue-common
  casual-queue-api
  casual-service
  casual-cli
  casual-common
)

add_library( casual-queue-upgrade-archive STATIC)

target_sources( casual-queue-upgrade-archive
  PRIVATE
    source/group/queuebase/upgrade.cpp
)

add_executable( casual-queue-upgrade)

target_sources(casual-queue-upgrade
  PRIVATE
    tools/source/upgrade.cpp
)

target_link_libraries(casual-queue-upgrade PRIVATE
  casual-queue-upgrade-archive
  casual-queue-common
  casual-common
  sqlite3
)


add_library(casual-queue-unittest SHARED)

target_sources(casual-queue-unittest
  PRIVATE
    unittest/source/utility.cpp
)

target_link_libraries(casual-queue-unittest PRIVATE
  casual-queue-common
  casual-common
  casual-unittest
  casual-service
  casual-service-unittest
  gtest::gtest
)


add_executable(test-casual-queue)

target_sources(test-casual-queue
  PRIVATE
    unittest/source/manager/test_transform.cpp
    unittest/source/group/test_queuebase.cpp
    unittest/source/test_queue.cpp
    unittest/source/api/test_c.cpp
    unittest/source/test_forward.cpp
    unittest/source/test_fanout.cpp
    unittest/source/test_example.cpp
)

target_link_libraries(test-casual-queue PRIVATE
  casual-queue-group-archive
  casual-queue-manager-archive
  casual-queue-common
  casual-queue-api
  casual-queue-c-api
  casual-queue-upgrade-archive
  casual-queue-unittest
  casual-common
  casual-configuration
  casual-transaction
  casual-service
  sqlite3
  casual-unittest
  casual-service-unittest
  casual-transaction-unittest
  casual-domain-discovery-library
  casual-domain-unittest
  casual-xatmi
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-casual-queue "" AUTO)

add_library(casual-queue-forward-common STATIC)

target_sources(casual-queue-forward-common
  PRIVATE
    source/forward/deprecated/common.cpp
)

add_executable(casual-queue-forward-service)

target_sources(casual-queue-forward-service
  PRIVATE
    source/forward/deprecated/service.cpp
)

target_link_libraries(casual-queue-forward-service PRIVATE
  casual-queue-forward-common
  casual-queue-common
  casual-queue-api
  casual-common
  casual-buffer
  casual-service
  casual-transaction
)


add_executable(casual-queue-forward-queue)

target_sources(casual-queue-forward-queue
  PRIVATE
    source/forward/deprecated/queue.cpp
)

target_link_libraries(casual-queue-forward-queue PRIVATE
  casual-queue-forward-common
  casual-queue-common
  casual-queue-api
  casual-common
  casual-buffer
  casual-service
  casual-transaction
)

install(
    FILES
      include/casual/queue/c/queue.h    
    DESTINATION
      include/casual/queue/c/
)

install(
    TARGETS
      casual-queue-common
      casual-queue-api
      casual-queue-c-api
      casual-queue-admin-cli
    DESTINATION
      lib
)

install(
    TARGETS
      casual-queue-group
      casual-queue-manager
      casual-queue-forward-group
      casual-queue-example-server
      casual-queue-upgrade
      casual-queue-forward-service
      casual-queue-forward-queue
      casual-queue-fanout-group
    DESTINATION
      bin
)

