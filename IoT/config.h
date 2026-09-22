#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// Réglages du sas. C'est le seul fichier à modifier pour brancher la carte
// sur le réseau du vaisseau : le reste du programme n'a pas besoin d'y toucher.
// ---------------------------------------------------------------------------

// --- Wi-Fi du bord ---
// Laisser WIFI_SSID vide ("") pour faire tourner le sas sans réseau du tout :
// badges, porte et alarme continuent de fonctionner, rien n'est envoyé.
#define WIFI_SSID     "S25 de Benjamin"
#define WIFI_PASSWORD "passwords"

// --- Serveur de bord (la machine qui fait tourner "npm run api") ---
//
// >>> LA SEULE VALEUR QUI RESTE A REMPLIR <<<
//
// Connecter le PC au partage de connexion, puis relever son adresse :
//   Windows : ipconfig        -> ligne "Adresse IPv4"
//   Mac     : ipconfig getifaddr en0
//   Linux   : hostname -I
//
// Surtout pas "localhost" : pour la carte, "localhost" désignerait la carte
// elle-même. Attention, cette adresse change a chaque fois qu'on passe d'un
// reseau a un autre.
#define SERVEUR_HOTE  "A_REMPLIR"
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
