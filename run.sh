#!/bin/bash

mkdir -p build
cd build
if [[ -z $(ls) ]]
then
    cmake ..
    cp $HOME/vcs/OneChance/OneChance.choreo .
fi
rm coco
make -j$(nproc)
if [ $? ]
then
    ./coco OneChance.choreo
fi

cd ..
