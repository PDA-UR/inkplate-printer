from PIL import Image, ImageOps
from pathlib import Path
from pdf2image import convert_from_path
import glob
import os
import sys

display_size = (1200, 825)
server_home_path = os.path.expanduser("~") + "/.inkplate-printer"
img_queue_folder = server_home_path + "/" + "queue"

def move_to_queue(ps_filename):
    Path(ps_filename).unlink()
    bmp_files = glob.glob(ps_filename[:-3] + "*.bmp")
    for f in bmp_files:
        print("1")
        img = Image.open(f)
        print("2")

        img = img.rotate(90, expand=1)
        print("3")

        img.save(img_queue_folder+f[1:])
        print("4")
        Path(f).unlink()

def process_image(file):
    bmp_files = glob.glob(ps_filename[:-3] + "*.bmp")
    for f in bmp_files:
        img = Image(f)
        img = img.rotate(90, expand=1)
        img.save(f)

if __name__ == "__main__":
    file = sys.argv[1]
    # process_image(file)
    move_to_queue(file)