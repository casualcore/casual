ARG IMAGE=casual/ubi8-builder:1.9-buildx

FROM ${IMAGE} as builder

ARG CASUAL_BUILD_VERSION
ARG CASUAL_COMMIT_HASH
ARG CASUAL_VERSION
ARG CASUAL_RELEASE=1

COPY casual /home/casual/repo/casual
RUN chown -R casual:casual /home/casual/

USER casual
WORKDIR /home/casual/repo/casual

ENV SOURCE_ROOT=/home/casual/repo/casual
ENV CMAKE_BUILD_HOME=/tmp/casual/build
ENV INSTALL_ETC="Y"
ENV CASUAL_HOME=/opt/casual
RUN echo ${CMAKE_BUILD_HOME}
ENV CMAKE_BINARY_DIR=${CMAKE_BUILD_HOME}/Release
ENV CMAKE_SOURCE_DIR=${SOURCE_ROOT}
ENV CASUAL_LOG_PATH=${CMAKE_BINARY_DIR}/casual.log
ENV LD_LIBRARY_PATH=/usr/local/lib

RUN mkdir -p ${CMAKE_BINARY_DIR}
RUN echo "set(CPACK_RPM_PACKAGE_VERSION ${CASUAL_VERSION})" > ${CMAKE_BINARY_DIR}/version.cmake
RUN echo "set(CPACK_RPM_PACKAGE_RELEASE ${CASUAL_RELEASE})" >> ${CMAKE_BINARY_DIR}/version.cmake
RUN cat ${CMAKE_BINARY_DIR}/version.cmake

SHELL ["/bin/bash", "-c", "source scl_source enable gcc-toolset-14 && bash -c \"$0\" \"$@\""]

RUN conan profile detect && \
    conan install conanfile.txt --output-folder ${CMAKE_BUILD_HOME}/.. --build=missing
RUN cmake --preset conan-release && \
    cmake --build . --preset conan-release && \
    ctest --output-junit report.xml --preset conan-release --output-on-failure --parallel 1 && \
    cmake --build . --target package --preset conan-release

FROM scratch
COPY --from=builder /tmp/casual/build/Release/casual-middleware*.rpm .
COPY --from=builder /tmp/casual/build/Release/casual.log .
COPY --from=builder /tmp/casual/build/Release/report.xml .
