set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../cli/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)

add_library(casual-cli SHARED)

target_sources(casual-cli
  PRIVATE
    source/pipe.cpp
    source/message.cpp
)

target_link_libraries(casual-cli PRIVATE
  casual-common
)

install(
    TARGETS
      casual-cli
    DESTINATION
      lib
)

