# Sentinel

Projet Workshop EPSI - Horizon 2080.

## Infrastructure

La V1 de l'infrastructure utilise :

- Debian Linux
- Docker
- Mosquitto MQTT
- Python
- PostgreSQL
- Grafana

## Architecture

```text
ESP8266
   |
   | MQTT
   v
Mosquitto
   |
   v
Python MQTT Listener
   |
   v
PostgreSQL
   |
   v
Grafana
```


<img width="1513" height="757" alt="SENTINEL_Architecture_V3 drawio" src="https://github.com/user-attachments/assets/c140471f-d6b1-40b4-9b26-e571d9b06512" />

## Notifications Telegram

Le listener MQTT envoie un message Telegram lorsqu'une alarme d'intrusion
(`sentinel/.../alarme`) ou un badge refusé (`sentinel/.../nfc` = `refuse`) est reçu.

### 1. Créer le bot avec BotFather

1. Dans Telegram, ouvrir une conversation avec **@BotFather**.
2. Envoyer `/newbot`, choisir un nom puis un identifiant finissant par `bot`
   (ex. `SentinelSasBot`).
3. BotFather renvoie un **token** du type `123456789:AAH...` : le garder secret.

### 2. Récupérer le chat_id

1. Ouvrir le bot et lui envoyer `/start` (ou l'ajouter à un groupe et y écrire un message).
2. Lancer :

```bash
cd backend
export TELEGRAM_BOT_TOKEN="123456789:AAH..."
python telegram_notifier.py chat_id
```

### 3. Configurer et tester

```bash
export TELEGRAM_BOT_TOKEN="123456789:AAH..."
export TELEGRAM_CHAT_ID="987654321"
python telegram_notifier.py      # envoie un message de test
python mqtt_listener.py          # notifications actives
```

Simuler une intrusion :

```bash
mosquitto_pub -h 127.0.0.1 -t sentinel/sas/A01/alarme -m intrusion
```

Sans ces variables, le listener fonctionne normalement et ignore les notifications.
Ne jamais committer le token (le fichier `.env` est déjà ignoré par git).

## Nom des équipes dans Grafana

À chaque scan, l'ESP8266 envoie sur `sentinel/sas/A01/nfc` un message
`autorise:<équipe>` ou `refuse:<équipe>` (`refuse:Inconnu` pour un badge
inconnu). Le listener range le résultat dans `payload` et le nom dans la
colonne `equipe` de la table `events`. Cette colonne est créée
automatiquement au démarrage du listener.

Requête pour un panneau **Table** Grafana (source PostgreSQL) :

```sql
SELECT
  received_at AS "Heure",
  equipe      AS "Équipe",
  CASE payload WHEN 'refuse' THEN 'Accès refusé' ELSE 'Accès autorisé' END AS "Résultat"
FROM events
WHERE event_type = 'nfc'
ORDER BY received_at DESC
LIMIT 50
```

Nombre de scans par équipe (panneau **Bar chart**) :

```sql
SELECT equipe AS "Équipe", payload AS "Résultat", COUNT(*) AS "Scans"
FROM events
WHERE event_type = 'nfc' AND $__timeFilter(received_at)
GROUP BY equipe, payload
ORDER BY "Scans" DESC
```
