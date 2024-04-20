#!/bin/sh

PERCIVAL_RUN_DIR=/home/prcvlusr/percival/run
CONFIG_DIR=/home/prcvlusr/percival/config

gnome-terminal --geometry 80x24+100+50 -t "FR1" --working-directory $PERCIVAL_RUN_DIR \
  -- /opt/percival-detector/odin/bin/frameReceiver --ctrl=tcp://0.0.0.0:5000 --json_file=$CONFIG_DIR/fr1.json  --debug-level=2 --logconfig $CONFIG_DIR/fr1.log4cxx

gnome-terminal --geometry 80x24+900+50 -t "FR2" --working-directory $PERCIVAL_RUN_DIR \
  -- /opt/percival-detector/odin/bin/frameReceiver --ctrl=tcp://0.0.0.0:5010 --json_file=$CONFIG_DIR/fr2.json  --debug-level=2 --logconfig $CONFIG_DIR/fr2.log4cxx

gnome-terminal --geometry 80x24+100+650 -t "FP1-plugin2" --working-directory $PERCIVAL_RUN_DIR \
  -- /opt/percival-detector/odin/bin/frameProcessor --ctrl=tcp://0.0.0.0:5004 --json_file=$CONFIG_DIR/fp1.json --debug-level=2 --logconfig $CONFIG_DIR/fp1.log4cxx

gnome-terminal --geometry 80x24+900+650 -t "FP2-plugin2" --working-directory $PERCIVAL_RUN_DIR \
  -- /opt/percival-detector/odin/bin/frameProcessor --ctrl=tcp://0.0.0.0:5014 --json_file=$CONFIG_DIR/fp2.json --debug-level=2 --logconfig $CONFIG_DIR/fp2.log4cxx

