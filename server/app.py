from flask import Flask, request, render_template, jsonify
from datetime import datetime, timezone

app = Flask(__name__)

data = [] # Fake database

@app.route("/")
def home():
    return "<h1>Homepage</h1>"

@app.route("/api/data", methods=["POST"])
def receive_data():
    new_data = request.get_json()
    if new_data:
        record = {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "data": new_data
        }
        data.append(record)
        return '{"status": "success"}'
    return '{"status": "error"}', 400

@app.route("/history")
def get_data():
    return render_template("data.html", data=data)
# Test: $ curl -X POST http://192.168.1.13:5000/api/data -H "Content-Type: application/json" -d '{"temperature": 25, "humidity": 60}'

if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)
