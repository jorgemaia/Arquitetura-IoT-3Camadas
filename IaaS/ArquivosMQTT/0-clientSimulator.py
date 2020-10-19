# Criar um Cliente de MQTT que manda a mensagem { "tipo": "d2c", "temperatura": 23.5, "umidade": 38.4, "luminosidade": 830}
# Topico: escritorio/[sala1, sala2, sala3]/iddispositivo(random)

# parametros: Qtd de Clientes, tempo de execução
# Dentro do cliente: tempo de envio tem que ser dinamico (random) entre 30s e 2 minutos


# import re
# import argparse
import uuid
import json
from typing import NamedTuple
import paho.mqtt.client as mqtt
import threading
import random
import time
from datetime import datetime, timedelta


# escritorio/local/iddispositivo
# { "tipo": "d2c", "temperatura": 23.5, "umidade": 38.4, "luminosidade": 830}
MQTTIP = '52.225.223.230'

topicoMQTT = 'escritorio/+/+'
regexMQTT = 'escritorio/([^/]+)/([^/]+)'
idclienteMQTT = 'PonteMQTTInflux'
medidas = []

LOCK = threading.Lock()


class Dados(NamedTuple):
    temperatura: float
    umidade: float
    luminosidade: float


class Contador(NamedTuple):
    GUID: str
    inicio: str
    fim: str
    contador: int


def medida2CSV(medida):
    return "{},{},{},{}\n".format(medida.GUID, medida.inicio, medida.fim, medida.contador)


def on_connect(client, userdata, flags, rc):
    print('Resultado da Tentativa de Conexão MQTT ' + str(rc))
    # client.subscribe(topicoMQTT)


def on_message(client, userdata, msg):
    print(msg.topic + ' ' + str(msg.payload))
    msg = _parse_mqtt_message(msg.topic, msg.payload.decode('utf-8'))
    print(msg.topic + ' ' + str(msg))


def _parse_mqtt_message(topic, payload):
    message = json.loads(payload)
    # match = re.match(regexMQTT, topic)
    # if match and message["tipo"] == 'd2c':
    #   local = match.group(1)
    #   iddispositivo = match.group(2)
    #   return Dados(float(message["temperatura"]),
    #                   float(message["umidade"]),
    #                   float(message["luminosidade"]))
    if message["tipo"] == 'c2d':
        return payload


def mqttf(GUID):
    mqtt_client = mqtt.Client(GUID)
    andar = [1, 2, 3, 4]

    topico = 'escritorio/Andar-' + str(random.choice(andar)) + '/' + str(GUID)
    # mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    conn=False
    retry=0

    while not conn:
        try:
            mqtt_client.connect(MQTTIP, 1883)
            conn = True
        except:
            retry = retry + 1
            if retry > 20:
                break
            time.sleep(3)
    mqtt_client.loop_start()
    contagem = 0
    inicio = datetime.now()
    fim = inicio + timedelta(minutes=3)
    while fim > datetime.now():
        time.sleep(random.uniform(15, 75))
        temperatura = random.uniform(22.2, 37.8)
        umidade = random.uniform(30.5, 62.3)
        luminosidade = random.uniform(750, 950)
        plj = {"tipo": "d2c",
               "temperatura": temperatura,
               "umidade": umidade,
               "luminosidade": luminosidade
               }
        pls = json.dumps(plj)
        print(pls)
        (rc,mid)=mqtt_client.publish(topico, pls, qos=1, retain=False)
        if (rc == 0):
            contagem = contagem + 1 
    LOCK.acquire()
    medidas.append(Contador(GUID, inicio, fim, contagem))
    LOCK.release()


def main(lote):
    threads = []
    for i in range(0, 500):
        # GUID = str(args.id) + '-' + str(i)
        GUID = "{}-{}".format(lote, i)
        print("Main    : creating device {}".format(GUID))
        x = threading.Thread(target=mqttf, args=(GUID,))
        x.start()
        threads.append(x)
    print("Devices Created!")
    for thread in threads:
        thread.join()


if __name__ == '__main__':
    print('Creating 500 devices...')
    lote = uuid.uuid4().hex[-4:]
    main(lote)
    now = datetime.now()
    csv = open("{}.csv".format(now.strftime("media/{}-%Y-%m-%d-%H-%M".format(lote))), "w")
    csv.write("GUID,Inicio,Fim,Contador\n")
    total = 0
    for medida in medidas:
        line = medida2CSV(medida)
        print(line)
        csv.write(line)
        total = total + medida.contador
    csv.write("TOTAL,,,{}\n".format(total))
    csv.close()
    print(str(total))


# Cada thread tem seu acumulador, e todas rodam durante 5 minutos.
# Ao fim de cada thread geramos um CSV com todos os Acumuladores no formato de um por linha, com quatro colunas, thread e envios dt hora inicio, dthora fim.


# 4 clientes que recebem cada um um andar e totalizam os recebimentos, estes devem fechar se tempo de inatividade de recebimento > 1 minuto.
# Ao fim da execução, gera um CSV com o total de recebimentos no formato de uma linha, com quatro colunas, andar e recebimentos dt hora inicio, dthora fim.
