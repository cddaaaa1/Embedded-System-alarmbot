from flask import Flask, request, jsonify, send_file
import subprocess
import requests 

app = Flask(__name__)

FRAME_WIDTH = 1920 
FRAME_HEIGHT = 1232  

@app.route('/capture', methods=['GET'])
def capture():
    """Requests image capture from Raspberry Pi camera."""
    print("📸 Capturing image using libcamera-still...")

    image_path = "capture.jpg"
    subprocess.run([
        "libcamera-still",
        "-o", "capture.jpg",
        "--width", str(FRAME_WIDTH),
        "--height", str(FRAME_HEIGHT),
        "--nopreview",
        "-t", "100"
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    print("✅ Image captured, sending to client...")
    return send_file(image_path, mimetype='image/jpeg')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001)