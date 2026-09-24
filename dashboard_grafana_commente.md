# Dashboard Grafana optimisé « Sentinel — Sécurité du SAS » — code commenté

## 1. Vue d'ensemble

Le fichier documenté est [`dashboard_grafana_optimise.json`](./dashboard_grafana_optimise.json), un dashboard Grafana au format JSON utilisant l'API `dashboard.grafana.app/v2`.

Cette documentation s'applique précisément à la version optimisée de 628 lignes. Cette version conserve les mêmes propriétés et les mêmes règles métier que le dashboard d'origine ; sa réduction vient du regroupement des petits objets JSON et de la mise en forme plus régulière du code.

> Le JSON standard n'accepte pas les commentaires. Les extraits ci-dessous utilisent donc `//` et `/* ... */` à des fins pédagogiques : ils ne doivent pas être importés tels quels dans Grafana. Le fichier JSON d'origine reste la version importable.

Tous les panels interrogent :

- la datasource PostgreSQL dont l'UID Grafana est `efz0jrz9r18g0d` ;
- le dataset/base déclaré `sentinel` ;
- la table `events` ;
- uniquement le dispositif `SAS-A01`.

Le schéma de données déduit des requêtes est :

| Colonne | Rôle |
|---|---|
| `device` | Identifiant de l'équipement, ici toujours `SAS-A01` |
| `event_type` | Famille de l'événement : `status`, `alarme`, `alarme_etat`, `porte`, `nfc`, etc. |
| `payload` | Valeur textuelle de l'événement : `offline`, `intrusion`, `ouverte`, `fermee`, `autorise`, `refuse`, etc. |
| `received_at` | Date et heure de réception de l'événement |

Chaîne générale de récupération et d'affichage :

```text
Équipement SAS-A01
  -> écrit des événements dans PostgreSQL / table events
  -> Grafana exécute la requête SQL du panel
  -> PostgreSQL renvoie une valeur, plusieurs champs ou une série temporelle
  -> Grafana applique les mappings, couleurs et options du panel
  -> le résultat apparaît dans le dashboard
```

Le dashboard affiche par défaut les 6 dernières heures :

```jsonc
"timeSettings": {
  "autoRefresh": "5s", // Grafana relance périodiquement les requêtes toutes les 5 secondes.
  "from": "now-6h",    // Début de la période affichée : maintenant moins 6 heures.
  "to": "now",         // Fin de la période : maintenant.
  "timezone": "browser" // Les dates sont affichées dans le fuseau du navigateur.
}
```

Attention : seuls les panels contenant `$__timeFilter(received_at)` suivent la période choisie dans le sélecteur de temps. Les panels utilisant seulement `ORDER BY received_at DESC LIMIT 1` cherchent le dernier événement de toute la table, indépendamment de cette période.

### Résumé de la récupération des données

| Panel | Donnée récupérée | Période Grafana | Affichage |
|---|---|---|---|
| NIVEAU DE SÉCURITÉ | Derniers états `status`, `alarme`, `porte` et dernier refus NFC | Non, sauf test interne de 10 secondes pour le refus | Statut coloré |
| État de la porte | Dernier événement `porte` | Non | Statut coloré |
| Dernier événement reçu | Dernière ligne de `events`, tous types confondus | Non | Libellé coloré |
| Dernier badge utilisé | Dernier événement `nfc` | Non | Autorisé/refusé |
| Badges acceptés | Nombre de badges autorisés | Oui | Compteur vert |
| Badges refusés | Nombre de badges refusés | Oui | Compteur orange |
| Anomalies de sécurité | Deux compteurs d'anomalies | Oui | Deux barres horizontales |
| Histogramme des événements | Comptage par minute et catégorie | Oui | Barres temporelles empilées |

## 2. Structure Grafana commune aux panels

Chaque panel possède cette structure logique :

```jsonc
"panel-XX": {                     // Identifiant interne du panel dans le dashboard.
  "kind": "Panel",               // L'élément est un panel Grafana.
  "spec": {
    "data": {
      "kind": "QueryGroup",      // Groupe des requêtes du panel.
      "spec": {
        "queries": [
          {
            "kind": "PanelQuery",
            "spec": {
              "hidden": false,    // La requête est active.
              "query": {
                "datasource": {
                  "name": "efz0jrz9r18g0d" // UID de la source PostgreSQL.
                },
                "group": "grafana-postgresql-datasource",
                "kind": "DataQuery",
                "spec": {
                  "dataset": "sentinel",
                  "editorMode": "code", // Requête écrite directement en SQL.
                  "format": "table",     // Ou `time_series` pour l'histogramme.
                  "rawQuery": true,
                  "rawSql": "..."        // SQL réellement envoyé à PostgreSQL.
                }
              },
              "refId": "A"        // Nom interne de la requête dans le panel.
            }
          }
        ],
        "transformations": []       // Aucune transformation Grafana après le SQL.
      }
    },
    "vizConfig": {
      "group": "stat",            // Type visuel : stat, bargauge ou timeseries.
      "spec": {
        "fieldConfig": { /* mappings, seuils, unités et couleurs */ },
        "options": { /* mode d'affichage du panel */ }
      }
    }
  }
}
```

Comme `transformations` est vide partout, les calculs métier sont faits directement par PostgreSQL. Grafana se charge surtout de convertir les valeurs numériques en textes/couleurs et de les dessiner.

## 3. Panel `panel-10` — NIVEAU DE SÉCURITÉ

### Requête SQL commentée

```sql
-- Récupère le tout dernier statut connu de l'équipement.
WITH last_status AS (
    SELECT
        lower(COALESCE(payload, '')) AS payload, -- NULL devient '', puis le texte passe en minuscules.
        received_at
    FROM events
    WHERE device = 'SAS-A01'
      AND event_type = 'status'
    ORDER BY received_at DESC -- Le plus récent en premier.
    LIMIT 1                   -- Uniquement le dernier statut.
),

-- Récupère le dernier événement lié à l'état de l'alarme.
last_alarm AS (
    SELECT
        event_type,
        lower(COALESCE(payload, '')) AS payload,
        received_at
    FROM events
    WHERE device = 'SAS-A01'
      AND event_type IN ('alarme', 'alarme_etat')
    ORDER BY received_at DESC
    LIMIT 1
),

-- Récupère le dernier état connu de la porte.
last_door AS (
    SELECT
        lower(COALESCE(payload, '')) AS payload,
        received_at
    FROM events
    WHERE device = 'SAS-A01'
      AND event_type = 'porte'
    ORDER BY received_at DESC
    LIMIT 1
),

-- Récupère le dernier refus de badge, quelle que soit son ancienneté.
last_refused AS (
    SELECT received_at
    FROM events
    WHERE device = 'SAS-A01'
      AND event_type = 'nfc'
      AND lower(COALESCE(payload, '')) IN ('refuse', 'refusé')
    ORDER BY received_at DESC
    LIMIT 1
)

-- Convertit l'état global en un code numérique compris entre 0 et 3.
SELECT CASE
    -- Priorité 1 : aucun statut trouvé ou dernier statut explicitement `offline`.
    WHEN NOT EXISTS (SELECT 1 FROM last_status) THEN 3
    WHEN (SELECT payload FROM last_status) = 'offline' THEN 3

    -- Priorité 2 : le dernier événement d'alarme est une intrusion active.
    WHEN EXISTS (
        SELECT 1
        FROM last_alarm
        WHERE event_type = 'alarme'
          AND payload = 'intrusion'
    ) THEN 2

    -- Priorité 3 : la dernière information de porte dit qu'elle est ouverte.
    WHEN EXISTS (
        SELECT 1
        FROM last_door
        WHERE payload = 'ouverte'
    ) THEN 1

    -- Priorité 4 : un badge a été refusé dans les 10 dernières secondes.
    WHEN EXISTS (
        SELECT 1
        FROM last_refused
        WHERE received_at > now() - interval '10 seconds'
    ) THEN 1

    -- Sinon, l'installation est considérée comme sécurisée.
    ELSE 0
END::double precision AS "État global";
```

### Passage de la donnée à l'écran

La requête renvoie un seul nombre. Grafana applique le mapping suivant :

| Valeur SQL | Texte affiché | Couleur |
|---:|---|---|
| `0` | `SÉCURISÉ` | vert |
| `1` | `ALERTE` | jaune |
| `2` | `INTRUSION` | rouge |
| `3` | `HORS LIGNE` | gris |

Le mode `colorMode: background` colore tout le fond. `lastNotNull` demande à Grafana d'afficher la dernière valeur non nulle reçue.

La priorité est importante : hors ligne l'emporte sur intrusion, qui l'emporte sur porte ouverte, qui l'emporte sur badge refusé récent.

## 4. Panel `panel-24` — État de la porte

```sql
SELECT CASE lower(payload)
    WHEN 'ouverte' THEN 1 -- Dernier payload = ouverte.
    WHEN 'fermee'  THEN 0 -- Dernier payload = fermee, sans accent dans les données.
    ELSE -1               -- Toute autre valeur est considérée inconnue.
END::double precision AS "Porte"
FROM events
WHERE device = 'SAS-A01'
  AND event_type = 'porte'       -- Ignore tous les événements non liés à la porte.
ORDER BY received_at DESC        -- Cherche le plus récent dans toute la table.
LIMIT 1;
```

| Valeur SQL | Texte affiché | Couleur configurée |
|---:|---|---|
| `-1` | `INCONNUE` | gris |
| `0` | `FERMÉE` | rouge |
| `1` | `OUVERTE` | vert |

Ce panel ne dépend pas du sélecteur temporel. S'il n'existe aucune ligne `porte`, PostgreSQL renvoie zéro ligne et Grafana affiche généralement « No data », pas la valeur `-1`.

## 5. Panel `panel-12` — Dernier événement reçu

```sql
SELECT CASE
    WHEN event_type = 'alarme' AND lower(payload) = 'intrusion' THEN 5
    WHEN event_type = 'status' AND lower(payload) = 'offline' THEN 4
    WHEN event_type = 'nfc' AND lower(payload) IN ('refuse', 'refusé') THEN 3
    WHEN event_type = 'nfc' AND lower(payload) IN ('autorise', 'accepte') THEN 2
    WHEN event_type = 'porte' AND lower(payload) = 'ouverte' THEN 1
    WHEN event_type = 'porte' AND lower(payload) = 'fermee' THEN 0
    ELSE 6 -- Tout couple type/payload non reconnu devient « autre événement ».
END::double precision AS "Dernier événement"
FROM events
WHERE device = 'SAS-A01'
ORDER BY received_at DESC -- Dernier événement du dispositif, tous types confondus.
LIMIT 1;
```

| Valeur SQL | Texte affiché | Couleur |
|---:|---|---|
| `0` | `PORTE FERMÉE` | rouge |
| `1` | `PORTE OUVERTE` | vert |
| `2` | `BADGE AUTORISÉ` | vert |
| `3` | `BADGE REFUSÉ` | orange |
| `4` | `SUPERVISION HORS LIGNE` | gris |
| `5` | `INTRUSION` | rouge |
| `6` | `AUTRE ÉVÉNEMENT` | bleu |

La description du panel annonce « Type, valeur et heure du dernier message », mais la requête ne renvoie que le code du type/valeur. Elle ne sélectionne pas `received_at` : l'heure n'est donc pas affichée par ce code.

## 6. Panel `panel-13` — Dernier badge utilisé

```sql
SELECT CASE
    WHEN lower(payload) IN ('autorise', 'accepte') THEN 1
    WHEN lower(payload) IN ('refuse', 'refusé') THEN 0
    ELSE -1
END::double precision AS "Dernier badge"
FROM events
WHERE device = 'SAS-A01'
  AND event_type = 'nfc'  -- Ne conserve que les passages de badge.
ORDER BY received_at DESC
LIMIT 1;                  -- Dernière décision NFC de toute la table.
```

| Valeur SQL | Texte affiché | Couleur |
|---:|---|---|
| `-1` | `INCONNU` | gris |
| `0` | `REFUSÉ` | rouge |
| `1` | `AUTORISÉ` | vert |

Le panel est indépendant de la période Grafana. Comme pour la porte, l'absence totale d'événement NFC produit zéro ligne et non `-1`.

## 7. Panel `panel-15` — Badges acceptés

```sql
SELECT
    COUNT(*)::double precision AS "Badges acceptés" -- Compte les lignes correspondantes.
FROM events
WHERE device = 'SAS-A01'
  AND event_type = 'nfc'
  AND lower(payload) IN ('autorise', 'accepte')
  AND $__timeFilter(received_at); -- Macro Grafana remplacée par les bornes de temps choisies.
```

`$__timeFilter(received_at)` devient approximativement une condition du type :

```sql
received_at BETWEEN 'date_de_début' AND 'date_de_fin'
```

Le résultat est un compteur unique. Le panel `stat` l'affiche sur un fond vert fixe. Il se recalcule au rafraîchissement et change lorsque l'utilisateur modifie la période.

## 8. Panel `panel-16` — Badges refusés

```sql
SELECT
    COUNT(*)::double precision AS "Badges refusés"
FROM events
WHERE device = 'SAS-A01'
  AND event_type = 'nfc'
  AND lower(payload) IN ('refuse', 'refusé')
  AND $__timeFilter(received_at);
```

Le fonctionnement est identique au panel précédent, mais il compte les refus. Le fond est orange fixe.

## 9. Panel `panel-14` — Anomalies de sécurité

```sql
SELECT
  -- Premier compteur : intrusion, type « porte forcée » ou payload « forcée ».
  COUNT(*) FILTER (
    WHERE event_type = 'alarme' AND lower(payload) = 'intrusion'
       OR event_type IN ('porte_forcee', 'porte-forcée')
       OR lower(payload) IN ('forcee', 'forcée')
  )::double precision AS "Porte forcée / intrusion",

  -- Deuxième compteur : refus NFC.
  COUNT(*) FILTER (
    WHERE event_type = 'nfc'
      AND lower(payload) IN ('refuse', 'refusé')
  )::double precision AS "Mauvais badge"
FROM events
WHERE device = 'SAS-A01'
  AND $__timeFilter(received_at); -- Les deux compteurs suivent la période Grafana.
```

PostgreSQL renvoie une seule ligne avec deux colonnes. Le `Bar gauge` les transforme en deux barres horizontales :

- `Porte forcée / intrusion`, forcée en rouge par un override ;
- `Mauvais badge`, forcée en orange.

Il n'existe pas de transformation Grafana : les deux agrégations proviennent directement des deux `COUNT(*) FILTER (...)` SQL.

## 10. Panel `panel-28` — Histogramme des événements

```sql
SELECT
  -- Macro Grafana : arrondit received_at par intervalles d'une minute
  -- et donne à cette colonne l'alias temporel attendu par Grafana.
  $__timeGroupAlias(received_at, '1m'),

  -- Transforme chaque événement en nom de série.
  CASE
    WHEN event_type = 'nfc' AND lower(payload) IN ('autorise', 'accepte')
      THEN 'Badge autorisé'
    WHEN event_type = 'nfc' AND lower(payload) IN ('refuse', 'refusé')
      THEN 'Mauvais badge'
    WHEN event_type = 'porte' AND lower(payload) = 'ouverte'
      THEN 'Porte ouverte'
    WHEN event_type = 'porte' AND lower(payload) = 'fermee'
      THEN 'Porte fermée'
    WHEN event_type = 'alarme' AND lower(payload) = 'intrusion'
      THEN 'Porte forcée / intrusion'
    ELSE 'Autre événement'
  END AS metric,

  -- Nombre d'événements dans chaque couple minute/catégorie.
  COUNT(*)::double precision AS value
FROM events
WHERE device = 'SAS-A01'
  AND event_type IN ('nfc', 'porte', 'alarme', 'porte_forcee', 'porte-forcée')
  AND $__timeFilter(received_at)
GROUP BY 1, 2 -- Groupe par minute puis par libellé `metric`.
ORDER BY 1;   -- Trie les points chronologiquement.
```

La requête est déclarée avec `format: time_series`. Grafana reçoit donc, pour chaque ligne :

1. une date arrondie à la minute ;
2. un nom de série dans `metric` ;
3. un nombre dans `value`.

Le panel dessine des barres empilées (`drawStyle: bars`, `stacking.mode: normal`) et leur affecte ces couleurs :

| Série | Couleur |
|---|---|
| Badge autorisé | vert |
| Mauvais badge | orange |
| Porte ouverte | bleu |
| Porte fermée | violet |
| Porte forcée / intrusion | rouge |
| Autre événement | palette automatique |

La légende affiche aussi la somme de chaque série sur la période (`legend.calcs: ["sum"]`).

## 11. Disposition du dashboard

La grille Grafana fait 24 colonnes. Les coordonnées `x`, `y`, `width` et `height` ne changent pas les données : elles déterminent seulement la position et la taille des panels.

```text
Ligne 1 : [Niveau sécurité 8] [État porte 4] [Dernier événement 6] [Dernier badge 6]
Ligne 2 : [Badges acceptés 6] [Anomalies 12] [Badges refusés 6]
Ligne 3 : [Histogramme 24]
```

## 12. Points d'attention relevés

1. **Pas de gestion de vieillissement du statut.** `NIVEAU DE SÉCURITÉ` considère l'équipement en ligne tant que le dernier payload `status` n'est pas `offline`, même si cet événement date de longtemps. Il n'y a pas de test du type `received_at < now() - interval ...`.

2. **L'heure promise n'est pas affichée.** Le panel `Dernier événement reçu` ne sélectionne pas `received_at`, contrairement à sa description.

3. **Absence de données différente de valeur inconnue.** Dans `État de la porte` et `Dernier badge utilisé`, le `ELSE -1` ne s'applique que si une ligne existe avec un payload inattendu. S'il n'existe aucune ligne, la requête ne renvoie rien.

4. **Classification partielle des portes forcées dans l'histogramme.** Le `WHERE` accepte `porte_forcee` et `porte-forcée`, mais le `CASE` ne les classe pas explicitement comme `Porte forcée / intrusion`. Ces événements tombent dans `Autre événement`, sauf si une autre condition correspond.

5. **Couleurs porte potentiellement contre-intuitives.** Les panels configurent `FERMÉE` en rouge et `OUVERTE` en vert. Pour un dashboard de sécurité, l'inverse pourrait être attendu ; il faut confirmer le sens métier voulu.

6. **Orthographes normalisées seulement en partie.** `lower(...)` neutralise les majuscules/minuscules, mais pas les accents ni les variantes de mots. Par exemple `fermee` est reconnu, tandis que `fermée` ne l'est pas dans les requêtes actuelles.
