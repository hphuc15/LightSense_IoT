from flask import Flask, render_template, request, jsonify

app = Flask(__name__)

records = []

# Test demo for HTTP POST
@app.route("/test_post", methods = ["GET", "POST"])
def test_post():
    if request.method == "POST":
        new_data = request.get_json()
        if not new_data:
            return jsonify({"status": "record failed"}), 400
    
    # record này chỉ dành cho templates/test_post.html
        record = {
            "timestamp": new_data["timestamp"],
            "light": new_data["light"]
        }
        records.append(record)
        return jsonify({"status": "OK"})

    if request.method == "GET":
        latest = records[-1] if records else None
        return render_template("test_post.html", record=latest)
    # render một html để hiển thị data vừa gửi lên



if __name__ == "__main__":
    app.run(host = "0.0.0.0", debug = True)