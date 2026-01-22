from flask import Flask, render_template, request, redirect, session, jsonify
import mysql.connector
import hashlib

app = Flask(__name__)

app = Flask(__name__)
app.secret_key = "secret_key_demo"  # cần cho session

def get_db_connection():
    return mysql.connector.connect(
        host="127.0.0.1",
        user="root",
        password="",
        database="smartparkingdb"
    )

@app.route('/')
def index():
    # Trang chủ hiển thị giao diện
    return render_template('index.html')

@app.route('/api/status')
def status():
    # API để lấy dữ liệu mới nhất từ MySQL gửi lên Web
    db = get_db_connection()
    cursor = db.cursor(dictionary=True)
    cursor.execute("SELECT * FROM parking_slots ORDER BY slot_id")
    slots = cursor.fetchall()
    cursor.close()
    db.close()
    return jsonify(slots)
@app.route('/api/dashboard')
def dashboard():
    db = get_db_connection()
    cursor = db.cursor(dictionary=True)

    # Lấy trạng thái từng ô
    cursor.execute("SELECT slot_id, is_occupied FROM parking_slots ORDER BY slot_id")
    slots = cursor.fetchall()

    # Lấy tổng số chỗ trống mới nhất
    cursor.execute("""
        SELECT total_available 
        FROM parking_logs 
        ORDER BY created_at DESC 
        LIMIT 1
    """)
    total = cursor.fetchone()

    cursor.close()
    db.close()

    return jsonify({
        "total_available": total["total_available"] if total else 0,
        "slots": slots
    })
# ================= ADMIN LOGIN =================
@app.route('/admin/login', methods=['GET', 'POST'])
def admin_login():
    error = None

    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        password_hash = hashlib.sha256(password.encode()).hexdigest()

        db = get_db_connection()
        cursor = db.cursor(dictionary=True)
        cursor.execute(
            "SELECT * FROM admin_users WHERE username=%s AND password_hash=%s",
            (username, password_hash)
        )
        admin = cursor.fetchone()
        cursor.close()
        db.close()

        if admin:
            session['admin_id'] = admin['admin_id']
            session['admin_name'] = admin['full_name']
            return redirect('/admin')
        else:
            error = "Sai tài khoản hoặc mật khẩu"

    return render_template('admin_login.html', error=error)

# ================= ADMIN DASHBOARD =================
@app.route('/admin')
def admin_dashboard():
    if 'admin_id' not in session:
        return redirect('/admin/login')

    return render_template(
        'admin_index.html',
        admin_name=session['admin_name']
    )

# ================= LOGOUT =================
@app.route('/admin/logout')
def admin_logout():
    session.clear()
    return redirect('/admin/login')

@app.route('/admin')
def admin_index():
    if 'admin_id' not in session:
        return redirect('/admin/login')

    return render_template(
        'admin_index.html',
        admin_name=session.get('admin_name', 'Admin')
    )

if __name__ == '__main__':
    app.run(debug=True, port=5000)