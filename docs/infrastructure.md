Infrastructure Sentinel V1
1. Objectif

L'infrastructure Sentinel V1 permet de centraliser, stocker et visualiser les événements envoyés par les équipements Sentinel.

L'architecture actuelle est :

Équipement Sentinel
        │
        │ MQTT
        ▼
┌───────────────────┐
│    Mosquitto      │
│   Broker MQTT     │
│      :1883        │
└─────────┬─────────┘
          │
          │ sentinel/#
          ▼
┌───────────────────┐
│ mqtt_listener.py  │
│      Python       │
└─────────┬─────────┘
          │
          │ SQL INSERT
          ▼
┌───────────────────┐
│    PostgreSQL     │
│  Base Sentinel    │
└─────────┬─────────┘
          │
          │ SQL
          ▼
┌───────────────────┐
│      Grafana      │
│    Dashboard      │
│      :3000        │
└───────────────────┘

La VM Debian héberge donc la partie centrale de supervision.

2. Environnement Linux

Serveur utilisé :

Debian Linux
VMware Workstation

Le projet se trouve dans :

/home/benjamin/sentinel

Arborescence utilisée :

sentinel/
├── backend/
│   ├── mqtt_listener.py
│   └── venv/
│
├── mosquitto/
│   └── config/
│       └── mosquitto.conf
│
├── docker-compose.yml
│
└── ...

La VM utilise actuellement une interface réseau :

ens33

La VM est configurée en DHCP afin de pouvoir fonctionner sur différents réseaux.

Configuration :

sudo nano /etc/network/interfaces

Contenu :

source /etc/network/interfaces.d/*

auto lo
iface lo inet loopback

allow-hotplug ens33
iface ens33 inet dhcp

Pour connaître l'adresse actuelle :

ip addr show ens33

Et la route réseau :

ip route
3. Docker

Docker est utilisé afin d'isoler les différents services de l'infrastructure.

Les trois conteneurs principaux sont :

sentinel-mqtt
sentinel-db
sentinel-grafana

Vérification :

docker ps

Architecture Docker :

Docker
│
├── Mosquitto
│     Port 1883
│
├── PostgreSQL
│     Port 5432
│
└── Grafana
      Port 3000
4. Docker Compose

Fichier :

/home/benjamin/sentinel/docker-compose.yml

Version utilisée durant le développement :

services:

  mosquitto:
    image: eclipse-mosquitto
    container_name: sentinel-mqtt
    restart: unless-stopped

    ports:
      - "1883:1883"

    volumes:
      - ./mosquitto/config/mosquitto.conf:/mosquitto/config/mosquitto.conf


  postgres:
    image: postgres:16
    container_name: sentinel-db
    restart: unless-stopped

    environment:
      POSTGRES_USER: sentinel
      POSTGRES_PASSWORD: sentinel
      POSTGRES_DB: sentinel

    ports:
      - "127.0.0.1:5432:5432"

    volumes:
      - postgres_data:/var/lib/postgresql/data


  grafana:
    image: grafana/grafana
    container_name: sentinel-grafana
    restart: unless-stopped

    ports:
      - "3000:3000"


volumes:
  postgres_data:

Lancement :

cd /home/benjamin/sentinel

docker compose up -d

Arrêt :

docker compose down

Vérification :

docker ps
5. Mosquitto MQTT

Mosquitto est le broker MQTT.

Son rôle est de recevoir les messages publiés par les équipements Sentinel et de les redistribuer aux applications abonnées.

Configuration :

/home/benjamin/sentinel/mosquitto/config/mosquitto.conf

Configuration V1 :

listener 1883
allow_anonymous true

Cette configuration a été choisie pour le prototype V1.

La sécurisation MQTT fait partie des évolutions prévues :

Authentification
ACL
TLS
Port 8883

Mosquitto écoute sur :

TCP 1883

Vérification :

docker logs sentinel-mqtt

Test du port :

ss -lnt | grep 1883
6. Topics MQTT

La convention utilisée est :

sentinel/sas/<ID>/<TYPE>

Exemple :

sentinel/sas/A01/status
sentinel/sas/A01/porte
sentinel/sas/A01/nfc
sentinel/sas/A01/equipe
sentinel/sas/A01/alarme
sentinel/sas/A01/alarme_etat
sentinel/sas/A01/presence

Exemples de messages :

sentinel/sas/A01/status       online
sentinel/sas/A01/porte        ouverte
sentinel/sas/A01/porte        fermee

sentinel/sas/A01/nfc          autorise
sentinel/sas/A01/nfc          refuse

sentinel/sas/A01/equipe       Equipe Medicale

sentinel/sas/A01/alarme       intrusion

sentinel/sas/A01/alarme_etat  active
sentinel/sas/A01/alarme_etat  inactive

Le wildcard :

sentinel/#

permet d'écouter tous les événements Sentinel.

7. Test MQTT

Pour observer tous les messages :

mosquitto_sub \
-h 127.0.0.1 \
-p 1883 \
-t "sentinel/#" \
-v

Exemple :

sentinel/sas/A01/nfc autorise
sentinel/sas/A01/equipe Equipe Medicale
sentinel/sas/A01/porte ouverte

Pour simuler un équipement :

mosquitto_pub \
-h 127.0.0.1 \
-p 1883 \
-t "sentinel/sas/A01/nfc" \
-m "autorise"

mosquitto_sub et mosquitto_pub sont uniquement des outils de test.

Ils ne sont pas nécessaires au fonctionnement normal de Sentinel.

8. PostgreSQL

PostgreSQL est utilisé pour stocker l'historique des événements.

Conteneur :

sentinel-db

Configuration :

Database : sentinel
User     : sentinel
Port     : 5432

Le port PostgreSQL est volontairement exposé uniquement sur :

127.0.0.1

grâce à :

ports:
  - "127.0.0.1:5432:5432"

Ainsi PostgreSQL n'est pas directement accessible depuis les autres machines du réseau.

9. Structure de la base

Table principale :

CREATE TABLE events (

    id BIGSERIAL PRIMARY KEY,

    received_at TIMESTAMPTZ
    NOT NULL
    DEFAULT NOW(),

    device VARCHAR(100)
    NOT NULL,

    topic VARCHAR(255)
    NOT NULL,

    event_type VARCHAR(100)
    NOT NULL,

    payload TEXT,

    severity VARCHAR(20)
    NOT NULL
    DEFAULT 'info'

);

Chaque événement contient donc :

Date
Équipement
Topic MQTT
Type d'événement
Valeur
Sévérité

Exemple :

SAS-A01
nfc
autorise
info

ou :

SAS-A01
alarme
intrusion
critical
10. Vérification PostgreSQL

Pour consulter les événements :

docker exec -it sentinel-db \
psql -U sentinel -d sentinel

Puis :

SELECT *
FROM events
ORDER BY received_at DESC;

Ou directement depuis Debian :

docker exec -it sentinel-db \
psql -U sentinel -d sentinel \
-c "SELECT id, received_at, device, event_type, payload, severity FROM events ORDER BY id DESC LIMIT 20;"
11. Backend Python

Le rôle du backend Python est de faire le lien entre :

Mosquitto
↓
Python
↓
PostgreSQL

Le programme écoute :

sentinel/#

et enregistre chaque événement MQTT dans PostgreSQL.

Emplacement :

/home/benjamin/sentinel/backend
12. Environnement virtuel Python

Un environnement Python isolé a été créé :

cd /home/benjamin/sentinel/backend

Création :

python3 -m venv venv

Activation :

source venv/bin/activate

Installation des bibliothèques :

pip install paho-mqtt "psycopg[binary]"

Les principales dépendances sont donc :

paho-mqtt
psycopg[binary]

Un requirements.txt GitHub peut contenir :

paho-mqtt
psycopg[binary]

Installation :

pip install -r requirements.txt
13. Listener MQTT Python

Fichier :

/home/benjamin/sentinel/backend/mqtt_listener.py

Principe :

Connexion Mosquitto
↓
Souscription sentinel/#
↓
Réception événement
↓
Analyse du topic
↓
Détermination de la sévérité
↓
INSERT PostgreSQL

Version correspondant à notre infrastructure :

import paho.mqtt.client as mqtt
import psycopg
from datetime import datetime


MQTT_HOST = "127.0.0.1"
MQTT_PORT = 1883

MQTT_TOPIC = "sentinel/#"

DEVICE_NAME = "SAS-A01"


DB_HOST = "127.0.0.1"
DB_PORT = 5432

DB_NAME = "sentinel"
DB_USER = "sentinel"
DB_PASSWORD = "sentinel"


def enregistrer_evenement(topic, message):

    event_type = topic.split("/")[-1]

    severity = "info"

    if event_type == "alarme":
        severity = "critical"

    elif event_type == "nfc" and message == "refuse":
        severity = "warning"


    try:

        with psycopg.connect(
            host=DB_HOST,
            port=DB_PORT,
            dbname=DB_NAME,
            user=DB_USER,
            password=DB_PASSWORD
        ) as conn:

            with conn.cursor() as cur:

                cur.execute(
                    """
                    INSERT INTO events
                    (
                        device,
                        topic,
                        event_type,
                        payload,
                        severity
                    )
                    VALUES
                    (
                        %s,
                        %s,
                        %s,
                        %s,
                        %s
                    )
                    """,
                    (
                        DEVICE_NAME,
                        topic,
                        event_type,
                        message,
                        severity
                    )
                )

                conn.commit()

        print("[DB] Evenement enregistre")

    except Exception as e:

        print("[DB] Erreur :", e)


def on_connect(client, userdata, flags, reason_code, properties):

    print("[MQTT] Connecte a Mosquitto")
    print("[MQTT] Ecoute :", MQTT_TOPIC)

    client.subscribe(MQTT_TOPIC)


def on_message(client, userdata, msg):

    message = msg.payload.decode()

    print("------------------------------")
    print("Date    :", datetime.now())
    print("Topic   :", msg.topic)
    print("Message :", message)

    enregistrer_evenement(
        msg.topic,
        message
    )


print("Connexion a Mosquitto...")

print("==============================")
print(" SENTINEL - MQTT LISTENER")
print("==============================")


client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2
)

client.on_connect = on_connect
client.on_message = on_message

client.connect(
    MQTT_HOST,
    MQTT_PORT,
    60
)

client.loop_forever()
14. Lancement manuel du listener

Avant automatisation :

cd /home/benjamin/sentinel/backend

Puis :

source venv/bin/activate

Et :

python mqtt_listener.py

Résultat attendu :

Connexion a Mosquitto...

==============================
 SENTINEL - MQTT LISTENER
==============================

[MQTT] Connecte a Mosquitto
[MQTT] Ecoute : sentinel/#
15. Service systemd

Afin d'éviter de lancer manuellement Python à chaque démarrage de Debian, le listener a été transformé en service systemd.

Fichier :

/etc/systemd/system/sentinel-listener.service

Contenu :

[Unit]

Description=Sentinel MQTT Listener

Wants=network-online.target

After=network-online.target docker.service


[Service]

Type=simple

User=benjamin

WorkingDirectory=/home/benjamin/sentinel/backend

ExecStart=/home/benjamin/sentinel/backend/venv/bin/python /home/benjamin/sentinel/backend/mqtt_listener.py

Restart=always

RestartSec=5

Environment=PYTHONUNBUFFERED=1


[Install]

WantedBy=multi-user.target

Activation :

sudo systemctl daemon-reload
sudo systemctl enable sentinel-listener
sudo systemctl start sentinel-listener

Vérification :

sudo systemctl status sentinel-listener

Résultat attendu :

Active: active (running)
16. Logs du listener

Le listener n'a plus besoin d'être laissé ouvert dans un terminal.

Les logs sont consultables via :

journalctl -u sentinel-listener -f

Exemple :

[MQTT] Connecte a Mosquitto
[MQTT] Ecoute : sentinel/#

Date    : 2026-09-22 15:26:26
Topic   : sentinel/sas/A01/nfc
Message : autorise

[DB] Evenement enregistre

Quitter l'affichage des logs :

Ctrl + C

Le service continue de fonctionner.

17. Grafana

Grafana est utilisé comme interface de supervision.

Conteneur :

sentinel-grafana

Port :

3000

Accès :

http://IP_VM:3000

Exemple durant le développement :

http://10.240.140.182:3000

L'adresse de la VM étant en DHCP, elle doit être vérifiée avec :

ip addr show ens33
18. Source PostgreSQL dans Grafana

Grafana communique directement avec le conteneur PostgreSQL.

Configuration datasource :

Host :
postgres:5432

Database :
sentinel

User :
sentinel

Password :
sentinel

TLS :
Disabled

Il est important d'utiliser :

postgres:5432

et non :

127.0.0.1:5432

car Grafana et PostgreSQL sont deux conteneurs Docker différents.

Docker fournit automatiquement la résolution DNS du nom :

postgres
19. Historique des événements Grafana

Panel Table :

SELECT

    received_at AS "Heure",

    device AS "SAS",

    event_type AS "Type",

    payload AS "Valeur",

    severity AS "Niveau"

FROM events

WHERE device = 'SAS-A01'

ORDER BY received_at DESC

LIMIT 50;
20. État de la porte

Panel Grafana :

Stat

Query :

SELECT

    CASE

        WHEN payload = 'ouverte'
        THEN 'OUVERTE'

        WHEN payload = 'fermee'
        THEN 'FERMEE'

        ELSE UPPER(payload)

    END AS "Porte"

FROM events

WHERE device = 'SAS-A01'

AND event_type = 'porte'

ORDER BY received_at DESC

LIMIT 1;
21. Dernier badge
SELECT

    CASE

        WHEN payload = 'autorise'
        THEN 'AUTORISE'

        WHEN payload = 'refuse'
        THEN 'REFUSE'

        ELSE UPPER(payload)

    END AS "Badge"

FROM events

WHERE device = 'SAS-A01'

AND event_type = 'nfc'

ORDER BY received_at DESC

LIMIT 1;
22. Dernière équipe identifiée
SELECT

    payload AS "Equipe"

FROM events

WHERE device = 'SAS-A01'

AND event_type = 'equipe'

ORDER BY received_at DESC

LIMIT 1;

Exemple :

Equipe Medicale
23. Nombre d'intrusions
SELECT

    COUNT(*) AS "Intrusions"

FROM events

WHERE device = 'SAS-A01'

AND event_type = 'alarme'

AND payload = 'intrusion';
24. Nombre d'accès refusés
SELECT

    COUNT(*) AS "Acces refuses"

FROM events

WHERE device = 'SAS-A01'

AND event_type = 'nfc'

AND payload = 'refuse';
25. État de l'alarme

Le système peut utiliser :

sentinel/sas/A01/alarme_etat

avec :

active
inactive

Query Grafana :

SELECT

    CASE

        WHEN payload = 'active'
        THEN 'ALARME'

        WHEN payload = 'inactive'
        THEN 'RAS'

        ELSE UPPER(payload)

    END AS "Alarme"

FROM events

WHERE device = 'SAS-A01'

AND event_type = 'alarme_etat'

ORDER BY received_at DESC

LIMIT 1;
26. Activité du SAS

Panel :

Time series

Query :

SELECT

    date_trunc(
        'minute',
        received_at
    ) AS "time",

    COUNT(*) AS "Evenements"

FROM events

WHERE device = 'SAS-A01'

GROUP BY
    date_trunc(
        'minute',
        received_at
    )

ORDER BY "time";
27. Rafraîchissement Grafana

Le dashboard a été configuré avec un rafraîchissement automatique.

Valeur utilisée :

5s

Le flux complet devient donc :

Événement
    ↓
MQTT
    ↓
Mosquitto
    ↓
Python
    ↓
PostgreSQL
    ↓
Grafana
    ↓
Affichage sous ~5 secondes
28. Démarrage automatique de Sentinel

Au démarrage de la VM :

Debian démarre
       │
       ├── Docker
       │      │
       │      ├── Mosquitto
       │      ├── PostgreSQL
       │      └── Grafana
       │
       └── systemd
              │
              └── sentinel-listener
                       │
                       └── mqtt_listener.py

Grâce à :

restart: unless-stopped

les conteneurs Docker redémarrent automatiquement.

Grâce à :

systemctl enable sentinel-listener

le listener Python démarre également automatiquement.

Ainsi, il n'est plus nécessaire de lancer :

python mqtt_listener.py

manuellement à chaque démarrage.

29. Vérifications rapides

Après un reboot de Debian :

docker ps

doit afficher :

sentinel-mqtt
sentinel-db
sentinel-grafana

Puis :

systemctl status sentinel-listener

doit afficher :

active (running)

Pour surveiller les événements :

journalctl -u sentinel-listener -f
30. Fonctionnement complet de l'infrastructure

Exemple avec un événement RFID :

1. Un événement est généré

2. Publication MQTT

   sentinel/sas/A01/nfc
   autorise

3. Mosquitto reçoit le message

4. mqtt_listener.py reçoit le message

5. Python détermine :

   device = SAS-A01
   event_type = nfc
   payload = autorise
   severity = info

6. Python écrit dans PostgreSQL

7. PostgreSQL conserve l'événement

8. Grafana interroge PostgreSQL

9. Le dashboard affiche :

   DERNIER BADGE
   AUTORISE
31. Gestion des sévérités

Le listener attribue actuellement :

Événement normal
→ info

Badge refusé
→ warning

Intrusion
→ critical

Exemple Python :

severity = "info"

if event_type == "alarme":

    severity = "critical"

elif event_type == "nfc" and message == "refuse":

    severity = "warning"

Cette information pourra être utilisée dans Grafana pour différencier visuellement les alertes.

32. Sécurité actuelle

Quelques choix de sécurité sont déjà présents.

PostgreSQL n'est accessible qu'en local :

"127.0.0.1:5432:5432"

Le backend Python s'exécute sous :

User=benjamin

et non en root.

Le listener est séparé des services Docker.

Les événements sont historisés dans PostgreSQL.

Le redémarrage automatique permet une meilleure résilience.

33. Évolutions cybersécurité prévues

La V1 étant fonctionnelle, la suite prévue concerne davantage la partie administration système et cybersécurité :

Authentification Mosquitto
ACL MQTT
TLS MQTT
Firewall Debian
Suricata IDS
Détection Nmap
Journalisation des événements cyber
Remontée des alertes cyber dans PostgreSQL
Dashboard SOC Grafana
Isolation d'équipements suspects
Gestion du mode offline
Resynchronisation

Architecture V2 envisagée :

                    SENTINEL

                       │
                       ▼
                   Mosquitto
                       │
          ┌────────────┴────────────┐
          │                         │
          ▼                         ▼
 MQTT Listener                  Suricata
          │                         │
          │                         │
          └────────────┬────────────┘
                       ▼
                   PostgreSQL
                       │
                       ▼
                    Grafana
                       │
          ┌────────────┴─────────────┐
          │                          │
          ▼                          ▼
Sécurité physique             Cybersécurité
