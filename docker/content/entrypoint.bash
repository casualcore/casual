#!/usr/bin/env bash

casual-admin domain --boot "${CASUAL_DOMAIN_HOME}/configuration/domain.yaml"

exec "$@"
