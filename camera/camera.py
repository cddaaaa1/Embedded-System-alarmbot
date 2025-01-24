from flask import Flask, request, jsonify, send_file
import subprocess

app = Flask(__name__)

FRAME_WIDTH = 1920 
FRAME_HEIGHT = 1080  

@app.route('/capture', methods=['GET'])
def capture():
    print("Capturing image using libcamera-still...")

    # **优化拍照**
    image_path = "capture.jpg"
    
    subprocess.run([
        "libcamera-still",
        "-o", "capture.jpg",
        "--width", str(FRAME_WIDTH),
        "--height", str(FRAME_HEIGHT),
        "--nopreview",
        "-t", "100"
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


    print("Image captured, sending to client...")
    return send_file(image_path, mimetype='image/jpeg')

@app.route('/update_angles', methods=['POST'])
def update_angles():
    data = request.json
    if "yaw" in data and "pitch" in data:
        yaw, pitch = data["yaw"], data["pitch"]
        print(f"Received Angles: Yaw={yaw:.2f}°, Pitch={pitch:.2f}°")
        return jsonify({"status": "success", "message": "Angles received"}), 200
    else:
        return jsonify({"status": "error", "message": "Invalid data"}), 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001)