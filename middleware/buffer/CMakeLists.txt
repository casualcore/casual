set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../cli/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
)

add_library(casual-buffer SHARED)

target_sources(casual-buffer
  PRIVATE
    source/field.cpp
    source/octet.cpp
    source/order.cpp
    source/string.cpp
    source/internal/field/string.cpp
)

target_link_libraries(casual-buffer PRIVATE
  casual-common
)


add_executable(casual-buffer-field-make-header)

target_sources(casual-buffer-field-make-header
  PRIVATE
    tools/source/field.cpp
)


target_link_libraries(casual-buffer-field-make-header PRIVATE
  casual-common
  casual-buffer
)


add_library(casual-buffer-cli SHARED)

target_sources(casual-buffer-cli
  PRIVATE
    source/admin/cli.cpp
)

target_link_libraries(casual-buffer-cli PRIVATE
  casual-buffer
  casual-cli
  casual-common
)


add_executable(test-buffer)

target_sources(test-buffer
  PRIVATE
    unittest/source/test_field_buffer.cpp
    unittest/source/test_octet_buffer.cpp
    unittest/source/test_order_buffer.cpp
    unittest/source/test_string_buffer.cpp
    unittest/source/test_field_stream.cpp
    unittest/source/internal/field/test_string.cpp
    unittest/source/internal/field/test_table.cpp
)


target_link_libraries(test-buffer PRIVATE
  casual-buffer
  casual-common
  casual-xatmi
  casual-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-buffer "" AUTO)

add_executable(casual-buffer-protocol-sample)

target_sources(casual-buffer-protocol-sample
  PRIVATE
    sample/protocol/source/field.cpp
)

target_link_libraries(casual-buffer-protocol-sample PRIVATE
  casual-buffer
  casual-xatmi
  casual-common
)


add_library(casual-buffer-sample SHARED)

target_sources(casual-buffer-sample
  PRIVATE
    sample/client/source/field.cpp
    sample/client/source/octet.cpp
    sample/client/source/order.cpp
    sample/client/source/string.cpp
)

target_link_libraries(casual-buffer-sample PRIVATE
  casual-buffer
  casual-xatmi
)


add_executable(casual-buffer-field-from-human)

target_sources(casual-buffer-field-from-human
  PRIVATE
    tools/source/field/from_human.cpp
)

target_link_libraries(casual-buffer-field-from-human PRIVATE
  casual-buffer-cli
  casual-buffer
  casual-common
)


add_executable(casual-buffer-field-to-human)

target_sources(casual-buffer-field-to-human
  PRIVATE
    tools/source/field/to_human.cpp
)

target_link_libraries(casual-buffer-field-to-human PRIVATE
  casual-buffer-cli
  casual-buffer
  casual-common
)


add_executable(casual-buffer-duplicate)

target_sources(casual-buffer-duplicate
  PRIVATE
    tools/source/duplicate.cpp
)

target_link_libraries(casual-buffer-duplicate PRIVATE
  casual-buffer-cli
  casual-common
)


add_executable(casual-buffer-generic-compose)

target_sources(casual-buffer-generic-compose
  PRIVATE
    tools/source/generic/compose.cpp
)


target_link_libraries(casual-buffer-generic-compose PRIVATE
  casual-buffer-cli
  casual-common
)


add_executable(casual-buffer-generic-extract)

target_sources(casual-buffer-generic-extract
  PRIVATE
    tools/source/generic/extract.cpp
)


target_link_libraries(casual-buffer-generic-extract PRIVATE
  casual-buffer-cli
  casual-common
)


add_executable(casual-buffer-field-serialize)

target_sources(casual-buffer-field-serialize
  PRIVATE
    tools/source/field/serialize.cpp
)

target_link_libraries(casual-buffer-field-serialize PRIVATE
  casual-common
  casual-buffer
)

install(
    TARGETS
      casual-buffer
      casual-buffer-cli
    DESTINATION
      lib
)

install(
    TARGETS
      casual-buffer-field-make-header
      casual-buffer-field-from-human
      casual-buffer-field-to-human
      casual-buffer-duplicate
      casual-buffer-generic-compose
      casual-buffer-generic-extract
      casual-buffer-field-serialize
    DESTINATION
      bin
)

install(
    FILES
        include/casual/buffer/field.h
        include/casual/buffer/octet.h
        include/casual/buffer/order.h
        include/casual/buffer/string.h
    DESTINATION
        include/casual/buffer
)

install(
    FILES
        include/casual/buffer/internal/field/string.h
    DESTINATION
        include/casual/buffer/internal/field

)

# Documentation
add_custom_target(documentation-buffer
    #COMMAND echo "Generating documentation for buffer"
    COMMAND
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/bin/casual-buffer-protocol-sample
    VERBATIM
    DEPENDS
      casual-buffer-protocol-sample

)





