# Sentinel — Sas de sécurité autonome du vaisseau

Projet réalisé pour le **Workshop National EPSI B3 — "Horizon 2080"** (septembre 2026), Pilier 4.

## L'idée en une phrase

On simule le sas d'entrée d'un vaisseau spatial qui doit rester sécurisé même quand il n'a plus aucun contact avec la Terre : un badge ouvre la porte, un capteur surveille qu'elle ne s'ouvre pas sans autorisation, et tout continue de fonctionner même si le réseau est coupé.

## Contexte du sujet

En 2080, l'ESA envoie des vaisseaux autonomes explorer l'espace lointain. Ces vaisseaux ne peuvent plus communiquer en temps réel avec la Terre, donc ils doivent savoir se protéger et fonctionner tout seuls. Notre projet répond à ça avec un système de sécurité physique complet.

## Composition de l'équipe

- **2 développeurs**
- **3 infra/réseau**

## Le projet est découpé en 4 parties

### 1. La partie IoT (l'objet physique)

C'est le cœur du projet : une carte électronique (ESP8266) qui pilote un mini "sas" physique.

- Un **lecteur de badge** identifie qui essaie de passer
- Une **porte motorisée** (petit servo-moteur) s'ouvre ou reste fermée selon le badge
- Un **capteur** surveille si la porte s'ouvre sans autorisation
- Une **alarme sonore et lumineuse** se déclenche en cas de problème

Quatre équipes du vaisseau ont chacune leur badge :

| Équipe | Accès au sas |
|---|---|
| Équipe Sécurité | ✅ Autorisé |
| Équipe Médicale | ✅ Autorisé |
| Équipe Énergétique | ❌ Refusé |
| Équipe Alimentaire | ❌ Refusé |

**Ce qui se passe concrètement :**
- Bon badge d'une équipe autorisée → petit bip, lumière verte, la porte s'ouvre puis se referme toute seule après 3 secondes
- Mauvais badge ou équipe non autorisée → lumière rouge, bip d'alerte
- Quelqu'un force la porte sans badge → alarme continue (son + lumière rouge clignotante) qui ne s'arrête que si un badge autorisé est présenté, peu importe si la porte est encore ouverte ou non

### 2. Le dashboard (l'écran de contrôle)

Une interface web qui permet de suivre en direct ce qui se passe sur le sas : qui est passé, quand, si une alarme a été déclenchée, et si le lien avec la Terre est actif ou coupé.

**Comment il est relié au sas :** la carte n'envoie rien au dashboard directement. À chaque badge, chaque ouverture et chaque alarme, elle dépose une ligne sur l'API du serveur de bord, et le dashboard vient la lire toutes les 3 secondes.

```
   ESP8266  ──écrit──►   API du serveur de bord   ◄──lit──  Dashboard
   (le sas)              /evenements  /statut               (React)
```

Aucun des deux n'attend l'autre. Si le réseau tombe, le sas garde ses événements en mémoire et les renvoie tous, dans l'ordre et avec leur heure d'origine, dès que la liaison revient. Et toutes les 30 secondes la carte fait signe, même quand il ne se passe rien : c'est ce qui permet au dashboard d'afficher « Sas en ligne » ou « Sas injoignable ».

Le réglage tient dans un seul fichier à créer, `IoT/secrets.h` (Wi-Fi du bord et adresse du serveur), copié depuis `IoT/secrets.example.h` et ignoré par Git. La marche à suivre complète est dans [`IoT/RESEAU.md`](IoT/RESEAU.md).

### 3. Le réseau sécurisé (la partie infra)

On simule le réseau interne du vaisseau avec plusieurs machines virtuelles :
- Une machine qui joue le rôle de **passerelle vers la Terre**, qu'on peut couper à volonté pour simuler une panne de communication
- Une machine qui fait tourner le réseau interne du vaisseau (réception des données du badge, surveillance, tableaux de bord)
- Un système qui **repère un comportement suspect** sur le réseau et isole automatiquement l'appareil concerné

L'objectif : montrer que même sans la Terre, le vaisseau continue à se défendre et à fonctionner tout seul.

### 4. La maquette 3D

Une représentation physique du sas (porte, cadre) sur laquelle sont fixés les composants électroniques (badge, capteur, moteur, alarme), pour rendre la démonstration concrète et visuelle devant le jury.

## Matériel utilisé (partie IoT)

| Composant | Rôle |
|---|---|
| ESP8266 (NodeMCU) | Cerveau électronique du système |
| Lecteur RFID RC522 | Lit les badges |
| Micro-servo SG90 | Ouvre/ferme la porte |
| Buzzer | Son d'alerte |
| Capteur magnétique | Détecte si la porte est ouverte ou fermée |
| LED rouge | Voyant d'alerte |
| LED verte | Voyant d'accès autorisé |

## Câblage

| Composant | Branché sur |
|---|---|
| Lecteur de badge | broches 3V, D3, G, D6, D7, D5, D8 |
| Moteur de la porte | broche D2 |
| Buzzer | broche D1 |
| Capteur de porte | broches 3V, G, A0 |
| Voyant rouge | broche D4 |
| Voyant vert | broche D0 |

**Attention alimentation** : pendant les tests, la carte doit être alimentée uniquement par le câble USB de l'ordinateur — jamais avec le bloc d'alimentation externe en même temps, pour ne pas l'endommager.

## Scénario de démonstration

1. On montre le fonctionnement normal : un badge autorisé ouvre la porte sans problème, et la ligne apparaît sur le dashboard en moins de 3 secondes
2. On coupe volontairement le lien avec la Terre → le sas continue de fonctionner tout seul
3. On force la porte sans badge → l'alarme se déclenche immédiatement
4. On présente un badge autorisé → l'alarme s'arrête
5. On rétablit le lien avec la Terre → tout se resynchronise sur le dashboard

Variante qui montre bien le mode autonome : débrancher le serveur de bord (ou couper son Wi-Fi) entre les étapes 3 et 4. Le sas continue de réagir exactement pareil, sans le moindre ralentissement ; au retour du réseau, l'intrusion et l'arrêt de l'alarme arrivent d'un coup sur le dashboard, chacun à l'heure où il s'est réellement produit.

## Pistes d'amélioration pour une future version

- Sécuriser davantage les échanges entre le sas et le serveur de bord : aujourd'hui les événements partent en HTTP simple, sans authentification. Du HTTPS et un jeton partagé empêcheraient un appareil branché sur le réseau d'inventer de faux passages.
- Garder la file d'attente ailleurs qu'en mémoire vive : pour l'instant, une coupure de courant pendant une panne réseau fait perdre les événements non envoyés.
- Gérer plusieurs sas en même temps sur le même vaisseau. Chaque événement porte déjà le nom de son sas (champ `sas`), il resterait à le montrer sur le dashboard.
