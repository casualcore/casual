include(CMakePrintHelpers)

macro(LinkServer NAME)
    set(options OPTIONAL)
    set(oneValueArgs SERVER_DEFINITION CONFIGURATION_FILE)
    set(multiValueArgs OBJECTS INCLUDE_PATHS LIBRARIES LIBRARY_PATHS SERVICES)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    file(WRITE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dummy.cpp
"int main(int argc, char** argv) 
{
    return 0;
}"
    )
    configure_file(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dummy.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}-dummy.cpp)

    string(REPLACE ";" " -I" include_paths "${arg_INCLUDE_PATHS}")
    string(REPLACE ";" " -L" library_paths "${arg_LIBRARY_PATHS}")
    string(REPLACE ";" " -Wl,-rpath," library_rpaths "${arg_LIBRARY_PATHS}")
    string(REPLACE ";" " -l" libraries "${arg_LIBRARIES}")

    add_custom_command(
        OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}.real
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}-dummy.cpp
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../tools/bin/casual-build-server
            --output ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}.real
            --definition ${arg_SERVER_DEFINITION}
            --build-directives "$<JOIN:${arg_OBJECTS}, >" -O3 -I${include_paths} -Wl,-rpath,${library_rpaths} -L "${library_paths}" -l${libraries}
            --system-configuration "${arg_CONFIGURATION_FILE}"

        DEPENDS
            ${arg_LIBRARIES}
            ${arg_OBJECTS}
            casual-xatmi
            casual-build-server
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}-dummy.cpp
    )

    add_executable(${NAME})

    target_sources(${NAME} PRIVATE
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}-dummy.cpp
    )

    target_include_directories(${NAME} PRIVATE
        ${arg_INCLUDE_PATHS}
    )

    target_link_directories(${NAME} PRIVATE
        ${arg_LIBRARY_PATHS}
    )

    target_link_libraries(${NAME} PRIVATE
        ${arg_LIBRARIES}
    )

    add_custom_command(
        TARGET ${NAME}
        POST_BUILD
        COMMAND rm ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}
        COMMAND cp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}.real
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}
    )

endmacro()


