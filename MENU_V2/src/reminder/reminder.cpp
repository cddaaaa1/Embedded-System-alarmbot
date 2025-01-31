#include "REMINDER/reminder.h"

void connectWiFi()
{
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
}

// Function to save todos to `todo.json`
void saveTodosToFile(DynamicJsonDocument &doc)
{
    if (!SPIFFS.begin(true))
    { // Ensure SPIFFS is mounted
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    File file = SPIFFS.open(TODO_FILE, "w");
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
    Serial.println("Saved todos to SPIFFS!");
}

// Function to load todos from `todo.json`
void loadTodosFromFile(DynamicJsonDocument &doc)
{
    if (!SPIFFS.begin(true))
    { // Ensure SPIFFS is mounted
        Serial.println("SPIFFS Mount Failed!");
        doc["todos"] = JsonArray();
        return;
    }

    File file = SPIFFS.open(TODO_FILE, "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading, creating new one...");
        doc["todos"] = JsonArray();
        saveTodosToFile(doc);
        return;
    }

    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.println("Failed to parse JSON, resetting file...");
        doc["todos"] = JsonArray();
        saveTodosToFile(doc);
    }

    file.close();
}

// Function to mark a reminder as completed
void completeReminder(String task)
{
    DynamicJsonDocument doc(1024);
    loadTodosFromFile(doc);

    JsonArray todos = doc["todos"].as<JsonArray>();
    for (JsonVariant todo : todos)
    {
        if (todo["text"] == task)
        {
            todo["done"] = true;
            break;
        }
    }

    saveTodosToFile(doc);
}

// Function to get reminders from the server and store them in `todo.json`
void getReminders()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(String(SERVER_URL) + "/get_reminders");
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            String response = http.getString();
            DynamicJsonDocument serverDoc(1024);
            deserializeJson(serverDoc, response);

            DynamicJsonDocument localDoc(1024);
            loadTodosFromFile(localDoc);
            JsonArray localTodos = localDoc["todos"].to<JsonArray>();

            for (JsonVariant reminder : serverDoc["todos"].as<JsonArray>())
            {
                String task = reminder["text"];
                bool done = reminder["done"];

                // Check if task already exists in local JSON
                bool exists = false;
                for (JsonVariant todo : localTodos)
                {
                    if (todo["text"] == task)
                    {
                        exists = true;
                        break;
                    }
                }

                if (!exists)
                {
                    JsonObject newTodo = localTodos.createNestedObject();
                    newTodo["text"] = task;
                    newTodo["done"] = done;
                }
            }

            saveTodosToFile(localDoc);
        }
        else
        {
            Serial.println("Error fetching reminders. HTTP Response Code: " + String(httpResponseCode));
        }
        http.end();
    }
}

// Function to read user input from serial monitor and mark a task as complete
void readSerialInput()
{
    if (Serial.available() > 0)
    {
        String input = Serial.readStringUntil('\n'); // Read input until newline
        input.trim();                                // Remove any trailing newline or spaces

        if (input.startsWith("done "))
        {
            String task = input.substring(5); // Extract task name after "done "
            if (task.length() > 0)
            {
                Serial.println("User input received: Marking task as complete -> " + task);
                completeReminder(task);
            }
            else
            {
                Serial.println("Error: No task name provided.");
            }
        }
        else
        {
            Serial.println("Invalid input format. Use: done TASK_NAME");
        }
    }
}

void syncCompletedTodos()
{
    if (!SPIFFS.begin(true))
    { // Ensure SPIFFS is mounted
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    // // Generate timestamp from current time
    // long currentTimestamp = (currentHour * 3600) + (currentMinute * 60) + currentSecond;

    // Load local `todo.json`
    DynamicJsonDocument localDoc(1024);
    loadTodosFromFile(localDoc);
    JsonArray localTodos = localDoc["todos"].as<JsonArray>();

    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        Serial.println("Fetching server todos for sync...");

        // ✅ 1. Fetch current todos from server
        http.begin(String(SERVER_URL) + "/get_reminders");
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            String serverResponse = http.getString();
            Serial.println("Server response received.");
            DynamicJsonDocument serverDoc(1024);
            deserializeJson(serverDoc, serverResponse);
            JsonArray serverTodos = serverDoc["todos"].as<JsonArray>();

            // ✅ 2. Compare ESP32 and server todo lists
            for (JsonVariant serverTodo : serverTodos)
            {
                String serverTask = serverTodo["text"];
                bool serverDone = serverTodo["done"];
                long serverLastUpdated = serverTodo["last_updated"];

                bool foundInESP32 = false;

                for (JsonVariant localTodo : localTodos)
                {
                    String localTask = localTodo["text"];
                    bool localDone = localTodo["done"];
                    long localLastUpdated = localTodo["last_updated"];

                    // ✅ 3. If task matches, compare `last_updated` timestamps
                    if (serverTask.equalsIgnoreCase(localTask))
                    {
                        foundInESP32 = true;

                        if (localLastUpdated < serverLastUpdated)
                        {
                            // ✅ Server was updated more recently, update ESP32
                            localTodo["done"] = serverDone;
                            localTodo["last_updated"] = serverLastUpdated;
                            Serial.printf("Updated ESP32: %s -> %s\n",
                                          localTask.c_str(),
                                          serverDone ? "Completed" : "Pending");
                        }
                        else if (localLastUpdated > serverLastUpdated)
                        {
                            // ✅ ESP32 was updated more recently, update server
                            HTTPClient updateHttp;
                            updateHttp.begin(String(SERVER_URL) + "/sync_reminder");
                            updateHttp.addHeader("Content-Type", "application/json");

                            DynamicJsonDocument updateDoc(200);
                            updateDoc["text"] = localTask;
                            updateDoc["done"] = localDone;
                            updateDoc["last_updated"] = localLastUpdated;
                            String updatePayload;
                            serializeJson(updateDoc, updatePayload);

                            int updateResponse = updateHttp.POST(updatePayload);
                            if (updateResponse == 200)
                            {
                                Serial.printf("Updated Server: %s -> %s\n",
                                              localTask.c_str(),
                                              localDone ? "Completed" : "Pending");
                            }
                            else
                            {
                                Serial.printf("Failed to update server for %s. HTTP Code: %d\n",
                                              localTask.c_str(), updateResponse);
                            }

                            updateHttp.end();
                        }

                        break;
                    }
                }

                // ✅ 4. If a task exists on the server but NOT on ESP32, add it
                if (!foundInESP32)
                {
                    JsonObject newLocalTodo = localTodos.createNestedObject();
                    newLocalTodo["text"] = serverTask;
                    newLocalTodo["done"] = serverDone;
                    newLocalTodo["last_updated"] = serverLastUpdated;

                    Serial.printf("New task added to ESP32: %s -> %s\n",
                                  serverTask.c_str(),
                                  serverDone ? "Completed" : "Pending");
                }
            }

            // ✅ 5. Save the updated ESP32 todo list
            saveTodosToFile(localDoc);
        }
        else
        {
            Serial.printf("Failed to fetch todos from server. HTTP Code: %d\n", httpResponseCode);
        }

        http.end();
    }
    else
    {
        Serial.println("WiFi not connected, skipping sync.");
    }
}


void toggleTodoStateByName(String taskName)
{
    if (!SPIFFS.begin(true))
    { // Ensure SPIFFS is mounted
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    // Load `todo.json` from SPIFFS
    DynamicJsonDocument doc(1024);
    loadTodosFromFile(doc);
    JsonArray todos = doc["todos"].as<JsonArray>();

    bool taskFound = false;

    // // Generate timestamp from ESP32 time
    // long currentTimestamp = (currentHour * 3600) + (currentMinute * 60) + currentSecond;

    for (JsonVariant todo : todos)
    {
        String currentTask = todo["text"].as<String>();
        if (currentTask.equalsIgnoreCase(taskName))
        {
            bool currentState = todo["done"];
            todo["done"] = !currentState;            // Toggle state
            todo["last_updated"] = currentTimestamp; // Update timestamp
            taskFound = true;

            Serial.printf("Toggled task: %s -> %s\n",
                          currentTask.c_str(),
                          todo["done"] ? "Completed" : "Pending");
            break;
        }
    }

    if (!taskFound)
    {
        Serial.printf("Task not found: %s\n", taskName.c_str());
        return;
    }

    // Save the updated JSON back to SPIFFS
    saveTodosToFile(doc);
}
