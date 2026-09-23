Oui. Le mieux est de mettre tout ça dans un fichier du dépôt du type :

docs/infrastructure.md

Je te prépare ci-dessous une version complète, propre et crédible pour GitHub, avec objectif, technologies, architecture, commandes, scripts, fonctionnement, difficultés rencontrées, solutions, état actuel et évolutions prévues.

SENTINEL — Infrastructure V1
1. Objectif de l’infrastructure

L’objectif de l’infrastructure Sentinel est de fournir une plateforme locale capable de :

recevoir les événements générés par les équipements Sentinel ;
transporter les événements via MQTT ;
traiter les données reçues ;
conserver un historique des événements ;
afficher les données dans un dashboard de supervision ;
continuer à superviser le système sans dépendre d’une connexion Internet ;
permettre par la suite l’ajout de fonctions de cybersécurité : IDS, détection de scans réseau, authentification MQTT, firewall, alertes, etc.

L’infrastructure a été pensée pour séparer le fonctionnement physique critique du SAS et la supervision.

Le SAS doit donc pouvoir continuer à fonctionner localement même si l’infrastructure de supervision devient indisponible.

2. Architecture générale
                     SENTINEL V1

                ÉQUIPEMENT / ESP8266
                        │
                        │
                  Wi-Fi + MQTT
                        │
                        ▼
                ┌───────────────┐
                │   Mosquitto   │
                │ Broker MQTT   │
                │   TCP 1883    │
                └───────┬───────┘
                        │
                        │ sentinel/#
                        ▼
                ┌───────────────┐
                │ Python        │
                │ MQTT Listener │
                └───────┬───────┘
                        │
                     SQL INSERT
                        │
                        ▼
                ┌───────────────┐
                │ PostgreSQL    │
                │ Base Sentinel │
                └───────┬───────┘
                        │
                   Requêtes SQL
                        │
                        ▼
                ┌───────────────┐
                │    Grafana    │
                │   Dashboard   │
                └───────────────┘

La partie centrale est hébergée sur une VM Debian.

3. Technologies utilisées
Technologie	Utilisation	Pourquoi
Debian Linux	Système serveur	Léger, stable, adapté à l’administration système
VMware Workstation	Hébergement de la VM	Permet d’isoler l’infrastructure du poste Windows
Docker	Conteneurisation	Déploiement simple et séparation des services
Docker Compose	Orchestration	Permet de lancer toute l’infrastructure avec un seul fichier
Eclipse Mosquitto	Broker MQTT	Léger et adapté aux communications IoT
MQTT	Transport des événements	Protocole léger, asynchrone et adapté à l’IoT
Python	Traitement des événements	Simple à intégrer avec MQTT et PostgreSQL
Paho MQTT	Client MQTT Python	Permet à Python de recevoir les messages Mosquitto
Psycopg	Connexion PostgreSQL	Permet à Python d’insérer les événements dans la base
PostgreSQL 16	Base de données	Stockage durable et structuré des événements
Grafana	Supervision	Création rapide de dashboards à partir de PostgreSQL
systemd	Automatisation	Prévu pour démarrer automatiquement le listener Python
4. Environnement serveur

Le serveur Sentinel fonctionne sous :

Debian Linux

virtualisé avec :

VMware Workstation

Le projet est installé dans :

/home/benjamin/sentinel

Arborescence actuelle :

sentinel/
│
├── backend/
│   ├── mqtt_listener.py
│   └── venv/
│
├── mosquitto/
│   └── config/
│       └── mosquitto.conf
│
└── docker-compose.yml
5. Configuration réseau

La VM utilise l’interface :

ens33

La VM a d’abord été utilisée en NAT VMware.

Pour permettre à l’ESP8266 et aux autres machines du réseau de joindre directement Mosquitto, le réseau VMware a ensuite été passé en :

Bridged / VMnet0

La VM obtient actuellement son adresse via DHCP.

Configuration Debian :

sudo nano /etc/network/interfaces

Configuration :

source /etc/network/interfaces.d/*

auto lo
iface lo inet loopback

allow-hotplug ens33
iface ens33 inet dhcp

Vérification de l’adresse :

ip addr show ens33

Vérification de la route :

ip route
6. Point de blocage : adresse IP dynamique

Un problème rencontré concerne le DHCP.

L’adresse IP de la VM peut changer après un changement de réseau ou un redémarrage.

Exemple :

10.240.140.182

puis potentiellement :

10.240.140.xxx

Cela pose un problème car les clients MQTT utilisent actuellement directement l’adresse IP du broker.

Conséquences :

IP VM change
   │
   ├── adresse MQTT à modifier côté client
   ├── URL Grafana différente
   └── connexion SSH différente

Solution actuelle :

ip addr show ens33

avant de vérifier la configuration MQTT.

Évolution prévue :

réservation DHCP ;
IP fixe ;
ou résolution du broker par nom DNS/mDNS.
7. Docker

Docker permet de faire fonctionner séparément les différents services.

Services actuellement conteneurisés :

sentinel-mqtt
sentinel-db
sentinel-grafana

Vérification :

docker ps

Architecture :

Docker
│
├── sentinel-mqtt
│      └── Eclipse Mosquitto
│
├── sentinel-db
│      └── PostgreSQL 16
│
└── sentinel-grafana
       └── Grafana
8. Docker Compose

Fichier :

/home/benjamin/sentinel/docker-compose.yml

Configuration utilisée pendant la V1 :

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

Redémarrage :

docker compose restart

Vérification :

docker ps
9. Persistance des données PostgreSQL

Un volume Docker est utilisé :

volumes:
  postgres_data:

Puis :

- postgres_data:/var/lib/postgresql/data

Cela permet de conserver les données PostgreSQL même si le conteneur est recréé.

10. Mosquitto

Mosquitto joue le rôle de broker MQTT.

Il ne stocke pas les données métier dans PostgreSQL lui-même.

Son rôle est :

Publisher
   ↓
Mosquitto
   ↓
Subscribers

Dans Sentinel :

Équipement
   ↓
Mosquitto
   ↓
mqtt_listener.py

Configuration :

/home/benjamin/sentinel/mosquitto/config/mosquitto.conf

V1 :

listener 1883
allow_anonymous true

Port :

1883/TCP
11. Pourquoi MQTT ?

MQTT a été choisi car il correspond bien à une architecture IoT.

Il permet :

publication asynchrone ;
faible consommation réseau ;
découplage entre l’équipement et les applications ;
architecture publisher/subscriber ;
utilisation de topics hiérarchiques ;
fonctionnement sur un réseau local.

L’équipement n’a donc pas besoin de connaître PostgreSQL ou Grafana.

Il connaît uniquement :

Broker MQTT
12. Convention des topics

Structure retenue :

sentinel/sas/<ID>/<TYPE>

Exemple pour SAS-A01 :

sentinel/sas/A01/status
sentinel/sas/A01/porte
sentinel/sas/A01/nfc
sentinel/sas/A01/equipe
sentinel/sas/A01/alarme
sentinel/sas/A01/alarme_etat
sentinel/sas/A01/presence

Exemple :

sentinel/sas/A01/nfc autorise

ou :

sentinel/sas/A01/porte ouverte
13. Wildcard MQTT

Le listener Python utilise :

sentinel/#

Le caractère :

#

indique :

écouter tous les sous-topics commençant par sentinel/.

Ainsi le listener peut recevoir tous les événements actuels et futurs.

14. Test Mosquitto

Pour observer les messages MQTT :

mosquitto_sub \
-h 127.0.0.1 \
-p 1883 \
-t "sentinel/#" \
-v

Exemple obtenu :

sentinel/sas/A01/nfc autorise

Test de publication :

mosquitto_pub \
-h 127.0.0.1 \
-p 1883 \
-t "sentinel/sas/A01/nfc" \
-m "autorise"

Ces commandes sont des outils de diagnostic uniquement.

Elles ne sont pas nécessaires en fonctionnement normal.

15. PostgreSQL

PostgreSQL permet de stocker durablement les événements.

Configuration :

Database : sentinel
User     : sentinel
Port     : 5432

Le port a été volontairement exposé uniquement sur localhost :

ports:
  - "127.0.0.1:5432:5432"

Cela signifie qu’une autre machine du réseau ne peut pas directement se connecter à PostgreSQL via le port 5432 de la VM.

16. Table events

Table utilisée :

CREATE TABLE events (
    id BIGSERIAL PRIMARY KEY,
    received_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    device VARCHAR(100) NOT NULL,
    topic VARCHAR(255) NOT NULL,
    event_type VARCHAR(100) NOT NULL,
    payload TEXT,
    severity VARCHAR(20) NOT NULL DEFAULT 'info'
);

Exemple de données :

id          25
device      SAS-A01
event_type  nfc
payload     autorise
severity    info

Ou :

device      SAS-A01
event_type  alarme
payload     intrusion
severity    critical
17. Consultation de PostgreSQL

Connexion :

docker exec -it sentinel-db \
psql -U sentinel -d sentinel

Lecture :

SELECT *
FROM events
ORDER BY received_at DESC;

Ou directement :

docker exec -it sentinel-db \
psql -U sentinel -d sentinel \
-c "SELECT id, received_at, device, event_type, payload, severity FROM events ORDER BY id DESC LIMIT 20;"
18. Backend Python

Un programme Python fait le lien entre MQTT et PostgreSQL.

Architecture :

Mosquitto
   ↓
Paho MQTT
   ↓
mqtt_listener.py
   ↓
Psycopg
   ↓
PostgreSQL

Répertoire :

/home/benjamin/sentinel/backend
19. Environnement virtuel Python

Création :

cd /home/benjamin/sentinel/backend
python3 -m venv venv

Activation :

source venv/bin/activate

Installation :

pip install paho-mqtt "psycopg[binary]"

Dépendances :

paho-mqtt
psycopg[binary]

Pour GitHub, création recommandée de :

requirements.txt

avec :

paho-mqtt
psycopg[binary]

Puis installation :

pip install -r requirements.txt
20. mqtt_listener.py

Le listener :

se connecte à Mosquitto ;
s’abonne à sentinel/# ;
reçoit les messages ;
détermine le type d’événement ;
attribue une sévérité ;
insère l’événement dans PostgreSQL.

Version utilisée :

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
21. Gestion des sévérités

Logique actuelle :

Événement classique
→ info

Accès NFC refusé
→ warning

Intrusion
→ critical

Python :

severity = "info"

if event_type == "alarme":
    severity = "critical"

elif event_type == "nfc" and message == "refuse":
    severity = "warning"
22. Lancement du listener

Actuellement, lancement manuel :

cd /home/benjamin/sentinel/backend
source venv/bin/activate
python mqtt_listener.py

Résultat :

Connexion a Mosquitto...

==============================
 SENTINEL - MQTT LISTENER
==============================

[MQTT] Connecte a Mosquitto
[MQTT] Ecoute : sentinel/#

Le programme reste volontairement lancé en permanence car il attend les événements MQTT.

23. Automatisation systemd

Cette partie est prévue / en cours de mise en place.

Objectif :

VM démarre
   ↓
listener démarre automatiquement

Service prévu :

/etc/systemd/system/sentinel-listener.service

Configuration :

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

Logs :

journalctl -u sentinel-listener -f

Cette étape doit être validée avant de la considérer comme terminée dans le projet.

24. Grafana

Grafana permet de créer le dashboard de supervision.

Accès :

http://IP_VM:3000

Exemple :

http://10.240.140.182:3000

L’adresse dépend actuellement du DHCP.

25. Connexion Grafana → PostgreSQL

Datasource :

PostgreSQL

Configuration :

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

Pourquoi :

postgres:5432

et pas :

127.0.0.1:5432

Parce que Grafana et PostgreSQL sont deux conteneurs différents.

Docker Compose crée un réseau interne où le service PostgreSQL est accessible via son nom :

postgres
26. Dashboard Grafana
Historique
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
27. État de la porte
SELECT
    CASE
        WHEN payload = 'ouverte' THEN 'OUVERTE'
        WHEN payload = 'fermee' THEN 'FERMEE'
        ELSE UPPER(payload)
    END AS "Porte"
FROM events
WHERE device = 'SAS-A01'
AND event_type = 'porte'
ORDER BY received_at DESC
LIMIT 1;
28. Dernier badge
SELECT
    CASE
        WHEN payload = 'autorise' THEN 'AUTORISE'
        WHEN payload = 'refuse' THEN 'REFUSE'
        ELSE UPPER(payload)
    END AS "Badge"
FROM events
WHERE device = 'SAS-A01'
AND event_type = 'nfc'
ORDER BY received_at DESC
LIMIT 1;
29. Dernière équipe
SELECT
    payload AS "Equipe"
FROM events
WHERE device = 'SAS-A01'
AND event_type = 'equipe'
ORDER BY received_at DESC
LIMIT 1;
30. Intrusions
SELECT
    COUNT(*) AS "Intrusions"
FROM events
WHERE device = 'SAS-A01'
AND event_type = 'alarme'
AND payload = 'intrusion';
31. Accès refusés
SELECT
    COUNT(*) AS "Acces refuses"
FROM events
WHERE device = 'SAS-A01'
AND event_type = 'nfc'
AND payload = 'refuse';
32. Activité
SELECT
    date_trunc('minute', received_at) AS "time",
    COUNT(*) AS "Evenements"
FROM events
WHERE device = 'SAS-A01'
GROUP BY date_trunc('minute', received_at)
ORDER BY "time";
33. Actualisation Grafana

Le dashboard a été configuré avec :

Refresh : 5 secondes

Avant cette configuration, les événements apparaissaient bien dans PostgreSQL mais le dashboard Grafana ne se rafraîchissait pas automatiquement.

Ce problème a été corrigé en activant l’auto-refresh.

34. Flux complet validé

La chaîne suivante a été validée :

Événement
   ↓
MQTT
   ↓
Mosquitto
   ↓
mqtt_listener.py
   ↓
PostgreSQL
   ↓
Grafana

Exemple réel :

Topic :
sentinel/sas/A01/nfc

Payload :
autorise

Réception Python :

Date    : 2026-09-22 15:26:26
Topic   : sentinel/sas/A01/nfc
Message : autorise

[DB] Evenement enregistre

Le même événement a été observé simultanément via :

mosquitto_sub

et dans le listener Python.

35. Points de blocage rencontrés
Adresse IP dynamique

Problème :

La VM est en DHCP, donc son IP peut changer.

Impact :

connexion MQTT externe ;
Grafana ;
SSH.

Solution temporaire :

ip addr show ens33

Évolution :

IP fixe / DHCP reservation / DNS
Mosquitto non accessible depuis l’extérieur

Au début, la VM était derrière le NAT VMware.

Cela rendait la communication directe avec les équipements externes plus compliquée.

Solution :

VMware Bridged Network
Configuration Mosquitto

Mosquitto ne permettait pas immédiatement les connexions externes.

Pour la V1 :

listener 1883
allow_anonymous true

Cela simplifie la démonstration, mais ce n’est pas une configuration cible de production.

Listener Python bloqué dans le terminal

Lors du lancement :

python mqtt_listener.py

la commande ne rend pas la main.

Ce n’est pas un bug.

Le listener utilise :

client.loop_forever()

et doit rester actif pour recevoir les événements.

Solution cible :

service systemd
Plusieurs terminaux lors des tests

Durant le diagnostic, trois terminaux étaient parfois utilisés :

Terminal 1
mqtt_listener.py

Terminal 2
mosquitto_sub

Terminal 3
mosquitto_pub

Ils servaient uniquement aux tests.

En fonctionnement normal :

mosquitto_sub

et :

mosquitto_pub

ne sont pas nécessaires.

Grafana non actualisé automatiquement

Les données arrivaient correctement dans PostgreSQL mais le dashboard ne changeait qu’après rafraîchissement manuel.

Solution :

Grafana Auto Refresh = 5s
Connexion PostgreSQL depuis Grafana

La première difficulté était de comprendre que :

localhost

dans le conteneur Grafana désigne Grafana lui-même.

Solution :

postgres:5432

grâce au réseau Docker Compose.

36. État actuel de la V1

Fonctionnel :

✅ Debian
✅ VMware bridged network
✅ Docker
✅ Docker Compose
✅ Mosquitto
✅ MQTT
✅ PostgreSQL
✅ Persistance PostgreSQL
✅ Listener Python
✅ Paho MQTT
✅ Psycopg
✅ Historisation des événements
✅ Gestion des niveaux info / warning / critical
✅ Grafana
✅ Dashboard
✅ Rafraîchissement automatique
✅ Tests MQTT mosquitto_pub / mosquitto_sub

À finaliser :

⬜ Automatisation complète via systemd
⬜ Stabilisation de l'adresse IP
⬜ Gestion des secrets
37. Sécurité actuelle

Quelques mesures sont déjà présentes.

PostgreSQL n’est exposé que sur localhost :

127.0.0.1:5432

Les différents services sont séparés dans leurs conteneurs.

Les données PostgreSQL sont persistantes.

Le backend Python est séparé des conteneurs.

38. Limites de sécurité de la V1

Mosquitto utilise encore :

allow_anonymous true

Les communications MQTT ne sont pas chiffrées.

Les identifiants PostgreSQL apparaissent encore dans les fichiers.

Il n’y a pas encore :

ACL MQTT
TLS
Firewall configuré spécifiquement
IDS
IPS
Détection de scans
Gestion centralisée des secrets
39. GitHub — ne pas publier les secrets

Avant publication publique, les mots de passe doivent être retirés du code.

Exemple Docker :

environment:
  POSTGRES_USER: ${POSTGRES_USER}
  POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
  POSTGRES_DB: ${POSTGRES_DB}

Créer :

.env.example
POSTGRES_USER=sentinel
POSTGRES_PASSWORD=CHANGE_ME
POSTGRES_DB=sentinel

Et :

.gitignore
.env
venv/
__pycache__/
*.pyc
.vscode/
.idea/

Le fichier réel :

.env

ne doit pas être poussé sur GitHub.

40. Améliorations prévues

La V2 doit renforcer la partie administration système / cybersécurité.

Évolutions envisagées :

Authentification Mosquitto
↓
ACL MQTT
↓
MQTT TLS
↓
Firewall Debian
↓
Suricata IDS
↓
Détection de scans Nmap
↓
Logs cyber
↓
PostgreSQL
↓
Grafana
41. Détection réseau prévue

Un des objectifs suivants est de détecter une activité comme :

Scan Nmap

Architecture cible :

Machine de test
      │
      │ scan
      ▼
Debian
      │
      ▼
Suricata IDS
      │
      ▼
Alerte cyber
      │
      ▼
PostgreSQL
      │
      ▼
Grafana

Cela permettra de réunir dans un même dashboard :

Supervision opérationnelle
+
Supervision cybersécurité
42. Architecture cible V2
                       SENTINEL V2

                           │
                           ▼
                       Mosquitto
                           │
                 ┌─────────┴─────────┐
                 │                   │
                 ▼                   ▼
          MQTT Listener          Suricata IDS
                 │                   │
                 │                   │
                 └─────────┬─────────┘
                           │
                           ▼
                      PostgreSQL
                           │
                           ▼
                        Grafana
                           │
             ┌─────────────┴─────────────┐
             │                           │
             ▼                           ▼
     Supervision système          Cybersécurité
43. Résumé du fonctionnement

La V1 repose sur une architecture découplée.

Mosquitto transporte les événements.

Python assure le traitement.

PostgreSQL assure la persistance.

Grafana assure la visualisation.

Docker simplifie le déploiement et sépare les services.

MQTT
  ↓
Mosquitto
  ↓
Python
  ↓
PostgreSQL
  ↓
Grafana

Cette architecture constitue la base qui permettra ensuite d’ajouter les fonctionnalités de cybersécurité et de résilience de Sentinel.
