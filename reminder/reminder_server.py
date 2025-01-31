import eventlet
eventlet.monkey_patch()  # Must be the first import

from flask import Flask, request, jsonify
from flask_socketio import SocketIO
import json
import os

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet")  # Force eventlet for WebSockets

REMINDER_FILE = "reminders.json"

if not os.path.exists(REMINDER_FILE):
    with open(REMINDER_FILE, "w") as f:
        json.dump([], f)

def load_reminders():
    with open(REMINDER_FILE, "r") as f:
        return json.load(f)

def save_reminders(reminders):
    with open(REMINDER_FILE, "w") as f:
        json.dump(reminders, f, indent=4)

@app.route("/")
def home():
    return "ESP32 WebSocket Server Running!"

@app.route("/add_reminder", methods=["POST"])
def add_reminder():
    data = request.json
    reminders = load_reminders()
    new_reminder = {"task": data["task"], "status": "pending"}
    reminders.append(new_reminder)
    save_reminders(reminders)

    # Notify ESP32 via WebSocket
    socketio.emit("new_reminder", new_reminder)
    
    return jsonify({"message": "Reminder added successfully"})

@app.route("/update_reminder", methods=["POST"])
def update_reminder():
    data = request.json
    reminders = load_reminders()
    for reminder in reminders:
        if reminder["task"] == data["task"]:
            reminder["status"] = "completed"
    save_reminders(reminders)
    return jsonify({"message": "Reminder updated"})

@socketio.on("connect")
def handle_connect():
    print("✅ ESP32 connected via WebSocket")

@socketio.on("disconnect")
def handle_disconnect():
    print("❌ ESP32 disconnected")

@socketio.on("message")
def handle_message(message):
    print(f"📩 Received from ESP32: {message}")

if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=8080, debug=True)
