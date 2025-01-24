import requests
import cv2
import mediapipe as mp
import numpy as np
import sys
import signal 

RASPBERRY_PI_IP = "http://172.20.10.7:5001"

mp_face_detection = mp.solutions.face_detection
face_detection = mp_face_detection.FaceDetection(min_detection_confidence=0.5)

# FOV
HFOV = 62.2
VFOV = 48.8

def signal_handler(sig, frame):
    print("\nGracefully shutting down...")
    cv2.destroyAllWindows() 
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def request_capture():
    print("\nRequesting Raspberry Pi to capture an image...")

    response = requests.get(f"{RASPBERRY_PI_IP}/capture", stream=True)

    if response.status_code == 200:
        print("Image received from Raspberry Pi.")
        with open("received.jpg", "wb") as file:
            file.write(response.content)

        return "received.jpg"
    else:
        print("Failed to get image from Raspberry Pi.")
        return None

def send_yaw_pitch(yaw, pitch):
    url = f"{RASPBERRY_PI_IP}/update_angles"
    data = {"yaw": yaw, "pitch": pitch}
    response = requests.post(url, json=data)

    if response.status_code == 200:
        print("Yaw/Pitch sent successfully!")
    else:
        print(f"Failed to send Yaw/Pitch: {response.text}")

initial_pitch = None

def process_image(image_path):
    global initial_pitch

    frame = cv2.imread(image_path)
    if frame is None:
        print("Error: Failed to load image.")
        return None

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = face_detection.process(rgb_frame)

    if results.detections:
        for detection in results.detections:
            bboxC = detection.location_data.relative_bounding_box
            x, y, w, h = bboxC.xmin, bboxC.ymin, bboxC.width, bboxC.height

            x, y, w, h = int(x * frame.shape[1]), int(y * frame.shape[0]), int(w * frame.shape[1]), int(h * frame.shape[0])

            CENTER_X, CENTER_Y = frame.shape[1] // 2, frame.shape[0] // 2
            face_center_x, face_center_y = x + w // 2, y + h // 2

            yaw = (face_center_x - CENTER_X) / frame.shape[1] * HFOV
            pitch = (face_center_y - CENTER_Y) / frame.shape[0] * VFOV

            if initial_pitch is None:
                initial_pitch = pitch
                print(f"Auto-calibrated initial pitch: {initial_pitch:.2f}°")
            relative_pitch = pitch - initial_pitch

            print(f"Face Detected! Yaw: {yaw:.2f}°, Adjusted Pitch: {relative_pitch:.2f}°")

            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)

            cv2.putText(frame, f"Yaw: {yaw:.2f}°, Pitch: {relative_pitch:.2f}°", 
                        (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

            cv2.imshow("Yaw/Pitch Visualization", frame)
            cv2.waitKey(5000) 
            cv2.destroyAllWindows()

            return {"yaw": yaw, "pitch": relative_pitch}

    else:
        print("No face detected.")
        return None

if __name__ == '__main__':
    while True:
        cmd = input("\nPress 's' to request capture, 'q' to quit: ").strip().lower()
        if cmd == "s":
            image_path = request_capture()
            if image_path:
                yaw_pitch = process_image(image_path)
                if yaw_pitch:
                    print(f"Final Result: Yaw={yaw_pitch['yaw']:.2f}°, Pitch={yaw_pitch['pitch']:.2f}°")
                    send_yaw_pitch(yaw_pitch["yaw"], yaw_pitch["pitch"])
        elif cmd == "q":
            print("Exiting...")
            break