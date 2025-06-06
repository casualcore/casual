FROM redhat/ubi8
LABEL org.casualcore.image.authors="casual@laz.se"

RUN yum --exclude=iputils -y update && yum -y install wget cmake make libuuid-devel zlib-devel libcurl-devel
RUN yum -y install gcc-toolset-14-gcc* && scl enable gcc-toolset-14 bash
ADD sqlite-autoconf-3470100.tar.gz /tmp/.
RUN cd /tmp/sqlite-autoconf-3470100 && source scl_source enable gcc-toolset-14 && ./configure && make && make install

ARG CASUAL_RPM
COPY ${CASUAL_RPM} /tmp/casual-middleware.rpm
RUN rpm -i -p /tmp/casual-middleware.rpm
RUN useradd -ms /bin/bash casual
ENV CASUAL_HOME=/opt/casual
ENV PATH=$CASUAL_HOME/bin:$PATH
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CASUAL_HOME/lib:/usr/local/lib
ENV CASUAL_DOMAIN_HOME=/test/casual
ENV CASUAL_LOG_PATH=$CASUAL_DOMAIN_HOME/casual.log
ENV CASUAL_LOG='(error|warning|information)'
RUN mkdir -p $CASUAL_DOMAIN_HOME/logs
RUN cp -r /opt/casual/documentation/example/domain/single/* $CASUAL_DOMAIN_HOME/.
RUN chown -R casual $CASUAL_DOMAIN_HOME
RUN chown -R casual $CASUAL_HOME/nginx
RUN ln -s /dev/stdout /test/casual/casual.log
RUN chgrp -Rf root $CASUAL_DOMAIN_HOME $CASUAL_HOME/nginx && chmod -Rf g+w $CASUAL_DOMAIN_HOME $CASUAL_HOME/nginx
EXPOSE 8080 7771
USER casual
WORKDIR $CASUAL_DOMAIN_HOME
ENTRYPOINT ["casual-domain-manager", "-c", "configuration/domain.yaml"]
