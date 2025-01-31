import asyncio
import websockets
import json

connected_clients = set()  # Track connected ESP32 devices
reminders = []  # Store reminders in memory

async def send_reminders():
    """Send all pending reminders to connected ESP32 clients."""
    if reminders and connected_clients:
        message = json.dumps({"reminders": reminders})
        await asyncio.wait([client.send(message) for client in connected_clients])

async def handle_client(websocket, path):
    """Handle incoming ESP32 connections."""
    print(f"ESP32 Connected: {websocket.remote_address}")
    connected_clients.add(websocket)

    try:
        async for message in websocket:
            data = json.loads(message)

            if "done" in data:
                task_done = data["done"]
                reminders[:] = [task for task in reminders if task != task_done]
                print(f"Task '{task_done}' completed. Updated reminders: {reminders}")
                await send_reminders()
    except websockets.exceptions.ConnectionClosedError:
        print(f"ESP32 Disconnected: {websocket.remote_address}")
    finally:
        connected_clients.discard(websocket)  # Properly remove client

async def main():
    """Start the WebSocket server."""
    server = await websockets.serve(handle_client, "192.168.0.27", 8765)
    print("WebSocket server started on ws://<your_pc_ip>:8765")
    await server.wait_closed()

async def add_reminder():
    """Accept user input and send new reminders to ESP32."""
    while True:
        task = input("Enter a reminder: ").strip()
        if task:
            reminders.append(task)
            print(f"Reminder '{task}' added.")
            await send_reminders()  # Send updated list to ESP32

# Run server and user input handling concurrently
async def start():
    await asyncio.gather(main(), add_reminder())

asyncio.run(start())
