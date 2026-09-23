# Sentinel

Projet Workshop EPSI - Horizon 2080.

## Infrastructure

La V1 de l'infrastructure utilise :

- Debian Linux
- Docker
- Docker Compose
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
