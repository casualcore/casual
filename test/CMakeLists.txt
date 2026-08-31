set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/configuration/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/gateway/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/gateway/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/transaction/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/service/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/server/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/queue/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/queue/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/buffer/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../middleware/administration/unittest/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/xatmi/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/queue/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/gateway/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/http/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/administration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/cli/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../middleware/tools/bin>
)

add_executable(casual-test-mockup-string-server)

target_sources(casual-test-mockup-string-server
  PRIVATE
    source/mockup/string/server.cpp
)

target_link_libraries(casual-test-mockup-string-server PRIVATE
  casual-common
  casual-xatmi
  casual-service
  casual-server
)

add_executable(test-casual)

target_sources(test-casual
  PRIVATE
    unittest/source/test_configuration.cpp
    unittest/source/test_domain.cpp
    unittest/source/test_discovery.cpp
    unittest/source/xatmi/test_call.cpp
    unittest/source/xatmi/test_start.cpp
    unittest/source/test_transaction.cpp
    unittest/source/test_gateway.cpp
    unittest/source/gateway/test_reverse.cpp
    unittest/source/gateway/test_discovery.cpp
    unittest/source/test_service.cpp
    unittest/source/service/protocol/test_call.cpp
    unittest/source/test_server.cpp
    unittest/source/test_queue.cpp
    unittest/source/test_assassinate.cpp
    unittest/source/xatmi/test_conversation.cpp
)


target_link_libraries(test-casual PRIVATE
  casual-common
  casual-buffer
  casual-xatmi
  casual-xatmi-explicit
  casual-queue-unittest
  casual-queue-api
  casual-queue-common
  casual-unittest
  casual-mockup-rm
  casual-domain-unittest
  casual-domain-discovery-library
  casual-service
  casual-service-unittest
  casual-server
  casual-transaction-unittest
  casual-gateway-unittest
  casual-administration-unittest
  casual-configuration
  casual-domain-admin-cli
  casual-transaction
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-casual "" AUTO)

install(
    TARGETS
      casual-test-mockup-string-server
    DESTINATION
      test/bin
)

