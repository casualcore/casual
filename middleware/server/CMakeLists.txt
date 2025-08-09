set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
)

add_library(casual-server SHARED)

target_sources(casual-server
  PRIVATE
    source/context.cpp
    source/service.cpp
    source/start.cpp
    source/internal/start.cpp
    source/executable/start.cpp
    source/service/protocol.cpp
)

target_include_directories(casual-server PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

target_link_directories(casual-server PRIVATE

)

target_link_libraries(casual-server PRIVATE
  casual-common
  casual-service
  casual-transaction
)


add_executable(casual-server-test)

target_sources(casual-server-test
  PRIVATE
    unittest/source/test_arguments.cpp
)

target_include_directories(casual-server-test PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

target_link_directories(casual-server-test PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
)

target_link_libraries(casual-server-test PRIVATE
  casual-server
  casual-unittest
  casual-common
  casual-service
  gtest::gtest
)


enable_testing()
include(GoogleTest)
gtest_add_tests(casual-server-test "" AUTO)

install(
    TARGETS
      casual-server
    DESTINATION
      lib
)

