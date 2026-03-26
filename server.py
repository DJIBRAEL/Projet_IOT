from flask import Flask, request, jsonify, render_template
from flask_socketio import SocketIO, emit
from flask_cors import CORS
from datetime import datetime
import random
import time
import threading

# ─────────────────────────────────────────────
# Initialisation de l'application Flask
# ─────────────────────────────────────────────
app = Flask(__name__)
app.config["SECRET_KEY"] = "iot-dashboard-secret-2024"

# Autoriser les connexions cross-origin
CORS(app)

# Utilisation du mode "threading" (stable et compatible Python 3.12/3.13)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# ─────────────────────────────────────────────
# Stockage en mémoire et thread-safety
# ─────────────────────────────────────────────
MAX_HISTORY = 50
data_history = []
data_lock = threading.Lock()

# État de la LED (contrôle à distance)
led_state = False


# ─────────────────────────────────────────────
# Route principale : affiche l'interface web
# ─────────────────────────────────────────────
@app.route("/")
def index():
    """Sert la page HTML du dashboard."""
    return render_template("index.html")


# ─────────────────────────────────────────────
# Route /upload : reçoit les données de l'ESP32
# ─────────────────────────────────────────────
@app.route("/upload", methods=["POST"])
def upload():
    """Endpoint POST pour l'ESP32."""
    if not request.is_json:
        return jsonify({"error": "Content-Type doit être application/json"}), 400

    payload = request.get_json()
    if not payload or "temperature" not in payload or "humidity" not in payload:
        return jsonify({"error": "Champs 'temperature' et 'humidity' requis"}), 400

    # On traite la mesure et on renvoie l'état de la LED
    source_info = request.remote_addr or "esp32"
    response_data, status_code = process_measure(payload["temperature"], payload["humidity"], source=source_info)
    
    # On ajoute l'état de la LED à la réponse pour l'ESP32
    if status_code == 200:
        content = response_data.get_json()
        content["led"] = led_state
        return jsonify(content), 200
    
    return response_data, status_code


def process_measure(temp, hum, source="esp32"):
    """Traite, stocke et diffuse une mesure."""
    try:
        t_val = round(float(temp), 1)
        h_val = round(float(hum), 1)
    except (ValueError, TypeError):
        return jsonify({"error": "Valeurs numériques invalides"}), 400

    mesure = {
        "temperature": t_val,
        "humidity":    h_val,
        "timestamp":   datetime.now().strftime("%H:%M:%S"),
        "date":        datetime.now().strftime("%d/%m/%Y"),
        "source":      source
    }

    with data_lock:
        data_history.append(mesure)
        if len(data_history) > MAX_HISTORY:
            data_history.pop(0)

    # Diffusion via SocketIO
    socketio.emit("new_data", mesure)
    
    return jsonify({"status": "ok", "received": mesure}), 200


# ─────────────────────────────────────────────
# Route /history : renvoie l'historique en JSON
# ─────────────────────────────────────────────
@app.route("/history")
def history_api():
    with data_lock:
        return jsonify(list(data_history))


# ─────────────────────────────────────────────
# Événements SocketIO
# ─────────────────────────────────────────────
@socketio.on("connect")
def on_connect():
    global led_state
    with data_lock:
        hist_copy = list(data_history)
    print(f"[SocketIO] Client connecté — {len(hist_copy)} mesures envoyées")
    emit("history", hist_copy)
    # Envoyer l'état actuel de la LED au nouveau client
    emit("led_update", {"state": led_state})


@socketio.on("disconnect")
def on_disconnect():
    print("[SocketIO] Client déconnecté")


@socketio.on("toggle_led")
def handle_toggle_led():
    global led_state
    led_state = not led_state
    print(f"[SocketIO] LED commandée : {'ON' if led_state else 'OFF'}")
    # Diffuser à tous les clients
    socketio.emit("led_update", {"state": led_state})


# ─────────────────────────────────────────────
# Générateur de données fictives (pour test)
# ─────────────────────────────────────────────
def mock_data_generator():
    """Génère des données aléatoires pour simuler un ESP32."""
    print("[Mock] Générateur de données activé (intervalle: 5s)")
    while True:
        temp = random.uniform(20.0, 24.0)
        hum  = random.uniform(40.0, 60.0)
        
        with app.app_context():
            process_measure(temp, hum, source="mock")
        
        time.sleep(5)


# ─────────────────────────────────────────────
# Point d'entrée
# ─────────────────────────────────────────────
if __name__ == "__main__":
    print("-" * 50)
    print("  🌡️  IoT Dashboard Server")
    print("  URL locale : http://localhost:5000")
    print("  MODE       : Threading + Mock Data")
    print("-" * 50)

    simulator_thread = threading.Thread(target=mock_data_generator, daemon=True)
    simulator_thread.start()

    socketio.run(app, host="0.0.0.0", port=5000, debug=False)
