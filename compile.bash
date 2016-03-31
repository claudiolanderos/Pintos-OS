#!/bin/bash
# 
# File:   compile.bash
# Author: zangetsu
#
# Created on Mar 23, 2013, 1:31:36 PM
#

echo -e "\nBuilding threads...\n"
make -C threads/
echo -e "\nBuilding vm...\n"
make -C vm/
echo -e "\nBuilding Examples\n"
make -C examples/
echo -e "\nBuilding filesys...\n"
make -C filesys/
echo -e "\nBuilding userprog...\n"
make -C userprog/
echo -e "\nDone"
