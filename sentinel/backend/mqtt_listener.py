import paho.mqtt.client as mqtt
import psycopg
from datetime import datetime

from telegram_notifier import notifier_evenement


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

def connexion_db():

    return psycopg.connect(
        host=DB_HOST,
        port=DB_PORT,
        dbname=DB_NAME,
        user=DB_USER,
        password=DB_PASSWORD
    )


def preparer_table():

    # Colonne du nom de l'equipe qui a scanne son badge (vide sinon).
    # Ajoutee automatiquement si elle n'existe pas encore.
    try:

        with connexion_db() as conn:
            conn.execute(
                "ALTER TABLE events ADD COLUMN IF NOT EXISTS equipe TEXT"
            )

        print("[DB] Colonne equipe prete")

    except Exception as error:

        print(f"[DB] Erreur preparation table : {error}")


def separer_badge(topic, payload):

    # L'ESP envoie "autorise:Equipe Securite" sur sentinel/.../nfc.
    # On garde "autorise" dans payload et le nom de l'equipe a part.
    event_type = topic.split("/")[-1]

    if event_type == "nfc" and ":" in payload:
        resultat, _, equipe = payload.partition(":")
        return resultat, equipe

    # sentinel/.../equipe : le message est directement le nom.
    if event_type == "equipe":
        return payload, payload

    return payload, None


def get_severity(topic, payload):

    event_type = topic.split("/")[-1]

    if event_type == "alarme":
        return "critical"

    if event_type == "nfc" and payload == "refuse":
        return "warning"

    return "info"


def save_event(topic, payload, severity, equipe):

    event_type = topic.split("/")[-1]

    try:

        with connexion_db() as conn:

            with conn.cursor() as cursor:

                cursor.execute(
                    """
                    INSERT INTO events
                    (device, topic, event_type, payload, severity, equipe)
                    VALUES (%s, %s, %s, %s, %s, %s)
                    """,
                    (
                        DEVICE_NAME,
                        topic,
                        event_type,
                        payload,
                        severity,
                        equipe
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
    message_brut = message.payload.decode()

    payload, equipe = separer_badge(topic, message_brut)

    print("------------------------------")
    print(f"Date    : {date}")
    print(f"Topic   : {topic}")
    print(f"Message : {payload}")

    if equipe:
        print(f"Equipe  : {equipe}")

    severity = get_severity(topic, payload)

    save_event(topic, payload, severity, equipe)

    notifier_evenement(DEVICE_NAME, topic, message_brut, severity)


# =========================
# DEMARRAGE
# =========================

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2
)

client.on_connect = on_connect
client.on_message = on_message

preparer_table()

print("Connexion a Mosquitto...")

client.connect(
    MQTT_BROKER,
    MQTT_PORT,
    60
)

client.loop_forever()
