FROM centos:latest
MAINTAINER flurig <flurig@localhost>

RUN yum  -y install wget cmake make python rsync libuuid-devel sqlite-devel gcc-c++ zlib-devel unzip rpm-build
#RUN wget http://dl.fedoraproject.org/pub/epel/7/x86_64/Packages/e/epel-release-7-13.noarch.rpm
#RUN rpm -Uvh epel-release*rpm
#RUN yum -y install pugixml yaml-cpp03

ARG CASUAL_RPM
COPY ${CASUAL_RPM} /tmp/casual-middleware.rpm
RUN rpm -i -p /tmp/casual-middleware.rpm

RUN useradd -ms /bin/bash casual

ENV CASUAL_HOME /opt/casual
ENV PATH $CASUAL_HOME/bin:$PATH
ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:$CASUAL_HOME/lib
ENV CASUAL_DOMAIN_HOME /test/casual
ENV CASUAL_LOG_PATH $CASUAL_DOMAIN_HOME/casual.log
ENV CASUAL_LOG '(error|warning|information)'

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