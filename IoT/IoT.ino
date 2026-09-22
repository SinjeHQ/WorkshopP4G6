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
  {"Equipe Medicale",     {0x6B, 0xB0, 0xF1, 0xAE}, true},   // acces OK - a remplacer 6B B0 F1 AE
  {"Equipe Energetique",  {0x5B, 0xDB, 0x75, 0xBD}, false},  // acces refuse - a remplacer 5B DB 75 BD
  {"Equipe Alimentaire",  {0x2E, 0x45, 0xC9, 0x49}, false}   // acces refuse - a remplacer 2E 45 C9 49
};

bool accesEnCours       = false;
bool alarmeActive        = false;
bool alarmeDejaArretee   = false;

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
  monServo.write(0);

  Serial.println("Systeme pret. Approchez un badge...");
}

void loop() {
  int valeurMagnet = analogRead(PIN_MAGNET);
  bool porteFermee  = (valeurMagnet < SEUIL_MAGNET);
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

  if (porteFermee) {
    alarmeDejaArretee = false;
  }

  if (porteOuverte && !accesEnCours && !alarmeActive && !alarmeDejaArretee) {
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

// --- Cherche si l'UID scanne correspond a une equipe connue ---
int trouverEquipe() {
  if (rfid.uid.size != 4) return -1;   // si l'UID lu ne fait pas 4 octets, ce n'est pas un format connu, on abandonne直

  for (int e = 0; e < 4; e++) {          // parcourt les 4 equipes du tableau, une par une (e = 0,1,2,3)
    bool correspond = true;              // suppose que cette equipe correspond, jusqu'a preuve du contraire

    for (byte i = 0; i < 4; i++) {       // parcourt les 4 octets de l'UID (un UID fait 4 octets)
      if (rfid.uid.uidByte[i] != equipes[e].uid[i]) { // compare l'octet i lu avec l'octet i de l'equipe e
        correspond = false;              // des qu'un seul octet differe, cette equipe ne correspond pas
        break;                           // inutile de comparer les octets restants, on sort de cette boucle interne
      }
    }

    if (correspond) return e;            // si les 4 octets ont tous correspondu, on renvoie l'index de l'equipe trouvee
  }

  return -1;                             // si aucune des 4 equipes n'a correspondu, on renvoie -1 (badge inconnu)
}

void accesAutorise(const char* nomEquipe) {
  Serial.print("Acces autorise pour : ");
  Serial.println(nomEquipe);
  accesEnCours = true;
  digitalWrite(PIN_LED_VERTE, HIGH);
  bipCourt();
  monServo.write(80);
  delay(3000);
  monServo.write(0);
  accesEnCours = false;
  digitalWrite(PIN_LED_VERTE, LOW);
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