set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../xatmi/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../cobol/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../../xatmi/bin>
)


add_library(casual-cobol-xatmi SHARED)

target_sources(casual-cobol-xatmi
  PRIVATE
    source/tx.cpp
    source/xatmi.cpp
)

target_link_libraries(casual-cobol-xatmi PRIVATE
  casual-xatmi
)

add_library(casual-cobol-xatmi-archive STATIC)

target_sources(casual-cobol-xatmi-archive
  PRIVATE
    source/tx.cpp
    source/xatmi.cpp
)

target_include_directories(casual-cobol-xatmi-archive PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../xatmi/include>
)

install(
    FILES
      copy/TPSTATUS.cpy
      copy/TPSVCDEF.cpy
      copy/TPSVCRET.cpy
      copy/TPTYPE.cpy
      copy/TXINFDEF.cpy
      copy/TXSTATUS.cpy
      copy/TPRETURN.cpy
    DESTINATION
      binding/cobol/copy/
)

install(
    TARGETS
      casual-cobol-xatmi
      casual-cobol-xatmi-archive
    DESTINATION
      lib
)

