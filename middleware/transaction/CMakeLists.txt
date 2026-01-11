set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/database/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
)

add_library(casual-transaction SHARED)

target_sources(casual-transaction
  PRIVATE
    source/resource.cpp
    source/transaction.cpp
    source/context.cpp
)

target_link_libraries(casual-transaction PRIVATE
  casual-common
)


add_library(manager-archive STATIC)

target_sources(manager-archive
  PRIVATE
    source/manager/state.cpp
    source/manager/action.cpp
    source/manager/transform.cpp
    source/manager/configuration.cpp
    source/manager/log.cpp
    source/manager/handle.cpp
    source/manager/admin/server.cpp
    source/manager/admin/transform.cpp
)

add_executable(casual-transaction-manager)

target_sources(casual-transaction-manager
  PRIVATE
    source/manager/main.cpp
)

target_link_libraries(casual-transaction-manager PRIVATE
  manager-archive
  casual-common
  casual-manager
  casual-configuration
  casual-domain-utility
  sqlite3
)


add_library(casual-resource-proxy-server SHARED)

target_sources(casual-resource-proxy-server
  PRIVATE
    source/resource/proxy/server.cpp
    source/resource/proxy.cpp
)

target_link_libraries(casual-resource-proxy-server PRIVATE
  casual-transaction
  manager-archive
  casual-common
)


add_library(casual-mockup-rm SHARED)

target_sources(casual-mockup-rm
  PRIVATE
    unittest/source/rm.cpp
)

target_link_libraries(casual-mockup-rm PRIVATE
  casual-transaction
  casual-common
  casual-unittest
)


add_executable(rm-proxy-casual-mockup)

target_sources(rm-proxy-casual-mockup
  PRIVATE
    source/resource/template_build_resource_proxy.cpp
)

target_link_libraries(rm-proxy-casual-mockup PRIVATE
  casual-mockup-rm
  casual-resource-proxy-server
)


add_library(casual-transaction-unittest SHARED)

target_sources(casual-transaction-unittest
  PRIVATE
    unittest/source/utility.cpp
)


target_link_libraries(casual-transaction-unittest PRIVATE
  casual-unittest
  casual-common
  gtest::gtest
)


add_executable(test-casual-transaction)

target_sources(test-casual-transaction
  PRIVATE
    unittest/source/test_context.cpp
    unittest/source/manager/test_state.cpp
    unittest/source/manager/test_log.cpp
    unittest/source/test_admin.cpp
    unittest/source/test_manager.cpp
)

target_link_libraries(test-casual-transaction PRIVATE
  casual-transaction
  manager-archive
  casual-transaction-unittest
  casual-mockup-rm
  casual-common
  casual-configuration
  casual-domain-utility
  sqlite3
  casual-unittest
  casual-domain-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-casual-transaction "" AUTO)

add_library(casual-transaction-admin-cli SHARED)

target_sources(casual-transaction-admin-cli
  PRIVATE
    source/manager/admin/cli.cpp
)

target_link_libraries(casual-transaction-admin-cli PRIVATE
  casual-transaction
  manager-archive
  casual-cli
  casual-domain-utility
  casual-service
  casual-common
)

install(
    TARGETS
      casual-transaction-manager
      rm-proxy-casual-mockup
    DESTINATION
      bin
)

install(
    TARGETS
      casual-transaction
      casual-resource-proxy-server
      casual-mockup-rm
    DESTINATION
      lib
)

install(
    FILES
      include/casual/transaction/resource/proxy/server.h
    DESTINATION
      include/casual/transaction/resource/proxy
)

install(
    TARGETS
      casual-transaction-admin-cli
    DESTINATION
      lib
)

