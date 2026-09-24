import paho.mqtt.client as mqtt
import psycopg
from datetime import datetime


# =========================
# CONFIGURATION MQTT
# =========================

MQTT_BROKER = "127.0.0.1"
MQTT_PORT = 1883
MQTT_TOPIC = "sentinel/#"


# =========================
# CONFIGURATION POSTGRESQL
# =========================

DB_HOST = "127.0.0.1"
DB_PORT = 5432
DB_NAME = "sentinel"
DB_USER = "sentinel"
DB_PASSWORD = "sentinel"

DEVICE_NAME = "SAS-A01"


# =========================
# ENREGISTREMENT POSTGRESQL
# =========================

def save_event(topic, payload):

    event_type = topic.split("/")[-1]

    severity = "info"

    if event_type == "alarme":
        severity = "critical"

    elif event_type == "nfc" and payload == "refuse":
        severity = "warning"

    try:

        with psycopg.connect(
            host=DB_HOST,
            port=DB_PORT,
            dbname=DB_NAME,
            user=DB_USER,
            password=DB_PASSWORD
        ) as conn:

            with conn.cursor() as cursor:

                cursor.execute(
                    """
                    INSERT INTO events
                    (device, topic, event_type, payload, severity)
                    VALUES (%s, %s, %s, %s, %s)
                    """,
                    (
                        DEVICE_NAME,
                        topic,
                        event_type,
                        payload,
                        severity
                    )
                )

        print("[DB] Evenement enregistre")

    except Exception as error:

        print(f"[DB] Erreur PostgreSQL : {error}")


# =========================
# CONNEXION MQTT
# =========================

def on_connect(
    client,
    userdata,
    flags,
    reason_code,
    properties=None
):

    if reason_code == 0:

        print("==============================")
        print(" SENTINEL - MQTT LISTENER")
        print("==============================")

        print("[MQTT] Connecte a Mosquitto")
        print(f"[MQTT] Ecoute : {MQTT_TOPIC}")

        client.subscribe(MQTT_TOPIC)

    else:

        print(
            f"[MQTT] Erreur connexion : {reason_code}"
        )


# =========================
# RECEPTION MQTT
# =========================

def on_message(
    client,
    userdata,
    message
):

    date = datetime.now().strftime(
        "%Y-%m-%d %H:%M:%S"
    )

    topic = message.topic
    payload = message.payload.decode()

    print("------------------------------")
    print(f"Date    : {date}")
    print(f"Topic   : {topic}")
    print(f"Message : {payload}")

    save_event(topic, payload)


# =========================
# DEMARRAGE
# =========================

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2
)

client.on_connect = on_connect
client.on_message = on_message

print("Connexion a Mosquitto...")

client.connect(
    MQTT_BROKER,
    MQTT_PORT,
    60
)

client.loop_forever()
