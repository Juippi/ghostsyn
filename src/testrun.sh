#!/bin/bash

make DESTDIR=install/ install
LV2_PATH=file:///home/juippi/git/ghostsyn/src/install/usr/local/lib/lv2/ jalv http://example.com/plugins/lv2_ghostsyn
