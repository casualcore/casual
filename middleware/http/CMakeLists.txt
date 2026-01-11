set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../buffer/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../http/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
)

add_library(casual-http-common STATIC)

target_sources(casual-http-common
  PRIVATE
    source/common.cpp
    source/code.cpp
)

add_library(casual-http-outbound-common STATIC)

target_sources(casual-http-outbound-common
  PRIVATE
    source/outbound/configuration.cpp
    source/outbound/transform.cpp
    source/outbound/state.cpp
    source/outbound/request.cpp
    source/outbound/curl.cpp
)

add_library(casual-http-inbound-common SHARED)

target_sources(casual-http-inbound-common
  PRIVATE
    source/inbound/c/api.cpp
    source/inbound/call.cpp
)

target_link_libraries(casual-http-inbound-common PRIVATE
  casual-buffer
  casual-xatmi
  casual-common
  casual-http-common
)

add_executable(casual-http-outbound)

target_sources(casual-http-outbound
  PRIVATE
    source/outbound/main.cpp
    source/outbound/handle.cpp
)

target_link_libraries(casual-http-outbound PRIVATE
  casual-http-outbound-common
  casual-http-common
  casual-domain-discovery-library
  casual-common
  curl
)

add_executable(test-http)

target_sources(test-http
  PRIVATE
    unittest/source/outbound/test_model.cpp
    unittest/source/outbound/test_transcode.cpp
    unittest/source/outbound/test_state.cpp
    unittest/source/outbound/test_request.cpp
    unittest/source/inbound/test_call.cpp
    unittest/source/test_outbound.cpp
    unittest/source/test_code.cpp
)

target_link_libraries(test-http PRIVATE
  casual-http-outbound-common
  casual-http-inbound-common
  casual-http-common
  casual-common
  casual-domain-unittest
  casual-service-unittest
  casual-unittest
  casual-xatmi
  casual-domain-discovery-library
  gtest::gtest
  gtest_main
  curl
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-http "" AUTO)

install(
    TARGETS
      casual-http-outbound
    DESTINATION
      bin
)

install(
    TARGETS
      casual-http-inbound-common
    DESTINATION
      lib
)

