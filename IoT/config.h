#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// Réglages du sas. C'est le seul fichier à modifier pour brancher la carte
// sur le réseau du vaisseau : le reste du programme n'a pas besoin d'y toucher.
// ---------------------------------------------------------------------------

// --- Wi-Fi du bord ---
// Laisser WIFI_SSID vide ("") pour faire tourner le sas sans réseau du tout :
// badges, porte et alarme continuent de fonctionner, rien n'est envoyé.
#define WIFI_SSID     ""
#define WIFI_PASSWORD ""

// --- Serveur de bord (la VM Debian 12 qui héberge l'API du dashboard) ---
// Pendant les tests, c'est l'ordinateur qui fait tourner "npm run api".
// Mettre son adresse IP sur le réseau local, pas "localhost" : pour la carte,
// "localhost" désignerait la carte elle-même.
#define SERVEUR_HOTE  "192.168.1.10"
#define SERVEUR_PORT  3000

// Les deux adresses utilisées par le dashboard.
#define ROUTE_EVENEMENTS "/evenements"
#define ROUTE_STATUT     "/statut"

// --- Nom de ce sas, repris dans les messages envoyés au dashboard ---
#define NOM_DU_SAS "Sas principal"

// --- File d'attente (mode autonome) ---
// Nombre d'événements gardés en mémoire quand le serveur ne répond pas.
// Au-delà, le plus ancien est effacé pour laisser la place au plus récent.
#define TAILLE_FILE 30

// Temps maximum accordé à une requête HTTP, en millisecondes.
// Court exprès : le sas ne doit jamais rester bloqué à attendre le réseau.
#define DELAI_HTTP_MS 1500

// Intervalle entre deux tentatives de vidage de la file, en millisecondes.
#define INTERVALLE_RENVOI_MS 5000

// Intervalle du "battement de coeur", en millisecondes : même quand il ne se
// passe rien, le sas fait signe au serveur pour que le dashboard sache qu'il
// est toujours là. Le dashboard le déclare injoignable au bout de trois
// battements manqués.
#define INTERVALLE_BATTEMENT_MS 30000

#endif
