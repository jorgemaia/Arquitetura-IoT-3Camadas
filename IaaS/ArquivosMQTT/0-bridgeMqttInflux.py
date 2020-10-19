import re
import json
from typing import NamedTuple
from datetime import datetime, timedelta
import threading
import time
import argparse

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

# escritorio/local/iddispositivo
# {"tipo": "d2c", "temperatura": 23.5, "umidade": 38.4, "luminosidade": 830}
MQTTIP = '52.225.223.230'
InfluxIP = '52.225.223.230'
topicoMQTT = 'escritorio/+/+'
regexMQTT = 'escritorio/([^/]+)/([^/]+)'
idclienteMQTT = 'PonteMQTTInflux'
ndisp = 1000
qos = 0
cenario = 2
influxdb_client = InfluxDBClient(InfluxIP, 8086, 'mqttuser', 'mqtt', None)


class Dados(NamedTuple):
    temperatura: float
    umidade: float
    luminosidade: float
    local: str
    iddispositivo: str


def on_connect(client, userdata, flags, rc):
    print('Resultado da Tentativa de Conexão MQTT ' + str(rc))
    client.subscribe(topicoMQTT)


def on_message(client, userdata, msg):
    print(msg.topic + ' ' + str(msg.payload))
    sensor_data = _parse_mqtt_message(msg.topic, msg.payload.decode('utf-8'))
    if sensor_data is not None:
        _send_sensor_data_to_influxdb(sensor_data)


def _parse_mqtt_message(topic, payload):
    message = json.loads(payload)
    match = re.match(regexMQTT, topic)
    if match and message["tipo"] == 'd2c':
        local = match.group(1)
        iddispositivo = match.group(2)
        return Dados(float(message["temperatura"]), float(message["umidade"]), float(message["luminosidade"]), local, iddispositivo)
    else:
        return None


def _send_sensor_data_to_influxdb(sensor_data):
    json_body = [
        {
            'measurement': 'qos{}cenario{}qtd{}'.format(qos, cenario, ndisp),
            'tags': {
                'iddispositivo': sensor_data.iddispositivo,
                'local': sensor_data.local
            },
            'fields': {
                'temperatura': sensor_data.temperatura,
                'umidade': sensor_data.umidade,
                'luminosidade': sensor_data.luminosidade
            }
        }
    ]
    influxdb_client.write_points(json_body)
    print('Medida Salva' + str(json_body))


def _init_influxdb_database():
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == 'SensorInteligente', databases))) == 0:
        influxdb_client.create_database('SensorInteligente')
    influxdb_client.switch_database('SensorInteligente')


def main():
    _init_influxdb_database()

    mqtt_client = mqtt.Client('MQTT2Influx')
    # mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(MQTTIP, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('MQTT to InfluxDB bridge')
    # python3 filename.py --local 1
    parser = argparse.ArgumentParser(description='ID do Lote')
    parser.add_argument('--qos', dest='qos', type=int, help='Passe o QoS', default=0)
    parser.add_argument('-n', dest='n', type=int, help='Passe o numero de dispositivos que enviarão dados', default=1000)
    parser.add_argument('-c', dest='c', type=int, help='Passe o numero do cenario', default=1)
    args = parser.parse_args()
    qos = args.qos
    ndisp = args.n
    cenario = args.c
    main()
