#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// =====================================================
// WIFI / MQTT
// =====================================================

const char* WIFI_SSID = "S25 de Benjamin";
const char* WIFI_PASSWORD = "passwords";

// METTRE ICI L'IP ACTUELLE DE LA VM DEBIAN
const char* MQTT_SERVER = "10.240.140.182";
const int MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long dernierEssaiWifi = 0;
unsigned long dernierEssaiMQTT = 0;


// =====================================================
// RFID
// =====================================================

#define SS_PIN  D8
#define RST_PIN D3

MFRC522 rfid(SS_PIN, RST_PIN);


// =====================================================
// SERVO / BUZZER / CAPTEUR / LED
// =====================================================

const int PIN_SERVO     = D2;
const int PIN_BUZZER    = D1;
const int PIN_MAGNET    = A0;
const int PIN_LED_ROUGE = D4;
const int PIN_LED_VERTE = D0;

const int SEUIL_MAGNET = 512;


// =====================================================
// SERVO
// =====================================================

const int ANGLE_FERME  = 0;
const int ANGLE_OUVERT = 80;

Servo monServo;


// =====================================================
// TEMPORISATIONS
// =====================================================

// Si personne n'ouvre après un badge valide,
// le sas se reverrouille automatiquement.
const unsigned long DELAI_OUVERTURE_MS = 10000;

// Filtrage du capteur magnétique
const unsigned long STABILITE_MS = 150;


// =====================================================
// EQUIPES
// =====================================================

struct Equipe {
  const char* nom;
  byte uid[4];
  bool accesAutorise;
};

Equipe equipes[4] = {
  {"Equipe Securite",    {0x90, 0xE5, 0x2E, 0xA4}, true},
  {"Equipe Medicale",    {0x6B, 0xB0, 0xF1, 0xAE}, true},
  {"Equipe Energetique", {0x5B, 0xDB, 0x75, 0xBD}, false},
  {"Equipe Alimentaire", {0x2E, 0x45, 0xC9, 0x49}, false}
};


// =====================================================
// ETAT DU SAS
// =====================================================

enum EtatAcces {
  REPOS,
  DEVERROUILLE,
  PASSAGE_EN_COURS
};

EtatAcces etatAcces = REPOS;

unsigned long debutAcces = 0;

bool alarmeActive = false;
bool alarmeDejaArretee = false;


// =====================================================
// ETAT DE PORTE POUR MQTT
// =====================================================

bool premierEtatPorte = true;
bool ancienEtatPorteFermee = true;


// =====================================================
// MQTT
// =====================================================

void publierMQTT(const char* topic, const char* message) {

  // Le fonctionnement local du sas continue même si MQTT tombe
  if (!mqttClient.connected()) {
    return;
  }

  mqttClient.publish(topic, message);

  Serial.print("[MQTT] ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(message);
}


// Scan de badge : "autorise:Equipe Securite", "refuse:Inconnu"...
// Le listener separe le resultat et le nom de l'equipe pour Grafana.
void publierBadge(
  const char* resultat,
  const char* nomEquipe
) {

  String message =
    String(resultat) + ":" + nomEquipe;

  publierMQTT(
    "sentinel/sas/A01/nfc",
    message.c_str()
  );
}


// =====================================================
// WIFI
// =====================================================

void maintenirWifi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  // Nouvelle tentative toutes les 5 secondes
  if (millis() - dernierEssaiWifi < 5000) {
    return;
  }

  dernierEssaiWifi = millis();

  Serial.println("[WIFI] Tentative de reconnexion...");

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );
}


// =====================================================
// CONNEXION MQTT
// =====================================================

void maintenirMQTT() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (mqttClient.connected()) {
    mqttClient.loop();
    return;
  }

  // Nouvelle tentative toutes les 5 secondes
  if (millis() - dernierEssaiMQTT < 5000) {
    return;
  }

  dernierEssaiMQTT = millis();

  Serial.print("[MQTT] Connexion vers ");
  Serial.print(MQTT_SERVER);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  // Last Will :
  // si l'ESP disparaît brutalement, Mosquitto publie "offline"
  bool connexionOK = mqttClient.connect(
    "ESP8266-SAS-A01",

    "sentinel/sas/A01/status",
    0,
    true,
    "offline"
  );

  if (connexionOK) {

    Serial.println("[MQTT] Connecte a Mosquitto");

    // Etat online retenu par Mosquitto
    mqttClient.publish(
      "sentinel/sas/A01/status",
      "online",
      true
    );

    // Envoie immédiatement l'état réel de la porte
    bool porteFermee = lirePorteFermee();

    if (porteFermee) {

      mqttClient.publish(
        "sentinel/sas/A01/porte",
        "fermee"
      );

    } else {

      mqttClient.publish(
        "sentinel/sas/A01/porte",
        "ouverte"
      );
    }

  } else {

    Serial.print("[MQTT] Echec connexion. Code : ");
    Serial.println(mqttClient.state());
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("===============================");
  Serial.println("       SENTINEL - SAS A01");
  Serial.println("===============================");


  // RFID
  SPI.begin();
  rfid.PCD_Init();


  // Servo
  monServo.attach(
    PIN_SERVO,
    500,
    2450
  );


  // Sorties
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_ROUGE, OUTPUT);
  pinMode(PIN_LED_VERTE, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ROUGE, LOW);
  digitalWrite(PIN_LED_VERTE, LOW);

  monServo.write(ANGLE_FERME);


  // ===================================================
  // WIFI
  // ===================================================

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  // ===================================================
  // MQTT
  // ===================================================

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );

  mqttClient.setKeepAlive(30);


  Serial.println("[LOCAL] Systeme du SAS operationnel");
  Serial.println("[WIFI] Connexion en cours...");
  Serial.println("Approchez un badge...");
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // RESEAU
  // ===================================================

  maintenirWifi();
  maintenirMQTT();


  // Affichage IP ESP une seule fois après connexion
  static bool wifiAnnonce = false;

  if (
    WiFi.status() == WL_CONNECTED &&
    !wifiAnnonce
  ) {

    wifiAnnonce = true;

    Serial.println("[WIFI] Connecte");

    Serial.print("[WIFI] IP de l'ESP : ");
    Serial.println(WiFi.localIP());
  }

  if (WiFi.status() != WL_CONNECTED) {
    wifiAnnonce = false;
  }


  // ===================================================
  // CAPTEUR MAGNETIQUE
  // ===================================================

  bool porteFermee = lirePorteFermee();
  bool porteOuverte = !porteFermee;


  // ===================================================
  // ENVOI ETAT PORTE MQTT
  // ===================================================

  if (premierEtatPorte) {

    ancienEtatPorteFermee = porteFermee;
    premierEtatPorte = false;

  }

  else if (
    porteFermee != ancienEtatPorteFermee
  ) {

    ancienEtatPorteFermee = porteFermee;

    if (porteFermee) {

      Serial.println("[PORTE] Fermee");

      publierMQTT(
        "sentinel/sas/A01/porte",
        "fermee"
      );

    } else {

      Serial.println("[PORTE] Ouverte");

      publierMQTT(
        "sentinel/sas/A01/porte",
        "ouverte"
      );
    }
  }


  // ===================================================
  // RFID
  // ===================================================

  if (
    rfid.PICC_IsNewCardPresent() &&
    rfid.PICC_ReadCardSerial()
  ) {

    int index = trouverEquipe();


    // Badge connu
    if (index != -1) {

      Serial.print("[RFID] Badge reconnu : ");
      Serial.println(equipes[index].nom);


      // =================================================
      // BADGE AUTORISE
      // =================================================

      if (equipes[index].accesAutorise) {

        publierBadge(
          "autorise",
          equipes[index].nom
        );

        if (alarmeActive) {

          arreterAlarme(
            equipes[index].nom
          );

        } else {

          accesAutorise(
            equipes[index].nom
          );
        }
      }


      // =================================================
      // BADGE REFUSE
      // =================================================

      else {

        Serial.println(
          "[RFID] Acces refuse"
        );

        publierBadge(
          "refuse",
          equipes[index].nom
        );

        accesRefuse();
      }
    }


    // ===================================================
    // BADGE INCONNU
    // ===================================================

    else {

      Serial.println(
        "[RFID] Badge inconnu"
      );

      publierBadge(
        "refuse",
        "Inconnu"
      );

      accesRefuse();
    }


    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }


  // ===================================================
  // GESTION PASSAGE
  // ===================================================

  gererAcces(porteFermee);


  // ===================================================
  // RESET ALARME
  // ===================================================

  if (porteFermee) {
    alarmeDejaArretee = false;
  }


  // ===================================================
  // DETECTION INTRUSION
  // ===================================================

  if (
    porteOuverte &&
    etatAcces == REPOS &&
    !alarmeActive &&
    !alarmeDejaArretee
  ) {

    declencherAlarme();
  }


  // ===================================================
  // ALARME ACTIVE
  // ===================================================

  if (alarmeActive) {

    digitalWrite(
      PIN_BUZZER,
      HIGH
    );

    digitalWrite(
      PIN_LED_ROUGE,
      HIGH
    );

    delay(80);

    digitalWrite(
      PIN_BUZZER,
      LOW
    );

    digitalWrite(
      PIN_LED_ROUGE,
      LOW
    );

    delay(80);
  }
}


// =====================================================
// CAPTEUR MAGNETIQUE FILTRE
// =====================================================

bool lirePorteFermee() {

  static bool etatStable = true;
  static bool dernierBrut = true;
  static unsigned long vuA = 0;

  bool brut = (
    analogRead(PIN_MAGNET) <
    SEUIL_MAGNET
  );


  if (brut != dernierBrut) {

    dernierBrut = brut;

    vuA = millis();

  }

  else if (
    brut != etatStable &&
    millis() - vuA >= STABILITE_MS
  ) {

    etatStable = brut;
  }

  return etatStable;
}


// =====================================================
// GESTION PASSAGE AUTORISE
// =====================================================

void gererAcces(bool porteFermee) {

  if (etatAcces == REPOS) {
    return;
  }


  // ===================================================
  // PORTE DEVERROUILLEE
  // ===================================================

  if (etatAcces == DEVERROUILLE) {

    // La personne vient d'ouvrir
    if (!porteFermee) {

      Serial.println(
        "[ACCES] Porte ouverte, passage en cours..."
      );

      etatAcces = PASSAGE_EN_COURS;

      return;
    }


    // Personne n'a ouvert après 10 secondes
    if (
      millis() - debutAcces >=
      DELAI_OUVERTURE_MS
    ) {

      verrouiller(
        "personne n'est passe"
      );
    }

    return;
  }


  // ===================================================
  // PASSAGE EN COURS
  // ===================================================

  if (
    etatAcces == PASSAGE_EN_COURS &&
    porteFermee
  ) {

    verrouiller(
      "porte refermee"
    );
  }
}


// =====================================================
// VERROUILLAGE
// =====================================================

void verrouiller(
  const char* raison
) {

  monServo.write(
    ANGLE_FERME
  );

  digitalWrite(
    PIN_LED_VERTE,
    LOW
  );

  etatAcces = REPOS;

  Serial.print(
    "[ACCES] Sas verrouille : "
  );

  Serial.println(
    raison
  );
}


// =====================================================
// RECHERCHE EQUIPE
// =====================================================

int trouverEquipe() {

  if (rfid.uid.size != 4) {
    return -1;
  }

  for (int e = 0; e < 4; e++) {

    bool correspond = true;

    for (
      byte i = 0;
      i < 4;
      i++
    ) {

      if (
        rfid.uid.uidByte[i] !=
        equipes[e].uid[i]
      ) {

        correspond = false;

        break;
      }
    }

    if (correspond) {
      return e;
    }
  }

  return -1;
}


// =====================================================
// ACCES AUTORISE
// =====================================================

void accesAutorise(
  const char* nomEquipe
) {

  Serial.print(
    "[ACCES] Autorise pour : "
  );

  Serial.println(
    nomEquipe
  );


  // MQTT
  // Utile pour identifier l'équipe dans Grafana
  publierMQTT(
    "sentinel/sas/A01/equipe",
    nomEquipe
  );


  digitalWrite(
    PIN_LED_VERTE,
    HIGH
  );

  bipCourt();

  monServo.write(
    ANGLE_OUVERT
  );


  // IMPORTANT :
  // aucun delay(3000) ici.
  // Le programme continue à gérer MQTT et le capteur.
  etatAcces = DEVERROUILLE;

  debutAcces = millis();
}


// =====================================================
// ACCES REFUSE
// =====================================================

void accesRefuse() {

  digitalWrite(
    PIN_LED_ROUGE,
    HIGH
  );

  alarmerefuse();

  digitalWrite(
    PIN_LED_ROUGE,
    LOW
  );
}


// =====================================================
// DECLENCHEMENT ALARME
// =====================================================

void declencherAlarme() {

  Serial.println(
    "[ALARME] Porte forcee sans badge !"
  );

  alarmeActive = true;


  // MQTT -> listener -> PostgreSQL -> Grafana + Telegram
  publierMQTT(
    "sentinel/sas/A01/alarme",
    "intrusion"
  );
}


// =====================================================
// ARRET ALARME
// =====================================================

void arreterAlarme(
  const char* nomEquipe
) {

  Serial.print(
    "[ALARME] Arretee par : "
  );

  Serial.println(
    nomEquipe
  );

  alarmeActive = false;

  alarmeDejaArretee = true;

  digitalWrite(
    PIN_BUZZER,
    LOW
  );

  digitalWrite(
    PIN_LED_ROUGE,
    LOW
  );


  // Information supplémentaire pour la supervision
  publierMQTT(
    "sentinel/sas/A01/alarme_etat",
    "inactive"
  );

  publierMQTT(
    "sentinel/sas/A01/equipe",
    nomEquipe
  );
}


// =====================================================
// BIP COURT
// =====================================================

void bipCourt() {

  digitalWrite(
    PIN_BUZZER,
    HIGH
  );

  delay(150);

  digitalWrite(
    PIN_BUZZER,
    LOW
  );
}


// =====================================================
// ALARME ACCES REFUSE
// =====================================================

void alarmerefuse() {

  for (int i = 0; i < 2; i++) {

    digitalWrite(
      PIN_BUZZER,
      HIGH
    );

    delay(100);

    digitalWrite(
      PIN_BUZZER,
      LOW
    );

    delay(100);
  }
}
