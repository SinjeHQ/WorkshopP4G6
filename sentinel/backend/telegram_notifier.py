import json
import os
import threading
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime


# =========================
# CONFIGURATION TELEGRAM
# =========================
#
# Le token est donne par @BotFather, le chat_id se recupere avec :
#   python telegram_notifier.py chat_id
#
# Ne jamais ecrire le token dans le code : utiliser les variables
# d'environnement (ou le fichier .env, deja ignore par git).

def lire_variable(nom):

    # Retire espaces, guillemets et \r (fichier .env edite sous Windows).
    return os.getenv(nom, "").strip().strip("\"'").strip()


TELEGRAM_BOT_TOKEN = lire_variable("TELEGRAM_BOT_TOKEN")
TELEGRAM_CHAT_ID = lire_variable("TELEGRAM_CHAT_ID")

API_URL = "https://api.telegram.org/bot{token}/{method}"


# =========================
# MESSAGES
# =========================

TITRES = {
    "critical": "🚨 ALERTE INTRUSION",
    "warning": "⚠️ Tentative d'acces refusee",
}


def telegram_actif():

    return bool(TELEGRAM_BOT_TOKEN and TELEGRAM_CHAT_ID)


def appeler_api(method, params=None):

    url = API_URL.format(
        token=TELEGRAM_BOT_TOKEN,
        method=method
    )

    data = None

    if params is not None:
        data = urllib.parse.urlencode(params).encode()

    try:
        with urllib.request.urlopen(url, data=data, timeout=10) as reponse:
            return json.loads(reponse.read().decode())

    except urllib.error.HTTPError as error:
        # Telegram explique la cause dans le corps de la reponse.
        try:
            detail = json.loads(error.read().decode()).get("description")
        except Exception:
            detail = None

        raise RuntimeError(
            f"HTTP {error.code} - {detail or error.reason}"
        ) from None


def envoyer_message(texte):

    try:

        appeler_api(
            "sendMessage",
            {
                "chat_id": TELEGRAM_CHAT_ID,
                "text": texte,
            }
        )

        print("[TELEGRAM] Notification envoyee")

    except Exception as error:

        print(f"[TELEGRAM] Erreur envoi : {error}")


def notifier_evenement(device, topic, payload, severity):

    if severity not in TITRES:
        return

    if not telegram_actif():
        print("[TELEGRAM] Token ou chat_id manquant, notification ignoree")
        return

    date = datetime.now().strftime("%d/%m/%Y %H:%M:%S")

    texte = (
        f"{TITRES[severity]}\n"
        f"\n"
        f"Equipement : {device}\n"
        f"Topic : {topic}\n"
        f"Message : {payload}\n"
        f"Date : {date}"
    )

    # Envoi dans un thread pour ne pas bloquer la boucle MQTT
    # si Internet est lent ou indisponible.
    threading.Thread(
        target=envoyer_message,
        args=(texte,),
        daemon=True
    ).start()


# =========================
# OUTILS EN LIGNE DE COMMANDE
# =========================

def afficher_chat_ids():

    reponse = appeler_api("getUpdates")

    chats = {}

    for update in reponse.get("result", []):
        message = update.get("message") or update.get("channel_post") or {}
        chat = message.get("chat")

        if chat:
            chats[chat["id"]] = chat.get("title") or chat.get("first_name")

    if not chats:
        print("Aucun message recu : envoyez /start au bot puis relancez.")
        return

    for chat_id, nom in chats.items():
        print(f"chat_id = {chat_id}  ({nom})")


if __name__ == "__main__":

    import sys

    if not TELEGRAM_BOT_TOKEN:
        print("Definir TELEGRAM_BOT_TOKEN avant de lancer ce script.")
        sys.exit(1)

    if len(sys.argv) > 1 and sys.argv[1] == "chat_id":
        afficher_chat_ids()

    else:
        if not TELEGRAM_CHAT_ID:
            print("Definir TELEGRAM_CHAT_ID (voir : python telegram_notifier.py chat_id)")
            sys.exit(1)

        envoyer_message("✅ Test Sentinel : le bot Telegram fonctionne.")
