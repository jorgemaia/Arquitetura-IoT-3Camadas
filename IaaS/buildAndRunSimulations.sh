#!/bin/bash

sudo docker build -f Dockerfile -t mqtt/client .

#TODO: make the path an argument
for (( c=0; c<10; c++ ))
do  
   sudo docker run -d -v /home/JM/Workspace/Internal/Mestrado2020JM/MaquinaMQTT/csvFiles/out:/media --rm --name simulator$c mqtt/client:latest
done