import { formatHeure, secondesDepuis, ilYA } from "../heure";

// Au-delà de ce délai sans signe de vie, on considère le sas injoignable.
// La carte fait signe toutes les 30 secondes : on lui en laisse passer trois
// avant de s'inquiéter, pour ne pas crier au loup sur un simple retard.
const SILENCE_MAX_S = 95;

// Décrit la liaison avec le sas à partir de l'heure de son dernier signe de vie.
function liaisonDuSas(sasVuA) {
  const silence = secondesDepuis(sasVuA);

  if (silence === null) {
    return {
      classe: "liaison-inconnue",
      titre: "En attente du sas",
      detail: "Aucun signe de vie reçu depuis le démarrage de l'API.",
    };
  }

  if (silence > SILENCE_MAX_S) {
    return {
      classe: "liaison-perdue",
      titre: "Sas injoignable",
      detail: `Dernier signe de vie à ${formatHeure(sasVuA)}, ${ilYA(silence)}.`,
    };
  }

  return {
    classe: "liaison-active",
    titre: "Sas en ligne",
    detail: `Dernier signe de vie ${ilYA(silence)}.`,
  };
}

// Affiche les deux liaisons de la console :
// celle avec la Terre, et celle avec le sas lui-même.
function StatutTerre({ statut }) {
  // Retour anticipé : tant que la fiche n'est pas arrivée.
  if (!statut) {
    return (
      <section className="panneau terre">
        <h2>Liaisons</h2>
        <p className="vide">En attente des données.</p>
      </section>
    );
  }

  const sas = liaisonDuSas(statut.sasVuA);

  return (
    <section className="panneau terre">
      <h2>Liaisons</h2>

      <div className="zone">
        <h3>Avec la Terre</h3>
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
      </div>

      <div className="zone">
        <h3>Avec le sas</h3>
        <p className={"liaison " + sas.classe}>{sas.titre}</p>
        <p className="terre-attente">{sas.detail}</p>
        {statut.sasEnAttente > 0 && (
          <p className="terre-attente">
            {statut.sasEnAttente} événement(s) encore dans la mémoire du sas
          </p>
        )}
      </div>
    </section>
  );
}

export default StatutTerre;
