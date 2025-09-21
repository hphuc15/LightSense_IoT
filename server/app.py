from flask import Flask, request, render_template, jsonify
from datetime import datetime, timezone
import mysql.connector

app = Flask(__name__)


@app.route("/")
def home():
    return "<h1>Homepage</h1>"


@app.route("/api/data", methods=["POST"])
def receive_data():
    new_data = request.get_json()
    if not new_data:
        return '{"status": "record failed"}', 400
    
    record = {
        "timestamp": datetime.now(timezone.utc),
        "lux": new_data["lux"]
    }

    with mysql.connector.connect(
        host = "localhost",
        user = "root",
        password = "hongphucv9@",
        database = "bh1750_db"
    ) as conn:
        with conn.cursor() as curs:
            insert_query = "INSERT INTO data (timestamp, lux) VALUES (%s, %s)"
            insert_query_argument = (record["timestamp"], record["lux"])
            curs.execute(insert_query, insert_query_argument)
            conn.commit()

    return '{"status": "record succes"}'


@app.route("/history", methods=["GET"])
def get_data():
    with mysql.connector.connect(
        host = "localhost",
        user = "root",
        password = "hongphucv9@",
        database = "bh1750_db"
    ) as conn:
        with conn.cursor(dictionary = True) as curs:
            select_query = "SELECT timestamp, lux FROM data ORDER BY timestamp DESC"
            curs.execute(select_query)
            records = curs.fetchall()

    return render_template("data.html", records=records)

if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)


# record: a single entry to INSERT into the database
# records: a list of entries fetched from the database