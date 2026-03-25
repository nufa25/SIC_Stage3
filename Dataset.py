import paho.mqtt.client as mqtt
import json
import csv

# ========== MQTT CONFIG ==========
broker = "broker.hivemq.com"
port = 1883
topic = "sic/re202/sensor"

# ========== CSV CONFIG ==========
csv_file = "dataset778.csv"
fields = ["timestamp", "Suhu", "Humidity", "label", "suhu_luar"]
data_count = 0
max_data = 180

# Buat CSV + header
with open(csv_file, mode="w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=fields)
    writer.writeheader()

# ========== MQTT CALLBACK ==========
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Terhubung ke broker MQTT!")
        client.subscribe(topic)
    else:
        print("Gagal terhubung, kode:", rc)

def on_message(client, userdata, msg):
    global data_count

    if data_count >= max_data:
        print("Jumlah data tercapai, MQTT dihentikan.")
        client.disconnect()
        return

    try:
        payload = json.loads(msg.payload.decode())

        timestamp = payload.get("timestamp")
        suhu = payload.get("Suhu")
        kelembapan = payload.get("Humidity")
        label = payload.get("label")
        suhu_luar = payload.get("suhu_luar")

        # Validasi
        if timestamp is None:
            print("Data tanpa timestamp, dilewati")
            return

        # Simpan ke CSV
        with open(csv_file, mode="a", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=fields)
            writer.writerow({
                "timestamp": timestamp,
                "Suhu": suhu,
                "Humidity": kelembapan,
                "label": label,
                "suhu_luar": suhu_luar
            })

        data_count += 1
        print(f"{data_count}: t={timestamp} | Suhu={suhu} | Hum={kelembapan} | {label}")

    except Exception as e:
        print("Error parsing message:", e)

# ========== MQTT START ==========
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(broker, port, 60)
client.loop_forever()
