#!/bin/bash

sudo make uninstall
make clean
cmake ..
sudo make
sudo make install
sudo ldconfig
