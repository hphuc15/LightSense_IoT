from flask import Flask, request, render_template, jsonify, g
from datetime import datetime, timezone
import sqlite3

app = Flask(__name__)

DATABASE = r"D:\WorkSpace\Projects\LightSense_IoT\database\mydb.sqlite"
# data = [] # Fake database

def get_db():
    if "db" not in g:
        g.db = sqlite3.connect(DATABASE)
        g.db.row_factory = sqlite3.Row
    return g.db

@app.teardown_appcontext
def close_db(exception):
    db = g.pop("db", None)
    if db is not None:
        db.close() 

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
