#ifndef RESEAU_H
#define RESEAU_H

#include <Arduino.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Le pont entre le sas et le dashboard.
//
// Le principe : le programme du sas ne sait rien du réseau. Quand il se passe
// quelque chose (badge accepté, porte forcée, alarme arrêtée...), il appelle
// journaliser(). Ce fichier se charge du reste : ranger l'événement dans une
// file d'attente, puis l'envoyer à l'API que lit le dashboard.
//
// Si le réseau est coupé, rien ne bloque : l'événement reste dans la file et
// part tout seul dès que la liaison revient. C'est le "mode autonome" du sujet.
// ---------------------------------------------------------------------------

// Un événement qui attend d'être envoyé au serveur de bord.
// Les champs reprennent exactement ceux que le dashboard sait afficher.
struct Evenement {
  char type[20];             // badge_accepte, badge_refuse, intrusion...
  char niveau[10];           // normal, doute, critique
  char verrou[8];            // ferme, ouvert
  char badge[9];             // UID en hexadécimal, ou "" s'il n'y a pas de badge
  char message[96];          // la phrase affichée dans l'historique
  unsigned long instant;     // millis() au moment où l'événement s'est produit
};

// À appeler une fois dans setup().
void reseauDemarrer();

// À appeler à chaque tour de loop() : réessaie d'envoyer ce qui attend.
void reseauBoucle();

// Enregistre un événement et tente de l'envoyer tout de suite.
// badge peut valoir NULL ou "" quand aucun badge n'est en jeu.
void journaliser(const char* type, const char* niveau, const char* verrou,
                 const char* badge, const char* message);

// Vrai quand la carte est associée au Wi-Fi du bord.
bool reseauConnecte();

// Nombre d'événements encore en attente d'envoi.
int reseauEnAttente();

#endif
