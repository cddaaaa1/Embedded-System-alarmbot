from flask import Flask, request, jsonify
import json
import os

app = Flask(__name__)

REMINDER_FILE = "reminders.json"

# Initialize the JSON file if it doesn't exist
if not os.path.exists(REMINDER_FILE):
    with open(REMINDER_FILE, "w") as f:
        json.dump([], f)

def load_reminders():
    with open(REMINDER_FILE, "r") as f:
        return json.load(f)

def save_reminders(reminders):
    with open(REMINDER_FILE, "w") as f:
        json.dump(reminders, f, indent=4)

@app.route("/add_reminder", methods=["POST"])
def add_reminder():
    data = request.json
    reminders = load_reminders()
    reminders.append({"task": data["task"], "status": "pending"})
    save_reminders(reminders)
    return jsonify({"message": "Reminder added successfully", "reminders": reminders})

@app.route("/get_reminders", methods=["GET"])
def get_reminders():
    reminders = load_reminders()
    return jsonify(reminders)

@app.route("/update_reminder", methods=["POST"])
def update_reminder():
    data = request.json
    reminders = load_reminders()
    for reminder in reminders:
        if reminder["task"] == data["task"]:
            reminder["status"] = "completed"
    save_reminders(reminders)
    return jsonify({"message": "Reminder updated", "reminders": reminders})

@app.route("/delete_reminder", methods=["POST"])
def delete_reminder():
    data = request.json
    reminders = load_reminders()
    reminders = [r for r in reminders if r["task"] != data["task"]]
    save_reminders(reminders)
    return jsonify({"message": "Reminder deleted", "reminders": reminders})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
