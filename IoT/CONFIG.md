# Initialiser `IoT.ino` avec Arduino IDE et NodeMCU 1.0

## Prérequis

- Installer Arduino IDE.
- Connecter la carte **NodeMCU 1.0 (ESP-12E Module)** au PC avec un câble USB permettant le transfert de données.
- Ouvrir le fichier `IoT.ino` dans Arduino IDE.

## Installer le support ESP8266

1. Ouvrir **Fichier > Préférences**.
2. Dans **URL de gestionnaire de cartes supplémentaires**, ajouter :
	`https://arduino.esp8266.com/stable/package_esp8266com_index.json`
3. Aller dans **Outils > Type de carte > Gestionnaire de cartes**.
4. Rechercher `ESP8266` et installer **esp8266 by ESP8266 Community**.

## Sélectionner la carte et le COM

1. Aller dans **Outils > Type de carte > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**.
2. Aller dans **Outils > Port** et sélectionner le port **COM** correspondant à la carte (par exemple `COM3`).
3. Si aucun port n’apparaît, installer le pilote USB adapté à la puce de la carte (CH340 ou CP210x), reconnecter la carte et redémarrer Arduino IDE.

## Téléverser le programme

1. Vérifier que le fichier principal s’appelle `IoT.ino` et que les éventuelles bibliothèques requises sont installées via **Outils > Gérer les bibliothèques**.
2. Cliquer sur **Vérifier** pour compiler.
3. Cliquer sur **Téléverser**.
4. Attendre le message **Téléversement terminé**. Si nécessaire, maintenir le bouton **FLASH** pendant le début du téléversement, puis le relâcher.
5. Ouvrir **Outils > Moniteur série** et sélectionner le débit indiqué dans le code (`Serial.begin(...)`).

Après chaque reconnexion, vérifier que le bon port COM est sélectionné avant de téléverser.

## Envoi des alertes (Wi-Fi + MQTT)

Le sas envoie ses événements au broker Mosquitto de la VM Sentinel, qui les
transmet au bot Telegram :

| Événement | Topic | Message |
|---|---|---|
| Porte forcée sans badge | `sentinel/sas/alarme` | `intrusion` |
| Alarme arrêtée par badge | `sentinel/sas/alarme` | `arretee:<équipe>` |
| Badge refusé / inconnu | `sentinel/sas/nfc` | `refuse` |

1. Installer la bibliothèque **PubSubClient** (Nick O'Leary) via
   **Outils > Gérer les bibliothèques**.
2. Copier `secrets.h.example` en `secrets.h` (même dossier que `IoT.ino`) et
   renseigner le Wi-Fi et l'adresse IP de la VM (`ip a` sur la VM).
   `secrets.h` est ignoré par git.
3. L'ESP8266 et la VM doivent être sur le même réseau. Si la VM est sous
   VMware, sa carte réseau doit être en mode **Bridged** (pas NAT).
4. Le Wi-Fi ne supporte que le **2,4 GHz**.

Le sas fonctionne même sans réseau : une intrusion détectée pendant une
coupure est envoyée dès le retour de la connexion.
