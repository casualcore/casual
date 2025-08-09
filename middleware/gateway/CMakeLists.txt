set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../queue/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/unittest/include>)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../gateway/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../queue/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
)

add_library(casual-gateway-common STATIC)

target_sources(casual-gateway-common
  PRIVATE
    source/group/tcp.cpp
    source/group/tcp/logical/connect.cpp
    source/message/protocol/version.cpp
)

add_executable(casual-gateway-manager)

target_sources(casual-gateway-manager
  PRIVATE
    source/manager/main.cpp
    source/manager/handle.cpp
    source/manager/admin/server.cpp
    source/manager/transform.cpp
    source/manager/configuration.cpp
    source/manager/state.cpp
)

target_link_libraries(casual-gateway-manager PRIVATE
  casual-gateway-common
  casual-manager
  casual-common
  casual-domain-utility
)


add_library(casual-gateway-admin-cli SHARED)

target_sources(casual-gateway-admin-cli
  PRIVATE
    source/manager/admin/cli.cpp
)


target_link_libraries(casual-gateway-admin-cli PRIVATE
  casual-gateway-common
  casual-service
  casual-transaction
  casual-common
)


add_library(casual-gateway-outbound-archive STATIC)

target_sources(casual-gateway-outbound-archive
  PRIVATE
    source/group/outbound/state.cpp
    source/group/outbound/handle.cpp
)


add_executable(casual-gateway-outbound-group)

target_sources(casual-gateway-outbound-group
  PRIVATE
    source/group/outbound/main.cpp
)

target_link_libraries(casual-gateway-outbound-group PRIVATE
  casual-gateway-outbound-archive
  casual-gateway-common
  casual-domain-discovery-library
  casual-common
)


add_executable(casual-gateway-outbound-reverse-group)

target_sources(casual-gateway-outbound-reverse-group
  PRIVATE
    source/group/outbound/reverse/main.cpp
)

target_link_libraries(casual-gateway-outbound-reverse-group PRIVATE
  casual-gateway-outbound-archive
  casual-gateway-common
  casual-domain-discovery-library
  casual-common
)


add_library(casual-gateway-inbound-archive STATIC)

target_sources(casual-gateway-inbound-archive
  PRIVATE
    source/group/inbound/handle.cpp
    source/group/inbound/state.cpp
    source/group/inbound/task/create.cpp
)


add_executable(casual-gateway-inbound-group)

target_sources(casual-gateway-inbound-group
  PRIVATE
    source/group/inbound/main.cpp
)

target_link_libraries(casual-gateway-inbound-group PRIVATE
  casual-gateway-inbound-archive
  casual-gateway-common
  casual-domain-discovery-library
  casual-common
)


add_executable(casual-gateway-inbound-reverse-group)

target_sources(casual-gateway-inbound-reverse-group
  PRIVATE
    source/group/inbound/reverse/main.cpp
)

target_link_libraries(casual-gateway-inbound-reverse-group PRIVATE
  casual-gateway-inbound-archive
  casual-gateway-common
  casual-domain-discovery-library
  casual-common
)


add_executable(casual-gateway-group-tcp-logical-connect)

target_sources(casual-gateway-group-tcp-logical-connect
  PRIVATE
    source/group/tcp/logical/connect/main.cpp
)

target_link_libraries(casual-gateway-group-tcp-logical-connect PRIVATE
  casual-gateway-common
  casual-common
)


add_library(casual-gateway-unittest SHARED)

target_sources(casual-gateway-unittest
  PRIVATE
    unittest/source/utility.cpp
)

target_link_libraries(casual-gateway-unittest PRIVATE
  casual-gateway-common
  casual-unittest
  casual-common
  casual-service
  casual-service-unittest
  gtest::gtest
)


add_executable(test-casual-gateway)

target_sources(test-casual-gateway
  PRIVATE
    unittest/source/test_manager.cpp
    unittest/source/test_protocol.cpp
    unittest/source/group/outbound/test_state.cpp
    unittest/source/group/inbound/test_state.cpp
    unittest/source/test_reverse.cpp
    unittest/source/protocol/1.5/test_manager.cpp
    unittest/source/protocol/1.4/test_manager.cpp
    unittest/source/protocol/1.2/test_manager.cpp
    unittest/source/protocol/1.0/test_manager.cpp
    source/documentation/protocol/example.cpp
)

target_link_libraries(test-casual-gateway PRIVATE
  casual-gateway-unittest
  casual-gateway-outbound-archive
  casual-gateway-inbound-archive
  casual-gateway-common
  casual-transaction
  casual-service
  casual-queue-api
  casual-common
  casual-domain-discovery-library
  casual-configuration
  casual-unittest
  casual-domain-unittest
  casual-service-unittest
  casual-transaction-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_add_tests(test-casual-gateway "" AUTO)

add_executable(casual-gateway-markdown-protocol)

target_sources(casual-gateway-markdown-protocol
  PRIVATE
    source/documentation/protocol/markdown.cpp
    source/documentation/protocol/example.cpp
)

target_link_libraries(casual-gateway-markdown-protocol PRIVATE
  casual-gateway-common
  casual-common
)


add_executable(casual-gateway-binary-protocol)

target_sources(casual-gateway-binary-protocol
  PRIVATE
    source/documentation/protocol/binary.cpp
    source/documentation/protocol/example.cpp
)

target_link_libraries(casual-gateway-binary-protocol PRIVATE
  casual-gateway-common
  casual-common
)

install(
    TARGETS
      casual-gateway-markdown-protocol
      casual-gateway-binary-protocol
    DESTINATION
      bin
)

install(
    TARGETS
      casual-gateway-manager
      casual-gateway-outbound-group
      casual-gateway-outbound-reverse-group
      casual-gateway-inbound-group
      casual-gateway-inbound-reverse-group
      casual-gateway-group-tcp-logical-connect
    DESTINATION
      bin
)

install(
    TARGETS
      casual-gateway-admin-cli
    DESTINATION
      lib
)

# Documentation
add_custom_target(documentation-gateway
    #COMMAND echo "Generating documentation for gateway"
    COMMAND mkdir -p $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/documentation/protocol/
    COMMAND
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/bin/casual-gateway-markdown-protocol > $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/documentation/protocol/protocol.maintenance.md
      --root $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/documentation
    VERBATIM
    DEPENDS
      casual-gateway-markdown-protocol
)

