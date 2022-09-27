from flask import Flask, send_file, make_response, Response, request, after_this_request
import os
import io
import string
import sys
import configparser
import yaml

app = Flask(__name__)

clients = {}
server_home_path = os.path.expanduser("~") + "/.inkplate-printer"
img_queue_folder = server_home_path + "/" + "queue"


dirname = os.path.dirname(__file__)
config_filepath = os.path.join(dirname, '../config.yml')
config = yaml.safe_load(open(config_filepath))['PAGE_SERVER']
PORT = config['port']

# check if anything else is running on the port and kill it
os.system("lsof -t -i tcp:" + str(PORT) + " | xargs kill")

print("Starting PAGE server on port " + str(PORT))

@app.route("/hello")
def hello():
    return Response(
        "Hello World!",
        mimetype="text/plain",
        # set cors local network
        headers={"Access-Control-Allow-Origin": "*"}
    )


@app.route("/img")
def img_route():
    print(request)
    client_mac = request.args.get("client")
    doc_name = request.args.get("doc_name")
    page_num = request.args.get("page_num")

    file_path = img_queue_folder + "/" + doc_name + "_" + str(page_num) + ".bmp"

    # return an image if we got one, otherwise return a different status code
    if file_path:
        file_data = io.BytesIO()
        with open(file_path, "rb") as f:
            file_data.write(f.read())
        file_data.seek(0)
        # os.remove(file_path)
        print("sending file: " + file_path)
        return send_file(file_data, mimetype="image/bmp", download_name="unknown.bmp")
    else: 
        return Response("", status=201, mimetype='image/bmp')
    
@app.route("/")
def check_route():
    client_mac = request.args.get("client")

    # check if there is an image for this specific device
    file_path = client_has_new_img(client_mac)

    # otherwise get an image from the queue
    if not file_path:
        file_path = get_img_from_queue()
    
    # return the document name and number of pages if we got a doc to print
    # otherwise return a different status code
    if file_path:
        doc_name = file_path.split("/")[-1].split("_")[0]
        num_pages = get_doc_page_num(doc_name)
        print("doc name: " + doc_name)
        print("pages: " + str(num_pages))
        return Response(doc_name + "_" + str(num_pages), status=200)
    else: 
        return Response("", status=201)

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

def get_doc_page_num(doc_name):
    num_pages = 0
    for f in os.listdir(img_queue_folder):
        if f.startswith(doc_name):
            num_pages += 1
    return num_pages

def get_img_from_queue():
    for f in os.listdir(img_queue_folder):
        if f.endswith(".bmp"):
            return img_queue_folder + "/" + f
    return False

def get_page_for_doc(doc_name, page_num):
    for f in os.listdir(img_queue_folder):
        if f.endswith(".bmp"):
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
    app.run(host = "0.0.0.0", port = PORT)