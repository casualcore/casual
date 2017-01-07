#!/bin/bash

#
# This is just a temporary helper until we can produce this from the makefile.
#

../bin/casual-configuration-example-maker --output domain.yaml
../bin/casual-configuration-example-maker --output domain.json
../bin/casual-configuration-example-maker --output domain.xml
../bin/casual-configuration-example-maker --output domain.ini


../bin/casual-configuration-example-maker --default default/domain.yaml
../bin/casual-configuration-example-maker --default default/domain.json
../bin/casual-configuration-example-maker --default default/domain.xml
../bin/casual-configuration-example-maker --default default/domain.ini



