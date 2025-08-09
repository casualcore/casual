set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../server/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)


add_library(casual-xatmi-explicit SHARED)

target_sources(casual-xatmi-explicit
  PRIVATE
    source/xatmi/explicit/buffer.cpp
    source/xatmi/explicit/service.cpp
    source/xatmi/internal/server/service.cpp
    source/xatmi/internal/context.cpp
    source/xatmi/internal/code.cpp
    source/xatmi/internal/log.cpp
)

target_link_libraries(casual-xatmi-explicit PRIVATE
  casual-service
  casual-transaction
  casual-server
  casual-common
)

add_library(casual-xatmi SHARED)

target_sources(casual-xatmi
  PRIVATE
    source/xatmi.cpp
    source/xatmi/extended.cpp
    source/xatmi/server.cpp
    source/xatmi/executable.cpp
    source/xatmi/conversation.cpp
    source/xatmi/cobol.cpp
    source/xa.cpp
    source/tx.cpp
)

target_link_libraries(casual-xatmi PRIVATE
  casual-xatmi-explicit
  casual-transaction
  casual-service
  casual-server
  casual-common
)


add_executable(test-casual-xatmi)

target_sources(test-casual-xatmi
  PRIVATE
    unittest/source/test_buffer.cpp
    unittest/source/test_tx.cpp
    unittest/source/test_xatmi.cpp
    unittest/source/test_header.cpp
    unittest/source/server/test_service.cpp
)

target_link_libraries(test-casual-xatmi PRIVATE
  casual-xatmi
  casual-xatmi-explicit
  casual-common
  casual-unittest
  casual-server
  gtest::gtest
  gtest_main
  $<$<PLATFORM_ID:Linux>:uuid>
)


enable_testing()
include(GoogleTest)
gtest_add_tests(test-casual-xatmi "" AUTO)

install(
    TARGETS
      casual-xatmi-explicit
      casual-xatmi
    DESTINATION
      lib
)

install(
    FILES
      include/casual/xatmi/server.h
      include/casual/xatmi/executable.h
      include/casual/xatmi/extended.h
      include/casual/xatmi/explicit.h
      include/casual/xatmi/defines.h
      include/casual/xatmi/flag.h
      include/casual/xatmi/code.h
      include/casual/xatmi/xa.h
    DESTINATION
      include/casual/xatmi
)
install(
    FILES
      include/casual/xatmi.h
      include/casual/tx.h
      include/casual/xa.h
    DESTINATION
      include/casual
)

install(
    FILES
      include/xatmi.h
      include/tx.h
      include/xa.h
    DESTINATION
      include
)

install(
    FILES
      include/casual/tx/code.h
    DESTINATION
      include/casual/tx
)
install(
    FILES
      include/casual/xa/code.h
      include/casual/xa/flag.h
    DESTINATION
      include/casual/xa
)

