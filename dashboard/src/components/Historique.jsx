import { formatHeure } from "../heure";

// Affiche la liste des événements, déjà triés du plus récent au plus ancien.
// total : combien il y en a en tout, quand la liste affichée est tronquée.
function Historique({ evenements, total }) {
  return (
    <section className="panneau historique">
      <h2>Historique</h2>
      {evenements.length === 0 && <p className="vide">Aucun événement.</p>}
      <ul className="liste">
        {evenements.map((evenement) => (
          <li key={evenement.id} className="evenement">
            <span className="evenement-heure">
              {formatHeure(evenement.heure)}
            </span>
            <span>{evenement.message}</span>
            <span className={"niveau niveau-" + evenement.niveau}>
              {evenement.niveau}
            </span>
          </li>
        ))}
      </ul>
      {total > evenements.length && (
        <p className="historique-reste">
          {evenements.length} événements affichés sur {total}.
        </p>
      )}
    </section>
  );
}

export default Historique;
