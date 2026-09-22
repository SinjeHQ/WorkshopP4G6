import axios from "axios";

// L'adresse de l'API est rangée à un seul endroit : ici.
//
// Par défaut : la fausse API json-server, sur ton PC.
// Pour viser la VM Debian 12 sans toucher au code, créer un fichier `.env`
// à la racine du dashboard avec, par exemple :
//
//     VITE_API_URL=http://192.168.1.20:3000
//
// C'est la même API que celle où le sas (l'ESP8266) dépose ses événements :
// le dashboard lit, la carte écrit.
const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || "http://localhost:3000",
});

export default api;
