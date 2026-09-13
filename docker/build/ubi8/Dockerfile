ARG IMAGE=casual/ubi8-builder:1.9-buildx

FROM ${IMAGE} as builder

ARG CASUAL_BUILD_VERSION
ARG CASUAL_COMMIT_HASH
ARG CASUAL_VERSION
ARG CASUAL_RELEASE=1

COPY casual /home/casual/repo/casual
COPY casual-make /home/casual/repo/casual-make
RUN chown -R casual:casual /home/casual/

USER casual
WORKDIR /home/casual/repo/casual

ENV CASUAL_MAKE_SOURCE_ROOT=/home/casual/repo/casual
ENV CASUAL_MAKE_BUILD_ROOT=/tmp/casual/build
ENV CASUAL_MAKE_HOME=$CASUAL_MAKE_SOURCE_ROOT/../casual-make/source
ENV PYTHONPATH=$CASUAL_MAKE_HOME:$CASUAL_MAKE_SOURCE_ROOT/make
ENV PATH=$PATH:$CASUAL_MAKE_HOME

ENV INSTALL_ETC="Y"
ENV CASUAL_HOME=/opt/casual
RUN echo ${CASUAL_MAKE_BUILD_ROOT}
ENV CASUAL_LOG_PATH=${CASUAL_MAKE_BUILD_ROOT}/casual.log
ENV LD_LIBRARY_PATH=/usr/local/lib

RUN echo $PATH

RUN mkdir -p ${CASUAL_MAKE_BUILD_ROOT}
RUN echo "set(CPACK_RPM_PACKAGE_VERSION ${CASUAL_VERSION})" > ${CASUAL_MAKE_BUILD_ROOT}/version.cmake
RUN echo "set(CPACK_RPM_PACKAGE_RELEASE ${CASUAL_RELEASE})" >> ${CASUAL_MAKE_BUILD_ROOT}/version.cmake
RUN cat ${CASUAL_MAKE_BUILD_ROOT}/version.cmake

SHELL ["/bin/bash", "-c", "source scl_source enable gcc-toolset-14 && bash -c \"$0\" \"$@\""]

RUN conan profile detect && \
    conan install conanfile.py --output-folder ${CASUAL_MAKE_BUILD_ROOT}/.. --build=missing
RUN casual-make compile 
FROM scratch
COPY --from=builder /tmp/casual/build/Release/casual-middleware*.rpm .
COPY --from=builder /tmp/casual/build/Release/casual.log .
COPY --from=builder /tmp/casual/build/Release/report.xml .