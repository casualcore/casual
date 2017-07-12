FROM debian:latest
MAINTAINER Mikael Göransson <bitbucket@mgor.se>

ENV YAML_CPP_VERSION 0.3.0
ENV NGINX_VERSION 1.13.3

RUN apt-get update && \
    apt-get -y install uuid-dev \
        libyaml-cpp0.3-dev \
        libjson-c-dev \
        libpugixml-dev \
        libsqlite3-dev \
        wget \
        cmake \
        git \
        g++-6 \
        python \
        zlib1g-dev \
        rsync && \
    ln -s /usr/bin/g++-6 /usr/bin/g++ && \
    rm -rf /var/lib/apt/lists/*

RUN cd /tmp && \
    wget --no-check-certificate https://github.com/jbeder/yaml-cpp/archive/release-${YAML_CPP_VERSION}.tar.gz && \
    tar xf release-${YAML_CPP_VERSION}.tar.gz && \
    cd yaml-cpp-release-${YAML_CPP_VERSION} && \
    mkdir build && \
    cd build && \
    cmake -DBUILD_SHARED_LIBS=ON .. && \
    make && \
    make install && \
    rm -rf /tmp/release-${YAML_CPP_VERSION}.tar.gz /tmp/yaml-cpp-release-${YAML_CPP_VERSION}

ENV CASUAL_REPO_ROOT /git
ENV CASUAL_BUILD_HOME $CASUAL_REPO_ROOT/casual
ENV PYTHONPATH $CASUAL_REPO_ROOT
ENV YAML_INCLUDE_PATH /usr/local/include
ENV YAML_LIBRARY_PATH /usr/local/lib
ENV JSON_INCLUDE_PATH /usr/local/include
ENV JSON_LIBRARY_PATH /usr/lib/x86_64-linux-gnu
ENV CASUAL_OPTIONAL_INCLUDE_PATHS "$CASUAL_BUILD_HOME/thirdparty/unittest/gtest/include $YAML_INCLUDE_PATH $JSON_INCLUDE_PATH"
ENV CASUAL_OPTIONAL_LIBRARY_PATHS "$CASUAL_BUILD_HOME/thirdparty/unittest/gtest/bin $YAML_LIBRARY_PATH $JSON_LIBRARY_PATH"
ENV CASUAL_HOME /opt/casual
ENV PATH $CASUAL_BUILD_HOME/tools/bin:$CASUAL_BUILD_HOME/bin:$CASUAL_HOME/bin:$PATH
ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:$CASUAL_HOME/lib:$CASUAL_BUILD_HOME/configuration/bin:$CASUAL_BUILD_HOME/common/bin:$CASUAL_BUILD_HOME/serviceframework/bin:$CASUAL_BUILD_HOME/xatmi/bin
ENV CASUAL_DOMAIN_HOME /srv/domain/casual
ENV CASUAL_LOG %

RUN mkdir -p ${CASUAL_REPO_ROOT} && \
    cd ${CASUAL_REPO_ROOT} && \
    git clone https://bitbucket.org/casualcore/casual.git && \
    cd ${CASUAL_BUILD_HOME} && \
    casual-make compile && \
    casual-make link && \
    casual-make install

RUN cd /tmp && \
    wget --no-check-certificate http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz && \
    tar xf nginx-${NGINX_VERSION}.tar.gz && \
    cd nginx-${NGINX_VERSION} && \
    ./configure --prefix=${CASUAL_HOME}/nginx --with-cc-opt=-Wno-deprecated --add-module=${CASUAL_BUILD_HOME}/middleware/plugin --without-http_rewrite_module && \
    make && \
    make install && \
    rm -rf /tmp/nginx-${NGINX_VERSION}*

COPY content/entrypoint.bash /usr/local/bin

COPY content/casual-admin.bash /etc/bash_completion.d/casual-admin

RUN mkdir -p ${CASUAL_DOMAIN_HOME}/ && \
    ln -sf /dev/stdout ${CASUAL_DOMAIN_HOME}/casual.log && \
    ln -sf /dev/stderr ${CASUAL_HOME}/nginx/logs/error.log && \
    ln -sf /dev/stderr ${CASUAL_HOME}/nginx/logs/debug.log && \
    echo 'export PATH=$PATH' >> /root/.bashrc && \
    echo '. /etc/bash_completion.d/casual-admin' >> /root/.bashrc

ENTRYPOINT ["entrypoint.bash"]

CMD bash
