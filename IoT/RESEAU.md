# Brancher le sas sur le dashboard

Ce document explique comment la carte ESP8266 envoie ce qu'elle voit au
dashboard, et comment régler le tout le jour de la démonstration.

## L'idée

Le sas et le dashboard ne se parlent jamais directement. Entre les deux il y a
l'**API du serveur de bord** :

```
   ESP8266  ──écrit──►   API du serveur de bord   ◄──lit──  Dashboard
   (le sas)              /evenements  /statut               (React)
```

- la carte **écrit** : à chaque badge, chaque ouverture, chaque alarme ;
- le dashboard **lit** toutes les 3 secondes et affiche.

Personne n'attend personne : si l'un des deux s'arrête, l'autre continue.

## Ce que la carte envoie

### `POST /evenements` — une ligne d'historique

```json
{
  "heure": "2026-09-22T14:05:12Z",
  "type": "badge_accepte",
  "niveau": "normal",
  "verrou": "ouvert",
  "badge": "90E52EA4",
  "sas": "Sas principal",
  "message": "Badge 90E52EA4 accepté — Equipe Securite"
}
```

`niveau` décide de la couleur affichée : `normal` (vert), `doute` (orange),
`critique` (rouge). `verrou` vaut `ferme` ou `ouvert`, et c'est lui qui écrit
« Verrouillé » ou « Déverrouillé » en gros sur la console.

Les types envoyés par la carte :

| Type | Quand | Niveau |
|---|---|---|
| `verrouillage` | au démarrage, et après chaque refermeture | normal |
| `badge_accepte` | badge d'une équipe autorisée, la porte s'ouvre | normal |
| `badge_refuse` | équipe non autorisée, ou badge inconnu | doute |
| `intrusion` | porte ouverte sans badge | critique |
| `alarme_arretee` | un badge autorisé a fait taire l'alarme | normal |

L'`id` n'est pas envoyé : c'est l'API qui le donne.

### `PATCH /statut` — le battement de cœur

Toutes les 30 secondes, même quand il ne se passe rien :

```json
{ "sasVuA": "2026-09-22T14:05:42Z", "sasEnAttente": 0 }
```

C'est ce qui permet au dashboard d'afficher « Sas en ligne » ou « Sas
injoignable ». Au-delà de 95 secondes sans signe de vie (trois battements
manqués), la console passe au rouge.

La carte ne touche **ni à `terre` ni à `enAttente`** : ces deux champs
décrivent la liaison avec la Terre et appartiennent à la passerelle du
vaisseau. `PATCH` ne modifie que les champs envoyés, le reste de la fiche reste
intact.

## Le mode autonome

C'est le cœur du sujet : le sas doit continuer à protéger le vaisseau même
sans réseau.

- La carte **ne bloque jamais** sur le réseau. Le Wi-Fi se connecte en arrière-plan,
  chaque requête abandonne au bout de 1,5 seconde. Badges, porte et alarme
  répondent à la même vitesse, réseau ou pas.
- Quand le serveur ne répond pas, l'événement est **gardé en mémoire** (30 au
  maximum, les plus anciens sont sacrifiés au-delà).
- Dès que la liaison revient, tout part **dans l'ordre**, avec l'heure d'origine :
  rien n'est réécrit à l'heure du retour.
- Si `WIFI_SSID` est laissé vide dans `config.h`, la carte ne cherche même pas à
  se connecter. C'est utile pour tester le sas seul.

## L'heure

L'ESP8266 n'a pas de pile : au démarrage, il ignore la date. Il la lit dans
l'en-tête `Date` que le serveur de bord met dans chacune de ses réponses — donc
sans jamais avoir besoin d'Internet, ce qui compte quand le vaisseau est coupé
de la Terre.

Tant que cette heure est inconnue (premier envoi, serveur jamais joint), le
champ `heure` part à `null` et le dashboard affiche `--:--:--` au lieu d'une
date fausse.

Les heures circulent en temps universel (le `Z` final). C'est le navigateur qui
les remet dans le fuseau de celui qui regarde l'écran.

## Réglage, étape par étape

1. **Trouver l'adresse du serveur.** Sur la machine qui fait tourner l'API :
   `ip a` (Linux) ou `ipconfig` (Windows). Ce sera par exemple `192.168.1.10`.
   Surtout pas `localhost` : pour la carte, `localhost` désignerait la carte
   elle-même.

2. **Remplir `config.h`** avec le Wi-Fi du bord et cette adresse :

   ```c
   #define WIFI_SSID     "VaisseauWifi"
   #define WIFI_PASSWORD "motdepasse"
   #define SERVEUR_HOTE  "192.168.1.10"
   #define SERVEUR_PORT  3000
   ```

3. **Lancer l'API** côté dashboard : `npm run api`. Elle écoute déjà sur toutes
   les interfaces (`--host 0.0.0.0`), sinon la carte ne pourrait pas l'atteindre.

4. **Téléverser** le programme (voir `CONFIG.md`) et ouvrir le moniteur série à
   115200 bauds. On doit y lire :

   ```
   Connexion au reseau du bord : VaisseauWifi
   Systeme pret. Approchez un badge...
   [normal] Sas verrouillé au démarrage
   Horloge reglee sur celle du serveur de bord.
   ```

5. **Vérifier sur le dashboard** : le panneau « Liaisons » doit afficher
   « Sas en ligne », et l'événement de démarrage doit apparaître dans
   l'historique.

## Si ça ne marche pas

| Ce qu'on voit | Ce qu'il faut regarder |
|---|---|
| `Wi-Fi non configure (config.h)` | `WIFI_SSID` est resté vide |
| Rien après `Connexion au reseau du bord` | SSID ou mot de passe faux ; l'ESP8266 ne capte que le Wi-Fi 2,4 GHz, pas le 5 GHz |
| `Envoi impossible (code -1)` | l'API n'est pas lancée, l'adresse de `SERVEUR_HOTE` est fausse, ou le pare-feu du PC bloque le port 3000 |
| `Envoi impossible (code 404)` | l'API n'expose pas `/evenements` |
| Les heures affichent `--:--:--` | la carte n'a encore jamais joint le serveur ; elles se corrigeront au premier échange |
| « Sas injoignable » sur le dashboard | la carte est éteinte, ou elle n'atteint plus le serveur depuis plus de 95 secondes |
