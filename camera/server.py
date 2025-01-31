import requests
import cv2
import mediapipe as mp
import numpy as np
import sys
import time

# -------------------------------
# 1. 配置部分
# -------------------------------
RASPBERRY_PI_IP = "http://172.20.10.7:5001"
ESP32_IP = "http://172.20.10.8"

HFOV = 62.2  # 水平方向 FOV
VFOV = 48.8  # 垂直方向 FOV

SEARCH_STEP_YAW = 15   # 搜索步长
SEARCH_STEP_PITCH = 5  # 搜索步长
SEARCH_DELAY = 0.5  # 搜索间隔

CENTER_TOLERANCE = 10  # 偏移小于 8° 时认为人脸已居中

current_yaw = 0
current_pitch = 0
search_direction = 1  

mp_face_detection = mp.solutions.face_detection
face_detection = mp_face_detection.FaceDetection(min_detection_confidence=0.5)

# -------------------------------
# 2. 发送指令
# -------------------------------
def request_capture():
    """ 请求树莓派拍照 """
    response = requests.get(f"{RASPBERRY_PI_IP}/capture", stream=True)
    if response.status_code == 200:
        with open("received.jpg", "wb") as file:
            file.write(response.content)
        return "received.jpg"
    return None

def send_servo_angles(yaw_offset, pitch_offset):
    """ 发送角度调整到 ESP32 """
    requests.post(f"{ESP32_IP}/update_angles", json={"yaw": yaw_offset, "pitch": pitch_offset})

# -------------------------------
# 3. 处理图像 & 追踪人脸
# -------------------------------
def process_image(image_path):
    """ 处理图像，调整舵机角度，并显示摄像头画面 """
    global current_yaw, current_pitch, search_direction

    frame = cv2.imread(image_path)
    if frame is None:
        return None

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = face_detection.process(rgb_frame)

    if results.detections:
        # -------- 检测到人脸 --------
        for detection in results.detections:
            bboxC = detection.location_data.relative_bounding_box
            x, y, w, h = int(bboxC.xmin * frame.shape[1]), int(bboxC.ymin * frame.shape[0]), \
                         int(bboxC.width * frame.shape[1]), int(bboxC.height * frame.shape[0])
            face_center_x, face_center_y = x + w // 2, y + h // 2
            CENTER_X, CENTER_Y = frame.shape[1] // 2, frame.shape[0] // 2

            # 计算角度偏移量
            yaw_offset = (face_center_x - CENTER_X) / frame.shape[1] * HFOV * 0.5
            pitch_offset = (face_center_y - CENTER_Y) / frame.shape[0] * VFOV * 0.5

            print(f"✅ Face Detected! Yaw Offset: {yaw_offset:.2f}°, Pitch Offset: {pitch_offset:.2f}°")

            # 绘制检测框和角度信息
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
            cv2.putText(frame, f"Yaw: {yaw_offset:.2f}°, Pitch: {pitch_offset:.2f}°",
                        (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

            # 显示摄像头画面
            cv2.imshow("Face Tracking", frame)
            cv2.waitKey(1)  # 更新窗口，不阻塞程序

            # **如果人脸足够居中，停止搜索**
            if abs(yaw_offset) < CENTER_TOLERANCE and abs(pitch_offset) < CENTER_TOLERANCE:
                print("🎯 Face is centered! Stopping tracking.")
                return {"yaw": 0, "pitch": 0}  # 让舵机不再调整

            # 否则继续调整
            send_servo_angles(yaw_offset, pitch_offset)
            return {"yaw": yaw_offset, "pitch": pitch_offset}  

    else:
        # -------- 未检测到人脸，继续搜索模式 --------
        print("🔍 No face detected. Searching...")

        # 搜索模式可视化
        cv2.putText(frame, "NO FACE DETECTED", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 
                    1, (0, 0, 255), 2)
        cv2.imshow("Face Tracking", frame)
        cv2.waitKey(1)

        # 调整舵机搜索
        current_yaw += search_direction * SEARCH_STEP_YAW
        if abs(current_yaw) > 90:
            search_direction *= -1
            current_pitch += SEARCH_STEP_PITCH  
            if abs(current_pitch) > 45:
                search_direction *= -1  
                current_pitch += SEARCH_STEP_PITCH

        send_servo_angles(search_direction * SEARCH_STEP_YAW, search_direction * SEARCH_STEP_PITCH)
        return None

# -------------------------------
# 4. 主流程
# -------------------------------
if __name__ == '__main__':
    while True:
        cmd = input("\nPress 's' to start tracking, 'q' to quit: ").strip().lower()
        if cmd == "s":
            print("🔍 Starting face search...")
            while True:
                image_path = request_capture()
                if image_path:
                    result = process_image(image_path)
                    if result and result["yaw"] == 0 and result["pitch"] == 0:  
                        print("✅ Face is perfectly centered. Stopping search.")
                        break  
                time.sleep(SEARCH_DELAY)  
        elif cmd == "q":
            print("Exiting...")
            cv2.destroyAllWindows()  # 关闭所有窗口
            break