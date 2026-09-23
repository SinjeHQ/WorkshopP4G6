#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// --- Broches RFID ---
#define SS_PIN     D8
#define RST_PIN    D3

// --- Broches Servo / Buzzer / Capteur magnétique / LED ---
const int PIN_SERVO     = D2;
const int PIN_BUZZER    = D1;
const int PIN_MAGNET    = A0;
const int PIN_LED_ROUGE = D4;
const int PIN_LED_VERTE = D0;

const int SEUIL_MAGNET = 512; // a ajuster selon ton test

// --- Positions du servo ---
const int ANGLE_FERME  = 0;
const int ANGLE_OUVERT = 80;

// Si personne n'ouvre la porte apres un badge valide, le sas se reverrouille
// tout seul au bout de ce delai. Sans ca, un badge oublie laisserait le sas
// ouvert indefiniment.
const unsigned long DELAI_OUVERTURE_MS = 10000;

// Le capteur magnetique hesite quand la porte bouge : on n'accepte un
// changement d'etat que s'il se maintient pendant ce temps.
const unsigned long STABILITE_MS = 150;

MFRC522 rfid(SS_PIN, RST_PIN);
Servo monServo;

// --- Structure pour définir une équipe ---
struct Equipe {
  const char* nom;
  byte uid[4];
  bool accesAutorise;
};

// --- Liste des 4 équipes (REMPLACE les UID par ceux de tes vrais badges) ---
Equipe equipes[4] = {
  {"Equipe Securite",     {0x90, 0xE5, 0x2E, 0xA4}, true},   // acces OK
  {"Equipe Medicale",     {0x6B, 0xB0, 0xF1, 0xAE}, true},   // acces OK - a remplacer
  {"Equipe Energetique",  {0x5B, 0xDB, 0x75, 0xBD}, false},  // acces refuse - a remplacer
  {"Equipe Alimentaire",  {0x2E, 0x45, 0xC9, 0x49}, false}   // acces refuse - a remplacer
};

// --- Ou en est le sas ---
//
// REPOS            : porte verrouillee, rien en cours.
// DEVERROUILLE     : badge accepte, le servo a libere la porte. On attend
//                    que quelqu'un l'ouvre vraiment.
// PASSAGE_EN_COURS : la porte est ouverte. On attend qu'elle se referme
//                    pour reverrouiller.
enum EtatAcces { REPOS, DEVERROUILLE, PASSAGE_EN_COURS };

EtatAcces etatAcces = REPOS;
unsigned long debutAcces = 0;

bool alarmeActive      = false;
bool alarmeDejaArretee = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  monServo.attach(PIN_SERVO, 500, 2450);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_ROUGE, OUTPUT);
  pinMode(PIN_LED_VERTE, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ROUGE, LOW);
  digitalWrite(PIN_LED_VERTE, LOW);
  monServo.write(ANGLE_FERME);

  Serial.println("Systeme pret. Approchez un badge...");
}

void loop() {
  bool porteFermee  = lirePorteFermee();
  bool porteOuverte = !porteFermee;
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {

    int index = trouverEquipe();

    if (index != -1) {
      Serial.print("Badge reconnu : ");
      Serial.println(equipes[index].nom);

      if (equipes[index].accesAutorise) {
        if (alarmeActive) {
          arreterAlarme(equipes[index].nom);
        } else {
          accesAutorise(equipes[index].nom);
        }
      } else {
        Serial.println("Acces refuse pour cette equipe.");
        accesRefuse();
      }
    } else {
      Serial.println("Badge inconnu.");
      accesRefuse();
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // Fait avancer le passage en cours : ouverture, puis refermeture.
  gererAcces(porteFermee);

  if (porteFermee) {
    alarmeDejaArretee = false;
  }

  // Porte ouverte alors qu'aucun passage n'est autorise : intrusion.
  if (porteOuverte && etatAcces == REPOS && !alarmeActive && !alarmeDejaArretee) {
    declencherAlarme();
  }

  if (alarmeActive) {
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LED_ROUGE, HIGH);
    delay(80);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_ROUGE, LOW);
    delay(80);
  }
}

// --- Lit le capteur magnetique, en ignorant les hesitations ---
//
// Quand la porte bouge, la valeur passe plusieurs fois de part et d'autre du
// seuil. Sans ce filtre, un rebond d'une milliseconde pendant l'ouverture
// ferait croire que la porte s'est deja refermee, et le servo se verrouillerait
// alors que la porte est encore ouverte.
bool lirePorteFermee() {
  static bool etatStable    = true;   // au demarrage, on suppose la porte fermee
  static bool dernierBrut   = true;
  static unsigned long vuA  = 0;

  bool brut = (analogRead(PIN_MAGNET) < SEUIL_MAGNET);

  if (brut != dernierBrut) {
    // La valeur vient de changer : on demarre le chronometre.
    dernierBrut = brut;
    vuA = millis();
  }
  else if (brut != etatStable && millis() - vuA >= STABILITE_MS) {
    // Elle tient depuis assez longtemps : on accepte le changement.
    etatStable = brut;
  }

  return etatStable;
}

// --- Le deroulement d'un passage autorise ---
void gererAcces(bool porteFermee) {
  if (etatAcces == REPOS) return;

  if (etatAcces == DEVERROUILLE) {
    if (!porteFermee) {
      // La porte vient de s'ouvrir : on attend maintenant sa refermeture.
      Serial.println("Porte ouverte, passage en cours...");
      etatAcces = PASSAGE_EN_COURS;
      return;
    }

    // Personne n'a ouvert : on reverrouille pour ne pas laisser le sas libre.
    if (millis() - debutAcces >= DELAI_OUVERTURE_MS) {
      verrouiller("personne n'est passe");
    }
    return;
  }

  // PASSAGE_EN_COURS : c'est la refermeture de la porte qui verrouille.
  if (porteFermee) {
    verrouiller("porte refermee");
  }
}

void verrouiller(const char* raison) {
  monServo.write(ANGLE_FERME);
  digitalWrite(PIN_LED_VERTE, LOW);
  etatAcces = REPOS;

  Serial.print("Sas verrouille : ");
  Serial.println(raison);
}

// --- Cherche si l'UID scanne correspond a une equipe connue ---
int trouverEquipe() {
  if (rfid.uid.size != 4) return -1;

  for (int e = 0; e < 4; e++) {
    bool correspond = true;

    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != equipes[e].uid[i]) {
        correspond = false;
        break;
      }
    }

    if (correspond) return e;
  }

  return -1;
}

// Libere la porte, sans attendre : c'est loop() qui suivra la suite.
void accesAutorise(const char* nomEquipe) {
  Serial.print("Acces autorise pour : ");
  Serial.println(nomEquipe);

  digitalWrite(PIN_LED_VERTE, HIGH);
  bipCourt();
  monServo.write(ANGLE_OUVERT);

  etatAcces  = DEVERROUILLE;
  debutAcces = millis();
}

void accesRefuse() {
  digitalWrite(PIN_LED_ROUGE, HIGH);
  alarmerefuse();
  digitalWrite(PIN_LED_ROUGE, LOW);
}

void declencherAlarme() {
  Serial.println("ALERTE - porte forcee sans badge ! Badge autorise requis pour arreter.");
  alarmeActive = true;
}

void arreterAlarme(const char* nomEquipe) {
  Serial.print("Alarme arretee par : ");
  Serial.println(nomEquipe);
  alarmeActive = false;
  alarmeDejaArretee = true;
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ROUGE, LOW);
}

void bipCourt() {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(150);
  digitalWrite(PIN_BUZZER, LOW);
}

void alarmerefuse() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    delay(100);
  }
}