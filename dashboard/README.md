# Sentinel — console du sas

Dashboard React du groupe 6, Workshop Horizon 2080.

Il affiche quatre choses, rafraîchies toutes les 3 secondes :

1. l'état du sas, d'après le dernier événement reçu ;
2. le lien avec la Terre : connecté ou mode autonome ;
3. l'historique des événements ;
4. les appareils du réseau de bord, rangés par VLAN, avec ceux qui sont isolés.

## Lancer le dashboard

1. `npm install` — une seule fois, pour installer les paquets.
2. Premier terminal : `npm run api` — la fausse API démarre sur http://localhost:3000
3. Second terminal : `npm run dev` — le dashboard s'ouvre sur http://localhost:5173

Les deux doivent tourner en même temps.

## Tester

Pendant que tout tourne, modifie `db.json` et enregistre : par exemple `"terre": "coupe"` devient `"terre": "connecte"`. Le dashboard change dans les 3 secondes.

## Brancher sur la VM

Dans `src/api.js`, remplacer `http://localhost:3000` par l'adresse de la VM Debian 12.

L'API de la VM doit fournir les trois fiches aux mêmes adresses que la fausse API : `/evenements`, `/statut` et `/appareils`.

## Les fichiers

- `src/api.js` : l'adresse de l'API, à un seul endroit.
- `src/App.jsx` : charge les trois fiches toutes les 3 secondes et les passe aux composants.
- `src/components/EtatSas.jsx` : l'état du sas.
- `src/components/StatutTerre.jsx` : le lien avec la Terre.
- `src/components/Historique.jsx` : la liste des événements.
- `src/components/Reseau.jsx` : les appareils par VLAN.
- `db.json` : les données de la fausse API.
