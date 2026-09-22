// Le classeur : les trois zones (VLAN) du réseau de bord, dans l'ordre d'affichage.
const zones = [
  { id: "securite-physique", nom: "Sécurité physique" },
  { id: "commandement", nom: "Commandement" },
  { id: "capteurs", nom: "Capteurs" },
];

// Affiche les appareils rangés par zone, et signale ceux qui sont isolés.
function Reseau({ appareils }) {
  return (
    <section className="panneau reseau">
      <h2>Réseau de bord</h2>
      {zones.map((zone) => (
        <div key={zone.id} className="zone">
          <h3>{zone.nom}</h3>
          <ul className="liste">
            {appareils
              .filter((appareil) => appareil.vlan === zone.id)
              .map((appareil) => (
                <li
                  key={appareil.id}
                  className={
                    appareil.isole ? "appareil appareil-isole" : "appareil"
                  }
                >
                  {appareil.nom}
                  {appareil.isole && <span className="isole">Isolé</span>}
                </li>
              ))}
          </ul>
        </div>
      ))}
    </section>
  );
}

export default Reseau;
