import axios from "axios";

// L'adresse de l'API est rangée à un seul endroit : ici.
// Aujourd'hui : la fausse API json-server, sur ton PC.
// Le jour où la VM Debian 12 est prête : remplacer par son adresse.
const api = axios.create({
  baseURL: "http://localhost:3000",
});

export default api;
