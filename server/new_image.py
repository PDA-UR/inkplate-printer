from PIL import Image, ImageOps
from pathlib import Path
import os
import sys

display_size = (1200, 825)
server_home_path = os.path.expanduser("~") + "/.inkplate-printer"
img_queue_folder = server_home_path + "/" + "queue"

def move_to_queue(file):
    Path(file[:-4]).unlink()
    Path(file).rename(img_queue_folder+file[1:])

def process_image(file):
    img = Image.open(file)
    img = ImageOps.grayscale(img)
    img_size = img.size
    # rotate portrait images
    if img_size[0] < img_size[1]:
        ratio = display_size[0] / img_size[1]
    else:
        ratio = display_size[0] / img_size[0]

    if img_size[0] * ratio > display_size[1]:
        ratio = display_size[1] / img_size[0]

    # resize
    # img = img.resize((round(img_size[0]*ratio), round(img_size[1]*ratio)), Image.Resampling.BICUBIC)
    img = img.rotate(90, expand=1)

    img.save(file)

if __name__ == "__main__":
    file = sys.argv[1]
    process_image(file)
    move_to_queue(file)