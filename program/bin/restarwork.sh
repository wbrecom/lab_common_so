#!/bin/bash
# remove a so file
cp ../conf/work_config.ini.stop ../conf/work_config.ini
python2.6 binserverclient.py 127.0.0.1 21346 'update_work'

# add a so file
cp ../conf/work_config.ini.start ../conf/work_config.ini
python2.6 binserverclient.py 127.0.0.1 21346 'update_work'
