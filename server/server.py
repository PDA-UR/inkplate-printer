from flask import Flask, send_file, make_response, Response, request, after_this_request
import os
import io
import string
import sys

app = Flask(__name__)

clients = {}
server_home_path = os.path.expanduser("~") + "/.inkplate-printer"
img_queue_folder = server_home_path + "/" + "queue"

@app.route("/")
def img_route():
    client_mac = request.args.get("client")

    # check if there is an image for this specific device
    file_path = client_has_new_img(client_mac)
    # otherwise get an image from the queue
    if not file_path:
        file_path = get_img_from_queue()
    
    # return an image if we got one, otherwise return a different status code
    if file_path:
        file_data = io.BytesIO()
        with open(file_path, "rb") as f:
            file_data.write(f.read())
        file_data.seek(0)
        os.remove(file_path)
        return send_file(file_data, mimetype="image/png", download_name="unknown.png")
    else: 
        return Response("", status=201, mimetype='image/png')

def client_has_new_img(client_mac):
    if not client_mac in clients.keys():
        clients[client_mac] = False
        return False
    else:
        if(clients.get(client_mac)):
            img = clients.get(client_mac)
            clients[client_mac] = False
            return img
        else:
            return False

def get_img_from_queue():
    print(img_queue_folder)
    for f in os.listdir(img_queue_folder):
        if f.endswith(".png"):
            return img_queue_folder + "/" + f
    return False

def new_img_available(mac_addr, img):
    clients[mac_addr] = img

def create_folders():
    if not os.path.exists(server_home_path):
        os.mkdir(server_home_path)
    if not os.path.exists(img_queue_folder):
        os.mkdir(img_queue_folder)

if __name__ == "__main__":
    create_folders()
    app.run(host="0.0.0.0")