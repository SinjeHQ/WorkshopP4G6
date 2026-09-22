// Tout ce qui touche aux heures reçues de l'API est rangé ici.
//
// Le sas envoie ses heures en temps universel (2026-09-22T14:05:12Z) : c'est
// le navigateur qui les remet dans le fuseau de celui qui regarde l'écran.
// Tant que la carte n'a pas réglé son horloge sur le serveur de bord, le champ
// vaut null — il faut donc toujours se méfier de ce qu'on reçoit.

// Transforme une heure reçue en "14:05:12". Renvoie "--:--:--" si elle manque
// ou si elle est illisible, plutôt que le vilain "Invalid Date".
export function formatHeure(valeur) {
  if (!valeur) return "--:--:--";

  const date = new Date(valeur);
  if (Number.isNaN(date.getTime())) return "--:--:--";

  return date.toLocaleTimeString("fr-FR");
}

// Depuis combien de secondes cette heure est-elle passée ?
// Renvoie null si l'heure est absente ou illisible.
export function secondesDepuis(valeur) {
  if (!valeur) return null;

  const date = new Date(valeur);
  if (Number.isNaN(date.getTime())) return null;

  return Math.max(0, Math.round((Date.now() - date.getTime()) / 1000));
}

// "il y a 12 s", "il y a 3 min" : pour dire depuis quand le sas n'a plus parlé.
export function ilYA(secondes) {
  if (secondes === null) return "";
  if (secondes < 60) return `il y a ${secondes} s`;

  const minutes = Math.round(secondes / 60);
  if (minutes < 60) return `il y a ${minutes} min`;

  return `il y a ${Math.round(minutes / 60)} h`;
}

// Range les événements du plus récent au plus ancien.
//
// On se fie d'abord à l'heure de l'événement : c'est celle du sas, et elle
// reste juste même pour les événements arrivés en retard après une coupure
// réseau. Quand deux heures sont identiques ou absentes, l'ordre d'arrivée
// (l'id donné par l'API) départage.
export function trierDuPlusRecent(evenements) {
  return [...evenements].sort((a, b) => {
    const heureA = new Date(a.heure).getTime();
    const heureB = new Date(b.heure).getTime();
    const lisibles = !Number.isNaN(heureA) && !Number.isNaN(heureB);

    if (lisibles && heureA !== heureB) return heureB - heureA;
    return Number(b.id) - Number(a.id);
  });
}
