# Etape 1 : compiler le dashboard avec Node
FROM node:22-alpine AS build

# L'adresse de l'API, inscrite dans le code au moment de la compilation.
# Elle est donnee par le docker-compose.yml.
ARG VITE_API_URL=http://localhost:8000
ENV VITE_API_URL=$VITE_API_URL

WORKDIR /app
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build

# Etape 2 : servir les fichiers compiles avec nginx
FROM nginx:alpine
COPY --from=build /app/dist /usr/share/nginx/html
EXPOSE 80
