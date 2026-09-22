// Affiche la liste des événements, déjà triés du plus récent au plus ancien.
function Historique({ evenements }) {
  return (
    <section className="panneau historique">
      <h2>Historique</h2>
      {evenements.length === 0 && <p className="vide">Aucun événement.</p>}
      <ul className="liste">
        {evenements.map((evenement) => (
          <li key={evenement.id} className="evenement">
            <span className="evenement-heure">
              {new Date(evenement.heure).toLocaleTimeString("fr-FR")}
            </span>
            <span>{evenement.message}</span>
            <span className={"niveau niveau-" + evenement.niveau}>
              {evenement.niveau}
            </span>
          </li>
        ))}
      </ul>
    </section>
  );
}

export default Historique;
