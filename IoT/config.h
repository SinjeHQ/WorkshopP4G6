#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// Réglages du sas. C'est le seul fichier à modifier pour brancher la carte
// sur le réseau du vaisseau : le reste du programme n'a pas besoin d'y toucher.
// ---------------------------------------------------------------------------

// --- Wi-Fi et adresse du serveur ---
//
// Ces trois valeurs ne sont pas ici : elles changent d'un poste et d'un réseau
// à l'autre, et un mot de passe n'a rien à faire sur GitHub. Elles vivent dans
// secrets.h, que Git ignore.
//
// Au premier clonage du dépôt : copier secrets.example.h en secrets.h et le
// remplir. Sans ce fichier le programme compile quand même — le sas tourne
// alors sans réseau, et le dit sur le moniteur série.
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #warning "secrets.h absent : copier secrets.example.h en secrets.h (le sas tournera sans reseau)."
  #define WIFI_SSID     ""
  #define WIFI_PASSWORD ""
  #define SERVEUR_HOTE  ""
#endif

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

// Delai avant le diagnostic Wi-Fi, en millisecondes. Passe ce temps sans
// connexion, la carte liste les reseaux 2,4 GHz qu'elle voit sur le moniteur
// serie. C'est ce qui permet de savoir si le point d'acces est en 5 GHz : dans
// ce cas la carte ne le voit tout simplement pas.
#define DELAI_DIAGNOSTIC_MS 15000

// Intervalle du "battement de coeur", en millisecondes : même quand il ne se
// passe rien, le sas fait signe au serveur pour que le dashboard sache qu'il
// est toujours là. Le dashboard le déclare injoignable au bout de trois
// battements manqués.
#define INTERVALLE_BATTEMENT_MS 30000

#endif
