#!/bin/bash

clear

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

/opt/percival-detector/odin/bin/frameProcessor --ctrl=tcp://0.0.0.0:5014 --json_file=$SCRIPT_DIR/fp2.json --debug-level=0 --logconfig $SCRIPT_DIR/fp.log4cxx
