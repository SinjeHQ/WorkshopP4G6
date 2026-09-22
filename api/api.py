"""
API HTTP de Sentinel, pour le dashboard React.

Elle lit la table events de PostgreSQL et renvoie les trois fiches
attendues par le dashboard : /evenements, /statut et /appareils.

Installation, dans le même environnement Python que mqtt_listener.py :
    pip install fastapi uvicorn

Lancement, dans le dossier de ce fichier :
    uvicorn api:app --host 0.0.0.0 --port 8000
"""

import psycopg
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware


# =========================
# CONFIGURATION POSTGRESQL
# (la même que dans mqtt_listener.py)
# =========================

DB_HOST = "127.0.0.1"
DB_PORT = 5432
DB_NAME = "sentinel"
DB_USER = "sentinel"
DB_PASSWORD = "sentinel"


# =========================
# TRADUCTION : TABLE events -> FICHES DU DASHBOARD
# =========================

# severity (table) -> niveau (dashboard)
NIVEAUX = {
    "info": "normal",
    "warning": "doute",
    "critical": "critique",
}

# Texte affiché dans l'historique du dashboard, pour chaque type
MESSAGES = {
    "verrouillage": "Sas verrouillé",
    "badge_accepte": "Badge accepté",
    "badge_refuse": "Badge refusé",
    "passage": "Passage confirmé",
    "autorisation_annulee": "Autorisation annulée",
    "intrusion": "Alarme : intrusion détectée",
}

# Zone réseau (VLAN) de chaque appareil ; les autres vont dans "capteurs"
ZONES = {
    "SAS-A01": "securite-physique",
}


def traduire_type(event_type, payload):
    """Transforme event_type et payload de la table en type du dashboard.
    À compléter avec les autres messages que l'ESP publie."""
    if event_type == "nfc":
        if payload == "refuse":
            return "badge_refuse"
        return "badge_accepte"
    if event_type == "alarme":
        return "intrusion"
    return event_type


def traduire_verrou(type_dashboard):
    """Le verrou est ouvert après un badge accepté ou un passage, fermé sinon."""
    if type_dashboard in ("badge_accepte", "passage"):
        return "ouvert"
    return "ferme"


# =========================
# LECTURE POSTGRESQL
# =========================

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


# =========================
# L'API
# =========================

app = FastAPI(title="API Sentinel")

# Autorise le navigateur à appeler l'API depuis le dashboard (CORS)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["GET"],
    allow_headers=["*"],
)


@app.get("/evenements")
def lire_evenements():
    lignes = lire_lignes(
        """
        SELECT id, received_at, event_type, payload, severity
        FROM events
        ORDER BY id DESC
        LIMIT 50
        """
    )

    fiches = []
    for numero, recu_le, event_type, payload, severity in lignes:
        type_dashboard = traduire_type(event_type, payload)
        fiches.append({
            "id": numero,
            "heure": recu_le.isoformat(),
            "type": type_dashboard,
            "niveau": NIVEAUX.get(severity, "normal"),
            "verrou": traduire_verrou(type_dashboard),
            "badge": None,
            "message": MESSAGES.get(type_dashboard, f"{event_type} : {payload}"),
        })
    return fiches


@app.get("/statut")
def lire_statut():
    # Le lien avec la Terre n'est pas encore géré par la pile :
    # valeur fixe pour l'instant.
    return {"terre": "connecte", "enAttente": 0}


@app.get("/appareils")
def lire_appareils():
    lignes = lire_lignes("SELECT DISTINCT device FROM events ORDER BY device")

    fiches = []
    for numero, (device,) in enumerate(lignes, start=1):
        fiches.append({
            "id": numero,
            "nom": device,
            "vlan": ZONES.get(device, "capteurs"),
            "isole": False,
        })
    return fiches
