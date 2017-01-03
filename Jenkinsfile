#!groovy

//
// Script for the backend builder
//
def backend_builder = '''#! /bin/sh
umask 000
cd /git/casual
rm -rf middleware/.casual/unittest/.singleton/.domain-singleton
casual-make make && casual-make clean && casual-make compile && casual-make link 
casual-make install
cd /usr/local/casual
tar cvf /git/casual/casual-middleware.tar .
'''

def backend_builder_unittest = '''#! /bin/sh
umask 000
cd /git/casual
rm -rf middleware/.casual/unittest/.singleton/.domain-singleton
casual-make -d -a --use-valgrind make && casual-make clean && casual-make -d -a --use-valgrind compile && casual-make -d -a --use-valgrind link 
ISOLATED_UNITTEST_DIRECTIVES="--gtest_output='xml:report.xml'" casual-make test
'''
//
// Script for the frontend builder
//
def frontend_builder = '''#! /bin/sh
umask 000
cd /git/casual/webapp
bower update --allow-root
touch bower_components/app-route/app-location.html
polymer build
cd ..
zip -r casual-webapp.zip webapp
chmod a+w -R .
'''

//
// Dockerfile for creating testcontainer
//
def dockerfile = '''FROM ubuntu:trusty
MAINTAINER flurig <flurig@localhost>

RUN apt-get update && apt-get -y upgrade && apt-get -y install uuid-dev libyaml-cpp0.3-dev libjson-c-dev libpugixml-dev libsqlite3-dev wget cmake g++-4.8 python rsync unzip && ln -s /usr/bin/g++-4.8 /usr/bin/g++

RUN wget --no-check-certificate https://github.com/jbeder/yaml-cpp/archive/release-0.3.0.tar.gz && tar xf release-0.3.0.tar.gz && cd yaml-cpp-release-0.3.0 && mkdir build && cd build && cmake -DBUILD_SHARED_LIBS=ON .. && make && make install && rm -rf /release-0.3.0.tar.gz /yaml-cpp-release-0.3.0

RUN mkdir -p /opt/casual

COPY casual-middleware.tar /opt/casual/casual-middleware.tar
COPY casual-webserver.tar /opt/casual/casual-webserver.tar
COPY casual-webapp.zip /opt/casual/casual-webapp.zip
COPY start.sh /opt/casual/start.sh

RUN cd /opt/casual && tar xf casual-middleware.tar && tar xf casual-webserver.tar && unzip casual-webapp.zip && chmod 755 /opt/casual/start.sh

RUN useradd -ms /bin/bash casual

ENV CASUAL_HOME /opt/casual
ENV PATH $CASUAL_HOME/bin:$PATH
ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:$CASUAL_HOME/lib
ENV CASUAL_DOMAIN_HOME /test/casual
ENV CASUAL_LOG ".*"

RUN mkdir -p $CASUAL_DOMAIN_HOME
RUN cp -r /opt/casual/example/domain/single/minimal/* $CASUAL_DOMAIN_HOME/.
RUN cp $CASUAL_HOME/configuration/example/resources.yaml $CASUAL_HOME/configuration/.
RUN chown -R casual $CASUAL_DOMAIN_HOME
RUN chown -R casual $CASUAL_HOME/nginx

EXPOSE 8080 7771
USER casual
RUN ln -s /dev/stdout $CASUAL_DOMAIN_HOME/casual.log
WORKDIR $CASUAL_DOMAIN_HOME

ENTRYPOINT ["/opt/casual/start.sh"]
'''

//
// Startscript for dockercontainer
//
def dockerstart = '''#! /bin/bash

casual-admin domain --boot

sleep 5

while $( pgrep ^casual-broker$ > 0); do sleep 5; done
'''

def build( name, image, content)
{
   def current_dir = pwd()
   writeFile file: 'builder.sh', text: content

   sh """
   chmod +x builder.sh
   if docker ps -a | grep $name; then docker rm $name;fi
   docker run --name $name -v $current_dir:/git/casual $image
   """
}

node {

   stage('Checkout')
   {
      checkout scm
   }

   stage('Build backend - Unittest/CodeCoverage') {

       build( 'ubuntucompile', 'casual/ubuntu', backend_builder_unittest)

       step([$class: 'XUnitBuilder',
          thresholds: [[$class: 'FailedThreshold', failureThreshold: '1']],
          tools: [[$class: 'GoogleTestType', pattern: '**/report.xml']]])

   }

   stage('Build backend - Release') {

       build( 'ubuntucompile', 'casual/ubuntu', backend_builder)

       archive includes: '**/casual.log'
       archive includes: 'casual-middleware.tar'
   }

   stage('Build Nginx Ubuntu') {
       
       def current_dir = pwd()

       sh """
       export CASUAL_HOME=$current_dir/usr/local/casual
       export CASUAL_DOMAIN_HOME=$current_dir/test/casual
       export CASUAL_BUILD_HOME=$current_dir
       export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$current_dir/middleware/common/bin
       python $current_dir/thirdparty/setup/install_nginx.py
       cd $current_dir/usr/local/casual
       tar cvf $current_dir/casual-webserver.tar nginx
       """

       archive includes: '**/casual-webserver.tar'
   }


   stage('Build frontend') {

       build( 'casualfrontend', 'casual/frontend', frontend_builder)

       archive includes: '**/casual-webapp.zip'
   }

   stage('Create container') {

       // 
       // Setup files
       //
       writeFile file: 'Dockerfile', text: dockerfile
       writeFile file: 'start.sh', text: dockerstart

       sh """
       docker build -t casual-test-ubuntu -f Dockerfile .
       """
   }

   stage('Publishing to dockerhub') {

       if ( "${env.BRANCH_NAME}" == "develop" )
       {
          sh """
          docker tag -f casual-test-ubuntu casual/middleware:latest
          docker push casual/middleware
          """
       }
   }

   stage('Deploy') {

       sh """
       cd /var/lib/jenkins/git/casual/docker
       ./restart.sh
       """
   }


}
