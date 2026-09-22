#ifndef SECRETS_H
#define SECRETS_H

// ---------------------------------------------------------------------------
// MODÈLE — ne pas remplir ce fichier.
//
// Copier ce fichier sous le nom secrets.h, dans le même dossier, et remplir
// la copie :
//
//     Windows   copy secrets.example.h secrets.h
//     Mac/Linux cp secrets.example.h secrets.h
//
// secrets.h est ignoré par Git : les identifiants du réseau restent sur ta
// machine et ne partent jamais sur GitHub.
// ---------------------------------------------------------------------------

// --- Le Wi-Fi que la carte doit rejoindre ---
// Attention : l'ESP8266 ne capte que le 2,4 GHz, jamais le 5 GHz.
#define WIFI_SSID     "NomDuReseau"
#define WIFI_PASSWORD "MotDePasse"

// --- L'adresse de la machine qui fait tourner "npm run api" ---
//
// Se connecter d'abord au même réseau que la carte, puis relever l'adresse :
//   Windows : ipconfig        -> ligne "Adresse IPv4"
//   Mac     : ipconfig getifaddr en0
//   Linux   : hostname -I
//
// Surtout pas "localhost" : pour la carte, "localhost" désignerait la carte
// elle-même. Cette adresse change à chaque fois qu'on passe d'un réseau à un
// autre, donc à revérifier le jour de la démonstration.
#define SERVEUR_HOTE  "192.168.43.57"

#endif
