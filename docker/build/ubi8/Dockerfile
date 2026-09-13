ARG IMAGE=casual/ubi8-builder:1.9-buildx

FROM ${IMAGE} AS builder

# Input arguments
ARG CASUAL_BUILD_VERSION
ARG CASUAL_COMMIT_HASH
ARG CASUAL_VERSION
ARG CASUAL_RELEASE=1

# Copy repos from host
COPY casual /home/casual/repo/casual
COPY casual-thirdparty /home/casual/repo/casual-thirdparty

RUN chown -R casual:casual /home/casual/

USER casual
WORKDIR /home/casual/repo/casual

# Environment variables used in build and test
ENV CASUAL_MAKE_SOURCE_ROOT=/home/casual/repo/casual
ENV CASUAL_MAKE_BUILD_ROOT=/tmp/casual/build
ENV CASUAL_THIRDPARTY=/home/casual/repo/casual-thirdparty
ENV PYTHONPATH=$CASUAL_MAKE_SOURCE_ROOT/make/src
ENV PATH=$PATH:/home/casual/.local/bin
ENV CASUAL_HOME=/opt/casual
ENV CASUAL_LOG_PATH=${CASUAL_MAKE_BUILD_ROOT}/casual.log
ENV LD_LIBRARY_PATH=/usr/local/lib:$CASUAL_MAKE_BUILD_ROOT/middleware/common/bin:$CASUAL_MAKE_BUILD_ROOT/middleware/configuration/bin:$CASUAL_MAKE_BUILD_ROOT/middleware/buffer/bin:$CASUAL_MAKE_BUILD_ROOT/middleware/server/bin

RUN echo ${CASUAL_MAKE_BUILD_ROOT}

# Install casual-make
RUN pip3 install "git+https://github.com/casualcore/casual-make.git@feature/1.9/build-directory"

# Make sure that the toolset is enabled
SHELL ["/bin/bash", "-c", "source scl_source enable gcc-toolset-14 && bash -c \"$0\" \"$@\""]

# Install dependencies via conan
RUN conan profile detect && \
    conan install conanfile.py --output-folder ${CASUAL_MAKE_BUILD_ROOT}/.. --build=missing

RUN echo "compiling" && casual-make --stat --quiet --no-colors compile && \
    echo "linking" && casual-make --stat --no-colors link || (echo "Linking failed, showing created files:" && find ${CASUAL_MAKE_BUILD_ROOT} -ls  && exit 1) && \
    echo "testing" && casual-make --no-colors test --gtest_color=no --gtest_output='xml:report.xml' || (echo "Test failed, showing last 1000 rows in log:" && tail ${CASUAL_LOG_PATH}  && exit 1) && \
    echo "installing" && casual-make --no-colors install && \
    echo "building nginx" && casual-make nginx && \
    echo "packaging" && bash pipeline/script/package $CASUAL_VERSION $CASUAL_RELEASE 'ubi' && \
    find . -name "report.xml" -print0 | tar --null -cvf /tmp/casual/build/report.tar --files-from=-

# Copy relevant artifacts from build
FROM scratch
COPY --from=builder /tmp/casual/build/casual-middleware*.rpm .
COPY --from=builder /tmp/casual/build/casual.log .
COPY --from=builder /tmp/casual/build/report.tar .