#!/bin/bash

#
# This is just a temporary helper until we can produce this from the makefile.
#

CONFIGURATION_PATH=${CASUAL_BUILD_HOME}/middleware/configuration

${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --domain-file ${CONFIGURATION_PATH}/example/domain/domain.yaml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --domain-file ${CONFIGURATION_PATH}/example/domain/domain.json
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --domain-file ${CONFIGURATION_PATH}/example/domain/domain.xml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --domain-file ${CONFIGURATION_PATH}/example/domain/domain.ini

${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-domain-file ${CONFIGURATION_PATH}/example/domain/default/domain.yaml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-domain-file ${CONFIGURATION_PATH}/example/domain/default/domain.json
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-domain-file ${CONFIGURATION_PATH}/example/domain/default/domain.xml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-domain-file ${CONFIGURATION_PATH}/example/domain/default/domain.ini



${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --build-server-file ${CONFIGURATION_PATH}/example/build/server.yaml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --build-server-file ${CONFIGURATION_PATH}/example/build/server.json
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --build-server-file ${CONFIGURATION_PATH}/example/build/server.xml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --build-server-file ${CONFIGURATION_PATH}/example/build/server.ini

${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-build-server-file ${CONFIGURATION_PATH}/example/build/default/server.yaml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-build-server-file ${CONFIGURATION_PATH}/example/build/default/server.json
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-build-server-file ${CONFIGURATION_PATH}/example/build/default/server.xml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --default-build-server-file ${CONFIGURATION_PATH}/example/build/default/server.ini


${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --resource-property-file ${CONFIGURATION_PATH}/example/resource/property.yaml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --resource-property-file ${CONFIGURATION_PATH}/example/resource/property.json
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --resource-property-file ${CONFIGURATION_PATH}/example/resource/property.xml
${CONFIGURATION_PATH}/bin/casual-configuration-example-maker --resource-property-file ${CONFIGURATION_PATH}/example/resource/property.ini

