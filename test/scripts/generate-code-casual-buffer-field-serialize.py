#!/usr/bin/env python3
#-*- coding: utf-8-unix -*-
import subprocess

repository_root_status_output = subprocess.getstatusoutput('git rev-parse --show-toplevel')

if repository_root_status_output[0] != 0:
    sys.exit(repository_root_status_output[0])

output_filename = '/tmp/casually-generated.cpp'
stanza = "CASUAL_FIELD_TABLE={0}/middleware/buffer/sample/field.json LD_LIBRARY_PATH={0}/middleware/common/bin:{0}/middleware/buffer/bin {0}/middleware/buffer/bin/casual-buffer-field-serialize --files {0}/middleware/buffer/sample/serialize/mapping.yaml > {1}".format(repository_root_status_output[1], output_filename)

stanza_status_output = subprocess.getstatusoutput(stanza)

if stanza_status_output[0] != 0:
    sys.exit(stanza_status_output[0])

print("output written to: {0}".format(output_filename))    
