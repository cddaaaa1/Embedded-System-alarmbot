import requests

SERVER_URL = "http://127.0.0.1:5000"

def add_reminder():
    task = input("Enter a reminder: ")
    response = requests.post(f"{SERVER_URL}/add_reminder", json={"text": task})
    print(response.json())

def get_reminders():
    response = requests.get(f"{SERVER_URL}/get_reminders")
    data = response.json()
    
    print("\n--- Reminders ---")
    for todo in data.get("todos", []):
        status = "✅ Completed" if todo["done"] else "❌ Pending"
        print(f"- {todo['text']} [{status}]")
    print("-----------------\n")

def mark_reminder_complete():
    task = input("Enter the reminder to mark as completed: ")
    response = requests.post(f"{SERVER_URL}/update_reminder", json={"text": task})
    print(response.json())

def delete_reminder():
    task = input("Enter the reminder to delete: ")
    response = requests.post(f"{SERVER_URL}/delete_reminder", json={"text": task})
    print(response.json())

if __name__ == "__main__":
    while True:
        print("\n1. Add Reminder\n2. View Reminders\n3. Mark Reminder as Completed\n4. Delete Reminder\n5. Exit")
        choice = input("Choose an option: ")
        
        if choice == "1":
            add_reminder()
        elif choice == "2":
            get_reminders()
        elif choice == "3":
            mark_reminder_complete()
        elif choice == "4":
            delete_reminder()
        elif choice == "5":
            break
        else:
            print("Invalid choice, please try again.")
