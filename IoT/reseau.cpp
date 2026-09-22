#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>

#include "reseau.h"

// ---------------------------------------------------------------------------
// La file d'attente
//
// Un simple tableau utilisé en rond : fileDebut désigne le plus ancien
// événement non envoyé, fileTaille combien il y en a. Quand le tableau est
// plein, le plus ancien est effacé pour laisser la place au plus récent.
// ---------------------------------------------------------------------------
static Evenement file[TAILLE_FILE];
static int fileDebut = 0;
static int fileTaille = 0;

static WiFiClient clientTcp;

// ---------------------------------------------------------------------------
// L'horloge
//
// L'ESP8266 n'a pas de pile : au démarrage, il ignore la date et l'heure.
// Plutôt que d'aller la chercher sur Internet (impossible quand le vaisseau
// est coupé de la Terre), on la lit dans l'en-tête "Date" que le serveur de
// bord met dans chacune de ses réponses HTTP.
//
// Ensuite, l'heure d'un événement se recalcule à partir de son millis() :
// heure = heure de référence + (millis de l'événement - millis de référence).
// ---------------------------------------------------------------------------
static bool horlogeConnue = false;
static time_t horlogeReference = 0;
static unsigned long horlogeMillis = 0;

static unsigned long dernierEssai = 0;
static unsigned long dernierBattement = 0;
static int dernierEnAttentePublie = -1;

// Diagnostic Wi-Fi : joue une seule fois, quand la connexion tarde trop.
static bool diagnosticFait = false;
static bool scanLance = false;

static const char* MOIS[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// --- Les petites fonctions internes, déclarées ici pour l'ordre de lecture ---
static void empiler(const char* type, const char* niveau, const char* verrou,
                    const char* badge, const char* message);
static bool peutEnvoyer();
static void diagnostiquerWifi();
static void viderFile();
static bool envoyerEvenement(const Evenement& evenement);
static void publierEtatDuSas();
static void synchroniserHorloge(const String& enTeteDate);
static void demanderHeureAuServeur();
static bool heureIso(unsigned long instant, char* sortie, size_t taille);
static String urlDe(const char* route);
static void copier(char* destination, size_t taille, const char* source);

// ---------------------------------------------------------------------------
// Ce que le reste du programme utilise
// ---------------------------------------------------------------------------

void reseauDemarrer() {
  if (WIFI_SSID[0] == '\0') {
    Serial.println("Wi-Fi non configure : copier secrets.example.h en secrets.h.");
    Serial.println("Le sas fonctionne quand meme, en mode autonome.");
    return;
  }

  if (SERVEUR_HOTE[0] == '\0') {
    Serial.println("SERVEUR_HOTE vide dans secrets.h : le Wi-Fi va se connecter,");
    Serial.println("mais rien ne sera envoye au dashboard.");
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connexion au reseau du bord : ");
  Serial.println(WIFI_SSID);
  // On n'attend pas ici : la connexion se termine en arriere-plan. Le sas doit
  // etre operationnel immediatement, reseau ou pas.
}

bool reseauConnecte() {
  return WIFI_SSID[0] != '\0' && WiFi.status() == WL_CONNECTED;
}

int reseauEnAttente() {
  return fileTaille;
}

// Vrai quand on a un serveur a joindre ET une liaison pour le joindre.
static bool peutEnvoyer() {
  return SERVEUR_HOTE[0] != '\0' && reseauConnecte();
}

void journaliser(const char* type, const char* niveau, const char* verrou,
                 const char* badge, const char* message) {
  Serial.print("[");
  Serial.print(niveau);
  Serial.print("] ");
  Serial.println(message);

  empiler(type, niveau, verrou, badge, message);
  viderFile();   // on tente l'envoi tout de suite ; si ca echoue, ce n'est pas grave
}

void reseauBoucle() {
  if (WIFI_SSID[0] == '\0') return;

  unsigned long maintenant = millis();
  if (maintenant - dernierEssai < INTERVALLE_RENVOI_MS) return;
  dernierEssai = maintenant;

  if (!reseauConnecte()) {
    diagnostiquerWifi();
    return;
  }

  viderFile();
}

// ---------------------------------------------------------------------------
// Diagnostic Wi-Fi
//
// Quand la connexion ne vient pas, le moniteur serie reste desesperement muet
// et on ne sait pas quoi chercher. Au bout de DELAI_DIAGNOSTIC_MS, la carte
// liste donc une fois les reseaux qu'elle voit.
//
// Ce que ca repond, surtout : l'ESP8266 ne capte QUE le 2,4 GHz. Si le reseau
// cherche n'apparait pas dans la liste alors que le telephone est juste a cote,
// c'est qu'il diffuse en 5 GHz. Aucun mot de passe n'y changera rien.
// ---------------------------------------------------------------------------
static void diagnostiquerWifi() {
  if (diagnosticFait) return;
  if (millis() < DELAI_DIAGNOSTIC_MS) return;   // on laisse sa chance a la connexion

  if (!scanLance) {
    // true = scan en arriere-plan : la boucle du sas n'est pas bloquee.
    WiFi.scanNetworks(true);
    scanLance = true;
    return;
  }

  int trouves = WiFi.scanComplete();
  if (trouves == WIFI_SCAN_RUNNING) return;     // pas encore fini, on repassera

  diagnosticFait = true;

  Serial.println();
  Serial.println("--- Diagnostic Wi-Fi ---");
  Serial.print("Toujours pas connecte a : ");
  Serial.println(WIFI_SSID);

  if (trouves <= 0) {
    Serial.println("La carte ne voit aucun reseau.");
    Serial.println("Point d'acces eteint, ou trop loin ?");
  } else {
    bool vu = false;
    Serial.println("Reseaux 2,4 GHz visibles par la carte :");
    for (int i = 0; i < trouves; i++) {
      Serial.print("  - ");
      Serial.println(WiFi.SSID(i));
      if (WiFi.SSID(i) == WIFI_SSID) vu = true;
    }

    if (vu) {
      Serial.println("Le reseau est bien visible, donc il est en 2,4 GHz.");
      Serial.println("C'est le mot de passe qu'il faut verifier (secrets.h).");
    } else {
      Serial.println("Le reseau cherche n'est PAS dans cette liste.");
      Serial.println("La carte ne capte que le 2,4 GHz : le point d'acces est");
      Serial.println("tres probablement en 5 GHz. Le basculer en 2,4 GHz.");
      Serial.println("Sur Samsung : Point d'acces mobile > Configurer >");
      Serial.println("activer \"Compatibilite etendue\".");
    }
  }

  WiFi.scanDelete();   // on rend la memoire prise par le scan
  Serial.println("------------------------");
  Serial.println();
}

// ---------------------------------------------------------------------------
// La file d'attente
// ---------------------------------------------------------------------------

static void empiler(const char* type, const char* niveau, const char* verrou,
                    const char* badge, const char* message) {
  int position;

  if (fileTaille == TAILLE_FILE) {
    // File pleine : le plus ancien est sacrifie.
    position = fileDebut;
    fileDebut = (fileDebut + 1) % TAILLE_FILE;
    Serial.println("File pleine : le plus ancien evenement est efface.");
  } else {
    position = (fileDebut + fileTaille) % TAILLE_FILE;
    fileTaille++;
  }

  Evenement& e = file[position];
  copier(e.type, sizeof(e.type), type);
  copier(e.niveau, sizeof(e.niveau), niveau);
  copier(e.verrou, sizeof(e.verrou), verrou);
  copier(e.badge, sizeof(e.badge), badge);
  copier(e.message, sizeof(e.message), message);
  e.instant = millis();
}

// Envoie les evenements en attente, du plus ancien au plus recent.
// Des qu'un envoi echoue, on s'arrete : l'ordre est ainsi toujours respecte.
static void viderFile() {
  if (!peutEnvoyer()) return;

  if (!horlogeConnue) demanderHeureAuServeur();

  while (fileTaille > 0) {
    if (!envoyerEvenement(file[fileDebut])) break;
    fileDebut = (fileDebut + 1) % TAILLE_FILE;
    fileTaille--;
  }

  publierEtatDuSas();
}

// ---------------------------------------------------------------------------
// Les echanges avec le serveur de bord
// ---------------------------------------------------------------------------

static String urlDe(const char* route) {
  return String("http://") + SERVEUR_HOTE + ":" + String(SERVEUR_PORT) + route;
}

// POST /evenements : ajoute une ligne a l'historique du dashboard.
static bool envoyerEvenement(const Evenement& evenement) {
  HTTPClient http;
  http.setTimeout(DELAI_HTTP_MS);

  if (!http.begin(clientTcp, urlDe(ROUTE_EVENEMENTS))) return false;

  http.addHeader("Content-Type", "application/json");
  const char* entetesSuivis[] = {"Date"};
  http.collectHeaders(entetesSuivis, 1);

  // L'heure : entre guillemets si on la connait, sinon null.
  char heure[32];
  char heureJson[40];
  if (heureIso(evenement.instant, heure, sizeof(heure))) {
    snprintf(heureJson, sizeof(heureJson), "\"%s\"", heure);
  } else {
    strcpy(heureJson, "null");
  }

  // Le badge : entre guillemets s'il y en a un, sinon null.
  char badgeJson[16];
  if (evenement.badge[0] != '\0') {
    snprintf(badgeJson, sizeof(badgeJson), "\"%s\"", evenement.badge);
  } else {
    strcpy(badgeJson, "null");
  }

  char corps[384];
  snprintf(corps, sizeof(corps),
           "{\"heure\":%s,\"type\":\"%s\",\"niveau\":\"%s\",\"verrou\":\"%s\","
           "\"badge\":%s,\"sas\":\"%s\",\"message\":\"%s\"}",
           heureJson, evenement.type, evenement.niveau, evenement.verrou,
           badgeJson, NOM_DU_SAS, evenement.message);

  int code = http.POST((uint8_t*)corps, strlen(corps));
  if (code > 0) synchroniserHorloge(http.header("Date"));
  http.end();

  if (code == HTTP_CODE_OK || code == HTTP_CODE_CREATED) return true;

  Serial.print("Envoi impossible (code ");
  Serial.print(code);
  Serial.println("), l'evenement reste en attente.");
  return false;
}

// PATCH /statut : le battement de coeur du sas.
//
// Deux champs seulement, et ils appartiennent au sas :
//   sasVuA       : l'heure du dernier signe de vie, qui permet au dashboard de
//                  dire "sas en ligne" ou "sas injoignable" ;
//   sasEnAttente : combien d'evenements dorment encore dans la file.
//
// On ne touche ni a "terre" ni a "enAttente" : ces deux-la decrivent la liaison
// avec la Terre et c'est la passerelle du vaisseau qui les pilote. PATCH ne
// modifie que les champs envoyes, le reste de la fiche reste intact.
static void publierEtatDuSas() {
  if (!peutEnvoyer()) return;

  unsigned long maintenant = millis();
  bool fileChangee = (fileTaille != dernierEnAttentePublie);
  bool battementDu = (maintenant - dernierBattement >= INTERVALLE_BATTEMENT_MS);
  if (!fileChangee && !battementDu) return;   // rien de nouveau a dire

  HTTPClient http;
  http.setTimeout(DELAI_HTTP_MS);
  if (!http.begin(clientTcp, urlDe(ROUTE_STATUT))) return;

  http.addHeader("Content-Type", "application/json");

  char heure[32];
  char heureJson[40];
  if (heureIso(maintenant, heure, sizeof(heure))) {
    snprintf(heureJson, sizeof(heureJson), "\"%s\"", heure);
  } else {
    strcpy(heureJson, "null");
  }

  char corps[80];
  snprintf(corps, sizeof(corps), "{\"sasVuA\":%s,\"sasEnAttente\":%d}",
           heureJson, fileTaille);

  int code = http.sendRequest("PATCH", (uint8_t*)corps, strlen(corps));
  http.end();

  if (code == HTTP_CODE_OK) {
    dernierEnAttentePublie = fileTaille;
    dernierBattement = maintenant;
  }
}

// GET /statut, uniquement pour lire l'heure dans la reponse du serveur.
static void demanderHeureAuServeur() {
  HTTPClient http;
  http.setTimeout(DELAI_HTTP_MS);
  if (!http.begin(clientTcp, urlDe(ROUTE_STATUT))) return;

  const char* entetesSuivis[] = {"Date"};
  http.collectHeaders(entetesSuivis, 1);

  int code = http.GET();
  if (code > 0) synchroniserHorloge(http.header("Date"));
  http.end();
}

// ---------------------------------------------------------------------------
// L'heure
// ---------------------------------------------------------------------------

// Nombre de jours entre le 1er janvier 1970 et la date donnee.
// Formule classique du calendrier gregorien, valable pour toutes les dates.
static long joursDepuis1970(int annee, int mois, int jour) {
  annee -= (mois <= 2);
  long ere = (annee >= 0 ? annee : annee - 399) / 400;
  long anneeDansEre = annee - ere * 400;                       // 0 a 399
  long jourDansAnnee = (153 * (mois + (mois > 2 ? -3 : 9)) + 2) / 5 + jour - 1;
  long jourDansEre = anneeDansEre * 365 + anneeDansEre / 4 - anneeDansEre / 100 + jourDansAnnee;
  return ere * 146097L + jourDansEre - 719468L;
}

// Lit un en-tete HTTP "Date" du type : Wed, 21 Oct 2015 07:28:00 GMT
static void synchroniserHorloge(const String& enTeteDate) {
  if (enTeteDate.length() < 20) return;

  int jour, annee, heures, minutes, secondes;
  char mois[4];

  if (sscanf(enTeteDate.c_str(), "%*3s, %d %3s %d %d:%d:%d",
             &jour, mois, &annee, &heures, &minutes, &secondes) != 6) {
    return;
  }

  int numeroMois = 0;
  for (int i = 0; i < 12; i++) {
    if (strncmp(mois, MOIS[i], 3) == 0) numeroMois = i + 1;
  }
  if (numeroMois == 0) return;

  horlogeReference = (time_t)(joursDepuis1970(annee, numeroMois, jour) * 86400L
                              + heures * 3600L + minutes * 60L + secondes);
  horlogeMillis = millis();

  if (!horlogeConnue) {
    horlogeConnue = true;
    Serial.println("Horloge reglee sur celle du serveur de bord.");
  }
}

// Ecrit l'heure d'un evenement au format attendu par le dashboard
// (2026-09-22T14:05:12Z, en temps universel : le navigateur l'affiche ensuite
// dans le fuseau de l'utilisateur). Renvoie false tant que l'heure est inconnue.
static bool heureIso(unsigned long instant, char* sortie, size_t taille) {
  if (!horlogeConnue) return false;

  // La soustraction de deux millis() reste juste meme quand le compteur
  // repasse a zero (tous les 49 jours environ).
  long ecart = (long)(instant - horlogeMillis) / 1000L;
  time_t date = horlogeReference + ecart;

  struct tm* t = gmtime(&date);
  if (t == NULL) return false;

  snprintf(sortie, taille, "%04d-%02d-%02dT%02d:%02d:%02dZ",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
  return true;
}

// ---------------------------------------------------------------------------
// Outil
// ---------------------------------------------------------------------------

// Copie une chaine sans jamais deborder, et termine toujours par un zero.
//
// Au passage, on neutralise les deux caracteres qui casseraient le JSON envoye
// au serveur : le guillemet double et l'antislash. Sans cette precaution, il
// suffirait de renommer une equipe en L'equipe "Securite" dans IoT.ino pour que
// plus aucun evenement n'arrive sur le dashboard, sans le moindre message
// d'erreur. Les accents, eux, passent sans probleme : ils voyagent en UTF-8.
static void copier(char* destination, size_t taille, const char* source) {
  if (source == NULL) {
    destination[0] = '\0';
    return;
  }

  size_t i = 0;
  for (; source[i] != '\0' && i < taille - 1; i++) {
    char c = source[i];
    if (c == '"' || c == '\\') {
      destination[i] = '\'';
    } else if (c >= 0 && c < 32) {   // retours a la ligne et autres caracteres de controle
      destination[i] = ' ';
    } else {
      destination[i] = c;
    }
  }
  destination[i] = '\0';
}
