import psycopg
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware


DB_HOST = "127.0.0.1"
DB_PORT = 5432
DB_NAME = "sentinel"
DB_USER = "sentinel"
DB_PASSWORD = "sentinel"


NIVEAUX = {
    "info": "normal",
    "warning": "doute",
    "critical": "critique",
}


MESSAGES = {
    "verrouillage": "Sas verrouillé",
    "ouverture": "Porte ouverte",
    "badge_accepte": "Badge accepté",
    "badge_refuse": "Badge refusé",
    "passage": "Passage confirmé",
    "presence": "Présence détectée devant le sas",
    "autorisation_annulee": "Autorisation annulée",
    "intrusion": "Alarme : intrusion détectée",
}


ZONES = {
    "SAS-A01": "securite-physique",
}


def traduire_type(event_type, payload):

    if event_type == "nfc":
        if payload == "refuse":
            return "badge_refuse"
        return "badge_accepte"

    if event_type == "alarme":
        return "intrusion"

    if event_type == "porte":
        if payload == "ouverte":
            return "ouverture"

        if payload == "fermee":
            return "verrouillage"

    if event_type == "presence":
        return "presence"

    return event_type


def traduire_verrou(type_dashboard):

    if type_dashboard in (
        "badge_accepte",
        "passage",
        "ouverture"
    ):
        return "ouvert"

    return "ferme"


def lire_lignes(requete):

    with psycopg.connect(
        host=DB_HOST,
        port=DB_PORT,
        dbname=DB_NAME,
        user=DB_USER,
        password=DB_PASSWORD
    ) as conn:

        with conn.cursor() as cursor:
            cursor.execute(requete)
            return cursor.fetchall()


app = FastAPI(
    title="API Sentinel"
)


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["GET"],
    allow_headers=["*"],
)


@app.get("/")
def accueil():

    return {
        "service": "Sentinel API",
        "status": "online"
    }


@app.get("/evenements")
def lire_evenements():

    lignes = lire_lignes(
        """
        SELECT
            id,
            received_at,
            event_type,
            payload,
            severity
        FROM events
        ORDER BY id DESC
        LIMIT 50
        """
    )

    fiches = []

    for numero, recu_le, event_type, payload, severity in lignes:

        type_dashboard = traduire_type(
            event_type,
            payload
        )

        fiches.append({

            "id": numero,

            "heure":
                recu_le.isoformat(),

            "type":
                type_dashboard,

            "niveau":
                NIVEAUX.get(
                    severity,
                    "normal"
                ),

            "verrou":
                traduire_verrou(
                    type_dashboard
                ),

            "badge":
                None,

            "message":
                MESSAGES.get(
                    type_dashboard,
                    f"{event_type} : {payload}"
                ),
        })

    return fiches


@app.get("/statut")
def lire_statut():

    return {
        "terre": "connecte",
        "enAttente": 0
    }


@app.get("/appareils")
def lire_appareils():

    lignes = lire_lignes(
        """
        SELECT DISTINCT device
        FROM events
        ORDER BY device
        """
    )

    fiches = []

    for numero, (device,) in enumerate(
        lignes,
        start=1
    ):

        fiches.append({

            "id":
                numero,

            "nom":
                device,

            "vlan":
                ZONES.get(
                    device,
                    "capteurs"
                ),

            "isole":
                False,
        })

    return fiches
