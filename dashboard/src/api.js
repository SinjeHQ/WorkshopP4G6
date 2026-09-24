import axios from "axios";

// L adresse de l API est rangee a un seul endroit : ici.
// Elle est lue dans la variable VITE_API_URL, donnee au moment de la compilation.
// Si cette variable n existe pas, on retombe sur la fausse API json-server du PC.
const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || "http://localhost:3000",
});

export default api;
