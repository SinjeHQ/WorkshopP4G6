import { useState, useEffect } from "react";
import api from "./api";
import EtatSas from "./components/EtatSas";
import StatutTerre from "./components/StatutTerre";
import Historique from "./components/Historique";
import Reseau from "./components/Reseau";

function App() {
  // Les trois fiches reçues de l'API.
  const [evenements, setEvenements] = useState([]);
  const [statut, setStatut] = useState(null);
  const [appareils, setAppareils] = useState([]);
  // true quand l'API ne répond pas.
  const [erreur, setErreur] = useState(false);
  // L'heure du dernier chargement réussi.
  const [miseAJour, setMiseAJour] = useState(null);

  useEffect(() => {
    // Va chercher les trois fiches sur l'API et les range dans les états.
    async function charger() {
      try {
        const reponseEvenements = await api.get("/evenements");
        const reponseStatut = await api.get("/statut");
        const reponseAppareils = await api.get("/appareils");
        setEvenements(reponseEvenements.data);
        setStatut(reponseStatut.data);
        setAppareils(reponseAppareils.data);
        setErreur(false);
        setMiseAJour(new Date());
      } catch (err) {
        console.log(err);
        setErreur(true);
      }
    }

    // Un premier chargement tout de suite, puis un toutes les 3 secondes.
    charger();
    const minuteur = setInterval(charger, 3000);

    // Nettoyage : arrête le minuteur quand le composant disparaît.
    return () => clearInterval(minuteur);
  }, []); // [] : ce bloc ne s'exécute qu'une fois, après le premier affichage.

  // Du plus récent au plus ancien. [...evenements] fait une copie du tableau :
  // sans elle, sort modifierait le tableau rangé chez React.
  const evenementsRecents = [...evenements].sort((a, b) => b.id - a.id);
  // Le plus récent décide de l'état du sas.
  const dernier = evenementsRecents[0];

  return (
    <div className="console">
      <header className="entete">
        <div>
          <h1>Sentinel</h1>
          <p className="sous-titre">Console du sas principal</p>
        </div>
        {miseAJour && (
          <p className="mise-a-jour">
            Mis à jour à {miseAJour.toLocaleTimeString("fr-FR")}
          </p>
        )}
      </header>

      {erreur && (
        <p className="alerte-api">
          L'API ne répond pas. Vérifie qu'elle est lancée avec npm run api, et
          que son adresse dans src/api.js est la bonne.
        </p>
      )}

      <main className="grille">
        <EtatSas dernier={dernier} />
        <StatutTerre statut={statut} />
        <Historique evenements={evenementsRecents} />
        <Reseau appareils={appareils} />
      </main>
    </div>
  );
}

export default App;
