#!/bin/bash

sudo docker build -f Dockerfile -t mqtt/client .

#TODO: make the path an argument
for (( c=0; c<10; c++ ))
do  
   #sudo docker run -d -v /home/ctl-anderson/Workspace/Internal/Mestrado2020JM/MaquinaMQTT/Simulacao-0-qos0-tudojunto/csvFiles/5000:/media --rm --name simulator$c mqtt/client:latest
    sudo docker run -d -v /Users/jorgemaia/Codigos/Mestrado2020JM/MaquinaMQTT/cenario3/qos1/csvFiles/5000/execucao2:/media --rm --name simulator$c mqtt/client:latest
done