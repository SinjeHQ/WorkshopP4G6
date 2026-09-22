import { formatHeure } from "../heure";

// Affiche l'état du sas d'après le dernier événement reçu.
function EtatSas({ dernier }) {
  // Retour anticipé : tant qu'aucun événement n'est arrivé.
  if (!dernier) {
    return (
      <section className="panneau etat">
        <h2>État du sas</h2>
        <p className="vide">Aucun événement reçu pour l'instant.</p>
      </section>
    );
  }

  return (
    <section className={"panneau etat etat-" + dernier.niveau}>
      <h2>État du sas</h2>
      <p className="etat-verrou">
        {dernier.verrou === "ferme" ? "Verrouillé" : "Déverrouillé"}
      </p>
      <p className={"niveau niveau-" + dernier.niveau}>{dernier.niveau}</p>
      <p className="etat-message">
        {dernier.message}, à {formatHeure(dernier.heure)}
      </p>
    </section>
  );
}

export default EtatSas;
