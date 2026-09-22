// Affiche l'état du lien avec la Terre et la file d'attente.
function StatutTerre({ statut }) {
  // Retour anticipé : tant que la fiche n'est pas arrivée.
  if (!statut) {
    return (
      <section className="panneau terre">
        <h2>Lien avec la Terre</h2>
        <p className="vide">En attente des données.</p>
      </section>
    );
  }

  return (
    <section className="panneau terre">
      <h2>Lien avec la Terre</h2>
      <p
        className={
          statut.terre === "connecte"
            ? "terre-etat terre-connecte"
            : "terre-etat terre-coupe"
        }
      >
        {statut.terre === "connecte" ? "Connecté" : "Coupé : mode autonome"}
      </p>
      {statut.enAttente > 0 && (
        <p className="terre-attente">
          {statut.enAttente} événement(s) en attente d'envoi vers la Terre
        </p>
      )}
    </section>
  );
}

export default StatutTerre;
