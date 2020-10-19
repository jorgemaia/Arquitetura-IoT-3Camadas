import re
import json
from typing import NamedTuple
from datetime import datetime, timedelta
import threading
import time
import os
import argparse

import paho.mqtt.client as mqtt

# escritorio/local/iddispositivo
# {"tipo": "d2c", "temperatura": 23.5, "umidade": 38.4, "luminosidade": 830}
MQTTIP = '52.225.223.230'
topicoMQTT = 'escritorio/+/+'
regexMQTT = 'escritorio/([^/]+)/([^/]+)'
LOCK = threading.Lock()

ndisp=5000
# lastMessageTime = -1
# firstMessageTime = -1


class Dados(NamedTuple):
    temperatura: float
    umidade: float
    luminosidade: float
    local: str
    iddispositivo: str


def createGlobalVar():
    global contadorInst
    contadorInst = {}
    global contadorDisp
    contadorDisp = {}


def getLastTime():
    try:
        return lastMessageTime
    except:
        return None


def setLastTime(time):
    global lastMessageTime
    lastMessageTime = time
    if time is None:
        print("Set last message time to none")
    else:
        print("Set last message time: {}".format(lastMessageTime.strftime("%Y-%m-%d-%H-%M")))


def getFirstTime():
    try:
        return firstMessageTime
    except:
        return None

def setFirstTime(time):
    global firstMessageTime
    firstMessageTime = time
    if time is None:
        print("Set last message time to none")
    else:
        print("Set last message time: {}".format(firstMessageTime.strftime("%Y-%m-%d-%H-%M")))


def on_connect(client, userdata, flags, rc):
    print('Resultado da Tentativa de Conexão MQTT ' + str(rc))
    client.subscribe(topicoMQTT)


def on_message(client, userdata, msg):
    print(msg.topic + ' ' + str(msg.payload))
    if getFirstTime() is None:
        setFirstTime(datetime.now())
    setLastTime(datetime.now())
    sensor_data = _parse_mqtt_message(msg.topic, msg.payload.decode('utf-8'))


def _parse_mqtt_message(topic, payload):
    message = json.loads(payload)
    match = re.match(regexMQTT, topic)
    if match and message["tipo"] == 'd2c':
        local = match.group(1)
        iddispositivo = match.group(2)
        GUID = iddispositivo.split("-")[0]
        LOCK.acquire()
        if contadorInst.get(GUID) is None:
            contadorInst[GUID] = 1
        else:
            contadorInst[GUID] = contadorInst[GUID] + 1
        if contadorDisp.get(iddispositivo) is None:
            contadorDisp[iddispositivo] = 1
        else:
            contadorDisp[iddispositivo] = contadorDisp[iddispositivo] + 1
        LOCK.release()
        return Dados(float(message["temperatura"]), float(message["umidade"]), float(message["luminosidade"]), local, iddispositivo)
    else:
        return None


def writeFromDict(file, endTime, values):
    total = 0
    file.write("Key,Value,{}\n".format(firstMessageTime.strftime("%Y-%m-%d-%H-%M")))
    for key, value in values.items():
        file.write("{},{}\n".format(key, value))
        total = total + value
    file.write("TOTAL,{},{}".format(total, endTime.strftime("%Y-%m-%d-%H-%M")))


def writeCSVs():
    while True:
        print("Checking for message timeout...")
        if (getLastTime() is not None) and (getFirstTime() is not None):
            endTime = getLastTime() + timedelta(seconds=80)
            now = datetime.now()
            if endTime < now:
                print("Timeout reached! It has been {} seconds since the last message.".format((endTime - getLastTime()).seconds))
                
                print("Creating output folder")
                try:
                    os.mkdir("csvFiles/")
                except FileExistsError as exc:
                    print(exc)
                try:
                    os.mkdir("csvFiles/{}/".format(ndisp))
                except FileExistsError as exc:
                    print(exc)
                
                print("Starting to write the files!")
                GUIDfileName = "csvFiles/{}/instancias-{}.csv".format(ndisp,now.strftime("%Y-%m-%d-%H-%M"))
                GUIDcsv = open(GUIDfileName, "w")
                LOCK.acquire()
                writeFromDict(GUIDcsv, endTime, contadorInst)
                GUIDcsv.close()

                dispFileName = "csvFiles/{}/dispositivos-{}.csv".format(ndisp, now.strftime("%Y-%m-%d-%H-%M"))
                dispcsv = open(dispFileName, "w")
                writeFromDict(dispcsv, endTime, contadorDisp)
                LOCK.release()
                dispcsv.close()
                print("Finished writing the files!")
                print("Files:\n\t{}\n\t{}".format(GUIDfileName, dispFileName))
                setFirstTime(None)
                setLastTime(None)
                createGlobalVar()
            else:
                print("Timeout was not reached!")
        time.sleep(15)


def main():
    createGlobalVar()
    x = threading.Thread(target=writeCSVs, args=())
    x.start()

    mqtt_client = mqtt.Client('MQTT2CSV', clean_session=False)
    # mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(MQTTIP, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('MQTT to CSV bridge')
    # python3 filename.py --local 1
    parser = argparse.ArgumentParser(description='ID do Lote')
    parser.add_argument('-n', dest='n', type=int, help='Passe o numero de dispositivos que enviarão dados', default=1000)
    args = parser.parse_args()
    ndisp = args.n
    main()
