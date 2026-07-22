set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_library(casual-common SHARED)

target_sources(casual-common
  PRIVATE
    source/assert.cpp
    source/algorithm/random.cpp
    source/strong/id.cpp
    source/flag/xa.cpp
    source/flag/xatmi.cpp
    source/service/type.cpp
    source/header.cpp
    source/buffer/x_octet.cpp
    source/buffer/pool.cpp
    source/buffer/transport.cpp
    source/buffer/type.cpp
    source/transaction/id.cpp
    source/transaction/global.cpp
    source/transaction/transaction.cpp
    source/communication/ipc.cpp
    source/communication/ipc/message.cpp
    source/communication/ipc/send.cpp
    source/communication/socket.cpp
    source/communication/tcp.cpp
    source/communication/tcp/message.cpp
    source/communication/instance.cpp
    source/communication/device.cpp
    source/communication/stream.cpp
    source/communication/select/kqueue.cpp
    source/communication/select/epoll.cpp
    source/message/dispatch.cpp
    source/message/dispatch/handle.cpp
    source/message/type.cpp
    source/message/event.cpp
    source/message/counter.cpp
    source/event/listen.cpp
    source/event/send.cpp
    source/log/stream.cpp
    source/log/trace.cpp
    source/log/category.cpp
    source/log.cpp
    source/code/log.cpp
    source/code/xatmi.cpp
    source/code/tx.cpp
    source/code/xa.cpp
    source/code/system.cpp
    source/code/casual.cpp
    source/code/signal.cpp
    source/code/convert.cpp
    source/code/serialize.cpp
    source/code/queue.cpp
    source/exception/capture.cpp
    source/exception/guard.cpp
    source/exception/format.cpp
    source/environment.cpp
    source/environment/expand.cpp
    source/uuid.cpp
    source/file.cpp
    source/string.cpp
    source/signal.cpp
    source/signal/timer.cpp
    source/build.cpp
    source/chronology.cpp
    source/process.cpp
    source/instance.cpp
    source/domain.cpp
    source/execution/context.cpp
    source/network/byteorder.cpp
    source/transcode.cpp
    source/metric.cpp
    source/quantity.cpp
    source/argument.cpp
    source/terminal.cpp
    source/serialize/archive.cpp
    source/serialize/line.cpp
    source/serialize/log.cpp
    source/serialize/toml.cpp
    source/serialize/ini.cpp
    source/serialize/json.cpp
    source/serialize/xml.cpp
    source/serialize/yaml.cpp
    source/serialize/binary.cpp
    source/serialize/create.cpp
    source/serialize/archive/type.cpp
    source/serialize/service/protocol/describe.cpp
    source/serialize/service/protocol/implementation.cpp
    source/serialize/service/protocol.cpp
)

target_include_directories(casual-common PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CASUAL_THIRDPARTY_SOURCE_DIR}/rapidjson/include>
)

target_link_directories(casual-common PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)

target_link_libraries(casual-common PRIVATE
  yaml-cpp
  pugixml::pugixml
  cppcodec::cppcodec
  tomlplusplus::tomlplusplus
  $<$<PLATFORM_ID:Darwin>:iconv>
  $<$<PLATFORM_ID:Linux>:pthread>
  $<$<PLATFORM_ID:Linux>:uuid>
)

if(DEFINED ENV{CASUAL_COMMIT_HASH})
  set(CASUAL_COMMIT_HASH \"$ENV{CASUAL_COMMIT_HASH}\")
else()
  set(CASUAL_COMMIT_HASH "\"undefined\"")
endif()

if(DEFINED ENV{CASUAL_BUILD_VERSION})
  set(CASUAL_BUILD_VERSION \"$ENV{CASUAL_BUILD_VERSION}\")
else()
  set(CASUAL_BUILD_VERSION "\"undefined\"")
endif()

target_compile_definitions(casual-common
  PRIVATE
    CASUAL_COMMIT_HASH=${CASUAL_COMMIT_HASH}
    CASUAL_BUILD_VERSION=${CASUAL_BUILD_VERSION}
)


add_library(casual-unittest SHARED)

target_sources(casual-unittest
  PRIVATE
    source/unittest.cpp
    source/unittest/file.cpp
    source/unittest/log.cpp
    source/unittest/eventually/send.cpp
)

target_include_directories(casual-unittest PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CASUAL_THIRDPARTY_SOURCE_DIR}/rapidjson/include>
)

target_link_directories(casual-unittest PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)

target_link_libraries(casual-unittest PRIVATE
  casual-common
  yaml-cpp
  pugixml::pugixml
  gtest::gtest
)

add_executable( unittest_tcp_server)

target_sources( unittest_tcp_server
   PRIVATE
     unittest/source/communication/tcp/server/main.cpp
)

target_include_directories(unittest_tcp_server PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

target_link_directories(unittest_tcp_server PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)

target_link_libraries(unittest_tcp_server PRIVATE
  casual-common
)



add_executable(simple_process)

target_sources(simple_process
  PRIVATE
    unittest/source/simple_process.cpp
)

target_include_directories(simple_process PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

target_link_directories(simple_process PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)

target_link_libraries(simple_process PRIVATE
  casual-common
)


add_executable(test-casual-common-isolated)

target_sources(test-casual-common-isolated
  PRIVATE
    unittest/source/test_conformance.cpp
    unittest/source/test_algorithm.cpp
    unittest/source/container/test_index.cpp
    unittest/source/test_code.cpp
    unittest/source/test_range.cpp
    unittest/source/test_execute.cpp
    unittest/source/execution/test_context.cpp
    unittest/source/test_task.cpp
    unittest/source/log/test_stream.cpp
    unittest/source/test_memory.cpp
    unittest/source/test_pimpl.cpp
    unittest/source/test_flag.cpp
    unittest/source/test_signal.cpp
    unittest/source/test_stream.cpp
    unittest/source/test_buffer.cpp
    unittest/source/test_uuid.cpp
    unittest/source/test_file.cpp
    unittest/source/test_process.cpp
    unittest/source/test_instance.cpp
    unittest/source/test_string.cpp
    unittest/source/test_exception.cpp
    unittest/source/test_environment.cpp
    unittest/source/environment/test_variable.cpp
    unittest/source/environment/test_expand.cpp
    unittest/source/network/test_byteorder.cpp
    unittest/source/test_traits.cpp
    unittest/source/test_chronology.cpp
    unittest/source/test_transcode.cpp
    unittest/source/communication/test_tcp.cpp
    unittest/source/communication/test_ipc.cpp
    unittest/source/communication/test_stream.cpp
    unittest/source/communication/test_select.cpp
    unittest/source/message/test_dispatch.cpp
    unittest/source/message/test_coordinate.cpp
    unittest/source/service/test_header.cpp
    unittest/source/transaction/test_id.cpp
    unittest/source/event/test_dispatch.cpp
    unittest/source/event/test_listen.cpp
    unittest/source/test_domain.cpp
    unittest/source/strong/test_type.cpp
    unittest/source/test_predicate.cpp
    unittest/source/test_functional.cpp
    unittest/source/argument/test_cardinality.cpp
    unittest/source/argument/test_option.cpp
    unittest/source/argument/test_parse.cpp
    unittest/source/argument/test_complete.cpp
    unittest/source/test_metric.cpp
    unittest/source/test_quantity.cpp
    unittest/source/serialize/test_traits.cpp
    unittest/source/serialize/test_macro.cpp
    unittest/source/serialize/test_line.cpp
    unittest/source/serialize/test_archive.cpp
    unittest/source/serialize/test_consumed.cpp
    unittest/source/serialize/test_create.cpp
    unittest/source/serialize/test_toml.cpp
    unittest/source/serialize/test_ini.cpp
    unittest/source/serialize/test_json.cpp
    unittest/source/serialize/test_log.cpp
    unittest/source/serialize/test_xml.cpp
    unittest/source/serialize/test_yaml.cpp
    unittest/source/serialize/test_relaxed_write_read.cpp
    unittest/source/serialize/test_write_read.cpp
    unittest/source/serialize/native/test_binary.cpp
    unittest/source/serialize/named/test_value.cpp
)

target_include_directories(test-casual-common-isolated PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

target_link_directories(test-casual-common-isolated PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)

target_link_libraries(test-casual-common-isolated PRIVATE
  casual-common
  casual-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-casual-common-isolated "" AUTO)

install(
    TARGETS
      casual-common
      casual-unittest
    DESTINATION
      lib
)

install(
    FILES
      include/casual/platform.h
    DESTINATION
      include/casual
)

