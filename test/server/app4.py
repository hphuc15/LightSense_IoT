from flask import Flask, render_template, request, jsonify
from datetime import datetime

app = Flask(__name__)

# Lưu trữ các bản ghi ánh sáng
light_records = []

@app.route("/")
def index():
    """Trang chủ hiển thị dữ liệu mới nhất"""
    latest = light_records[-1] if light_records else None
    return render_template("index.html", record=latest, total=len(light_records))

@app.route("/api/light", methods=["GET", "POST"])
def handle_light_data():
    """API endpoint để nhận và trả về dữ liệu ánh sáng"""
    
    if request.method == "POST":
        # Nhận dữ liệu JSON từ ESP32
        data = request.get_json()
        
        if not data:
            return jsonify({"status": "error", "message": "No JSON data received"}), 400
        
        # Kiểm tra dữ liệu bắt buộc
        if "light" not in data:
            return jsonify({"status": "error", "message": "Missing 'light' field"}), 400
        
        # Tạo bản ghi mới
        record = {
            "timestamp": data.get("timestamp", datetime.now().strftime("%Y-%m-%d %H:%M:%S")),
            "light": data["light"],
            "unit": data.get("unit", "lux")
        }
        
        light_records.append(record)
        
        # Giới hạn số lượng bản ghi (giữ 100 bản ghi gần nhất)
        if len(light_records) > 100:
            light_records.pop(0)
        
        print(f"[INFO] Received light data: {record}")
        
        return jsonify({
            "status": "success",
            "message": "Data received successfully",
            "record": record
        }), 200
    
    elif request.method == "GET":
        # Trả về dữ liệu mới nhất
        if not light_records:
            return jsonify({
                "status": "success",
                "data": None,
                "message": "No data available"
            }), 200
        
        # Lấy số lượng bản ghi cần trả về (mặc định: 10)
        limit = request.args.get('limit', default=10, type=int)
        limit = min(limit, 100)  # Giới hạn tối đa 100
        
        return jsonify({
            "status": "success",
            "data": light_records[-limit:],
            "total": len(light_records)
        }), 200

@app.route("/api/light/latest")
def get_latest():
    """Lấy dữ liệu mới nhất"""
    latest = light_records[-1] if light_records else None
    return jsonify({
        "status": "success",
        "data": latest
    })

@app.route("/api/light/clear", methods=["POST"])
def clear_records():
    """Xóa tất cả dữ liệu"""
    light_records.clear()
    return jsonify({
        "status": "success",
        "message": "All records cleared"
    })

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)