#!/bin/bash

clear

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

echo $SCRIPT_DIR

/opt/percival-detector/odin/bin/frameReceiver --ctrl=tcp://0.0.0.0:5000 --json_file=$SCRIPT_DIR/fr1.json --logconfig $SCRIPT_DIR/fr.log4cxx

