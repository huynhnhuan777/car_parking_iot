import serial
import mysql.connector

# 1. Kết nối Database
db = mysql.connector.connect(
    host="127.0.0.1",
    user="root",
    password="",
    database="smartparkingdb"
)
cursor = db.cursor()

# 2. Kết nối Serial
ser = serial.Serial('COM2', 9600, timeout=1)

last_line = ""
last_slot_count = 11  # Ban đầu giả sử còn 11 chỗ trống

TOTAL_SLOTS = 11

print("--- Hệ thống đang chạy: Đang theo dõi xe vào/ra ---")

while True:
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()

        if line and line != last_line:
            data = line.split(',')

            # 11 slot + 1 total_available = 12
            if len(data) == TOTAL_SLOTS + 1:
                try:
                    # ===== LẤY TOTAL SLOT =====
                    current_slot_count = int(data[TOTAL_SLOTS])

                    # ===== PHẦN 1: UPDATE TRẠNG THÁI TỪNG SLOT =====
                    for i in range(TOTAL_SLOTS):
                        is_occupied = int(data[i])  # 0 / 1
                        slot_id = i + 1

                        sql_update = """
                            UPDATE parking_slots
                            SET is_occupied = %s
                            WHERE slot_id = %s
                        """
                        cursor.execute(sql_update, (is_occupied, slot_id))

                    # ===== PHẦN 2: XÁC ĐỊNH IN / OUT =====
                    action = None
                    if current_slot_count < last_slot_count:
                        action = 'IN'
                    elif current_slot_count > last_slot_count:
                        action = 'OUT'

                    if action:
                        sql_log = """
                            INSERT INTO parking_logs (action_type, total_available)
                            VALUES (%s, %s)
                        """
                        cursor.execute(sql_log, (action, current_slot_count))

                    db.commit()

                    print(
                        f"Sự kiện: {action if action else 'Sensor Update'} "
                        f"| Trống: {current_slot_count}"
                    )

                    last_slot_count = current_slot_count
                    last_line = line

                except Exception as e:
                    print(f"Lỗi xử lý dữ liệu: {e}")
            else:
                print(f"Dữ liệu sai format: {line}")
