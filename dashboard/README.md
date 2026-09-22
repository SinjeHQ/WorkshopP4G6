# Sentinel — console du sas

Dashboard React du groupe 6, Workshop Horizon 2080.

Il affiche quatre choses, rafraîchies toutes les 3 secondes :

1. l'état du sas, d'après le dernier événement reçu ;
2. les liaisons : avec la Terre (connectée ou coupée) et avec le sas lui-même
   (en ligne ou injoignable) ;
3. l'historique des événements ;
4. les appareils du réseau de bord, rangés par VLAN, avec ceux qui sont isolés.

Le dashboard ne parle jamais directement à la carte : il lit l'API du serveur
de bord, là où l'ESP8266 dépose ce qu'il voit. Le détail du branchement est
dans `../IoT/RESEAU.md`.

## Lancer le dashboard

1. `npm install` — une seule fois, pour installer les paquets.
2. Premier terminal : `npm run api` — la fausse API démarre sur http://localhost:3000
3. Second terminal : `npm run dev` — le dashboard s'ouvre sur http://localhost:5173

Les deux doivent tourner en même temps.

L'API écoute sur toutes les interfaces (`--host 0.0.0.0`) et pas seulement sur
`localhost` : sans ça, la carte ESP8266 ne pourrait pas la joindre depuis le
réseau.

## Tester

**Sans la carte** : modifie `db.json` et enregistre — par exemple `"terre": "coupe"`
devient `"terre": "connecte"`. Le dashboard change dans les 3 secondes.

**Avec la carte** : passe un badge. L'événement arrive dans l'historique en
moins de 3 secondes, et le panneau « Liaisons » affiche « Sas en ligne ».

Les événements de `db.json` sont un décor de départ, datés du 20 septembre.
Pour une démonstration propre, vide la liste avant de commencer :

```json
"evenements": [],
```

## Brancher sur la VM

Créer un fichier `.env` à la racine du dashboard :

```
VITE_API_URL=http://192.168.1.20:3000
```

Plus besoin de toucher au code. Sans ce fichier, le dashboard vise
`http://localhost:3000`.

L'API de la VM doit fournir les trois fiches aux mêmes adresses que la fausse
API : `/evenements`, `/statut` et `/appareils`, et accepter le `POST
/evenements` et le `PATCH /statut` que la carte envoie.

## Les fiches attendues

`/evenements` — une ligne par chose qui s'est passée :

```json
{
  "id": 7,
  "heure": "2026-09-22T14:05:12Z",
  "type": "badge_accepte",
  "niveau": "normal",
  "verrou": "ouvert",
  "badge": "90E52EA4",
  "message": "Badge 90E52EA4 accepté — Equipe Securite"
}
```

`niveau` donne la couleur (`normal`, `doute`, `critique`), `verrou` (`ferme` ou
`ouvert`) donne le grand mot affiché sur la console.

`/statut` — deux liaisons dans une seule fiche :

```json
{
  "terre": "coupe",
  "enAttente": 3,
  "sasVuA": "2026-09-22T14:05:42Z",
  "sasEnAttente": 0
}
```

`terre` et `enAttente` décrivent la liaison avec la Terre : c'est la passerelle
du vaisseau qui les tient. `sasVuA` et `sasEnAttente` sont le battement de cœur
de la carte : c'est elle qui les met à jour, toutes les 30 secondes.

`/appareils` — un appareil du réseau de bord par ligne, avec son `vlan`
(`securite-physique`, `commandement` ou `capteurs`) et son drapeau `isole`.

## Les fichiers

- `src/api.js` : l'adresse de l'API, à un seul endroit.
- `src/heure.js` : lecture des heures reçues et tri de l'historique.
- `src/App.jsx` : charge les trois fiches toutes les 3 secondes et les passe aux composants.
- `src/components/EtatSas.jsx` : l'état du sas.
- `src/components/StatutTerre.jsx` : les deux liaisons, Terre et sas.
- `src/components/Historique.jsx` : la liste des événements.
- `src/components/Reseau.jsx` : les appareils par VLAN.
- `db.json` : les données de la fausse API.
