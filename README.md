# 🌡️ IoT Dashboard — ESP32 + DHT22 + Flask + SocketIO

Tableau de bord web temps réel pour visualiser les données d'un capteur
température/humidité simulé sur Wokwi (ESP32 + DHT22).

---

## Structure du projet

```
iot-dashboard/
├── server.py            ← Serveur Flask + SocketIO (backend)
├── requirements.txt     ← Dépendances Python
├── templates/
│   └── index.html       ← Interface web (frontend)
├── esp32_dht22.ino      ← Code Arduino pour l'ESP32 (Wokwi)
├── diagram.json         ← Schéma de câblage Wokwi
└── wokwi.toml           ← Config projet Wokwi
```

---

## Lancement rapide

### 1. Installer les dépendances Python

```bash
pip install -r requirements.txt
```

### 2. Démarrer le serveur Flask

```bash
python server.py
```

Le serveur écoute sur **http://localhost:5000**

### 3. Accéder au dashboard

Ouvrir dans un navigateur : **http://localhost:5000**

---

## Configuration de l'ESP32 (Wokwi)

### Branchement DHT22

| DHT22 | ESP32          |
|-------|----------------|
| VCC   | 3.3V           |
| GND   | GND            |
| DATA  | GPIO 15 (D15)  |

### Paramétrer l'URL du serveur

Dans `esp32_dht22.ino`, modifier la ligne :

```cpp
const char* SERVER_URL = "http://YOUR_IP:5000/upload";
```

Remplacer `YOUR_IP` par l'adresse IP de votre machine.

**Trouver votre IP locale :**
- Windows : `ipconfig` dans le terminal
- Linux/Mac : `ip addr` ou `ifconfig`

---

## Utilisation avec ngrok (si Wokwi ne peut pas atteindre votre IP locale)

```bash
# Installer ngrok : https://ngrok.com/download
ngrok http 5000
```

Copier l'URL HTTPS générée (ex: `https://abc123.ngrok-free.app`) et
l'utiliser dans `esp32_dht22.ino` :

```cpp
const char* SERVER_URL = "https://abc123.ngrok-free.app/upload";
```

---

## API du serveur

| Méthode | Route      | Description                        |
|---------|------------|------------------------------------|
| GET     | `/`        | Interface web du dashboard         |
| POST    | `/upload`  | Reçoit les données de l'ESP32      |
| GET     | `/history` | Retourne l'historique JSON         |

### Format JSON attendu sur `/upload`

```json
{
  "temperature": 24.5,
  "humidity": 60.0
}
```

---

## Tester sans ESP32

Vous pouvez envoyer des données de test avec `curl` :

```bash
curl -X POST http://localhost:5000/upload \
     -H "Content-Type: application/json" \
     -d '{"temperature": 23.4, "humidity": 58.2}'
```

Ou avec Python :

```python
import requests, random

for i in range(10):
    requests.post("http://localhost:5000/upload", json={
        "temperature": round(20 + random.uniform(0, 15), 1),
        "humidity":    round(40 + random.uniform(0, 40), 1)
    })
```

---

## Fonctionnalités du dashboard

- Affichage temps réel de la température (°C) et humidité (%)
- Indicateur d'état (Normal / Tiède / Chaud / Humide / Sec)
- Graphiques linéaires avec historique des 50 dernières mesures
- Journal des mesures en tableau (ordre chronologique inversé)
- Indicateur de connexion WebSocket (point vert animé)
- Design sombre avec grille de fond, responsive mobile
"# Projet_IOT"  
