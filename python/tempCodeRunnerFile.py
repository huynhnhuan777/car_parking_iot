 #Kết nối Database
db = mysql.connector.connect(
    host="127.0.0.1",
    user="root",
    password="",
    database="smartparkingdb"
)
cursor = db.cursor()

# 2. Kết nối Serial
ser = serial.Serial('COM2', 9600, timeout=1)
