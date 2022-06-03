from flask import Flask, send_file

app = Flask(__name__)

@app.route("/")
def hello_world():
    return send_file("example.png", mimetype='image/png')

if __name__ == "__main__":
    app.run(host="0.0.0.0")