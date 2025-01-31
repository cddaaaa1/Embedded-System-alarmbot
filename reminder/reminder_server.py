from flask import Flask, request, jsonify
import json
import os

app = Flask(__name__)

REMINDER_FILE = "reminders.json"

# Initialize the JSON file if it doesn't exist
if not os.path.exists(REMINDER_FILE):
    with open(REMINDER_FILE, "w") as f:
        json.dump({"todos": []}, f, indent=4)

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
    
    # Add new task in correct format
    reminders["todos"].append({"text": data["text"], "done": False})
    
    save_reminders(reminders)
    return jsonify({"message": "Reminder added successfully", "todos": reminders["todos"]})

@app.route("/get_reminders", methods=["GET"])
def get_reminders():
    reminders = load_reminders()
    return jsonify(reminders)

@app.route("/update_reminder", methods=["POST"])
def update_reminder():
    data = request.json
    reminders = load_reminders()
    
    # Update the `done` status of the task
    for todo in reminders["todos"]:
        if todo["text"] == data["text"]:
            todo["done"] = True

    save_reminders(reminders)
    return jsonify({"message": "Reminder updated", "todos": reminders["todos"]})

@app.route("/delete_reminder", methods=["POST"])
def delete_reminder():
    data = request.json
    reminders = load_reminders()
    
    # Remove task from list
    reminders["todos"] = [r for r in reminders["todos"] if r["text"] != data["text"]]

    save_reminders(reminders)
    return jsonify({"message": "Reminder deleted", "todos": reminders["todos"]})

@app.route("/sync_reminder", methods=["POST"])
def sync_reminder():
    data = request.json
    reminders = load_reminders()
    
    # Find the task and update its state
    task_found = False
    for todo in reminders["todos"]:
        if todo["text"] == data["text"]:
            todo["done"] = data["done"]
            task_found = True
            break

    # If the task doesn't exist, add it (for ESP32 updates)
    if not task_found:
        reminders["todos"].append({"text": data["text"], "done": data["done"]})

    save_reminders(reminders)
    return jsonify({"message": "Reminder synchronized", "todos": reminders["todos"]})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
