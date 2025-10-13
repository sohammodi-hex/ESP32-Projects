# Face tracking to Arduino pan: detects face with OpenCV and sends "X<delta>\n" or "CENTER\n".
import cv2
import serial
import time
import os
import sys

# Adjust these as needed
PORT = 'COM8'       # Windows: COMx, Linux: '/dev/ttyACM0' or '/dev/ttyUSB0'
BAUD = 115200
CAM_INDEX = 0       # try 1 if 0 picks the wrong camera
FRAME_W, FRAME_H = 320, 240

# Serial open
try:
    ser = serial.Serial(PORT, BAUD, timeout=0.02)
except Exception as e:
    print(f"[ERROR] Cannot open serial port {PORT}: {e}")
    sys.exit(1)

time.sleep(2.0)  # allow Arduino to reset

# Camera open
cap = cv2.VideoCapture(CAM_INDEX)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_W)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_H)
if not cap.isOpened():
    print(f"[ERROR] Cannot open camera index {CAM_INDEX}")
    ser.close()
    sys.exit(1)

# Robust Haar cascade load (uses OpenCV’s data path)
cascade_path = os.path.join(cv2.data.haarcascades, 'haarcascade_frontalface_default.xml')
cascade = cv2.CascadeClassifier(cascade_path)
if cascade.empty():
    print(f"[ERROR] Cannot load Haar cascade at: {cascade_path}")
    cap.release()
    ser.close()
    sys.exit(1)

W, H = FRAME_W, FRAME_H
center_x = W // 2
deadband = 12          # pixels
max_cmd = 20           # max step command magnitude per frame
scale = 0.08           # pixels -> command units (tune 0.05..0.15)
last_seen = time.time()
sent_center = False

print("Running: ESC to quit")

try:
    while True:
        ok, frame = cap.read()
        if not ok:
            print("[WARN] Camera read failed")
            break

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=5, minSize=(40, 40))

        # center guide
        cv2.line(frame, (center_x, 0), (center_x, H), (255, 255, 255), 1)

        if len(faces) > 0:
            # largest face
            (x, y, w, h) = max(faces, key=lambda r: r[2] * r[3])
            cx = x + w // 2
            err = center_x - cx

            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)

            if abs(err) > deadband:
                cmd = int(max(-max_cmd, min(max_cmd, scale * err)))
                try:
                    ser.write(f"X{cmd}\n".encode('ascii'))
                except Exception as e:
                    print(f"[WARN] Serial write failed: {e}")
                sent_center = False
            last_seen = time.time()
        else:
            # If no face for 1.5s, issue a single CENTER
            if time.time() - last_seen > 1.5 and not sent_center:
                try:
                    ser.write(b"CENTER\n")
                except Exception as e:
                    print(f"[WARN] Serial write failed: {e}")
                sent_center = True

        cv2.imshow("Face Track", frame)
        if (cv2.waitKey(1) & 0xFF) == 27:  # ESC
            break

finally:
    try:
        ser.write(b"STOP\n")
    except Exception:
        pass
    cap.release()
    cv2.destroyAllWindows()
    ser.close()
