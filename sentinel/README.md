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


<img width="1513" height="757" alt="SENTINEL_Architecture_V3 drawio" src="https://github.com/user-attachments/assets/c140471f-d6b1-40b4-9b26-e571d9b06512" />
