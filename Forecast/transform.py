from datetime import datetime
import requests
import cairosvg
import subprocess
from datetime import datetime
from PIL import Image, ImageEnhance

import sys
import os
libdir = os.path.join('/home/pi/builds/e-Paper/RaspberryPi_JetsonNano/python', 'lib')

if os.path.exists(libdir):
    sys.path.append(libdir)

from waveshare_epd import epd2in7
import time


def get_img():

    dt = datetime.today()  # Get timezone naive now
    seconds = int(dt.timestamp())

    response = requests.get("https://meteo.search.ch/images/chart/meyrin.svg?width=264&height=176&t="+str(seconds)+"&harmonize=1")

    svg = response.text
    grad_start = svg.index("<linearGradient")
    grad_stop  = svg.index("</linearGradient>")
    if grad_start != -1 and grad_stop != -1:
        svg = svg[:grad_start] + svg[grad_stop+18:]

    with open("image.svg","w") as svg_file:
        svg_file.write(svg)

    cairosvg.svg2png(url='image.svg', write_to='image.png')


# sudo apt-get install imagemagick
def convert():

    pipe = subprocess.Popen(["convert",
                             "image.png",
                             "-threshold", "5",
                             "-negate", 
                             "-flatten",
#                             "+matte",
#                             "-colors","2",
#                             "-depth","1",
                             "image.bmp"
                            ],
                            shell=False,
                            stdout=subprocess.PIPE)

    stdout, strerr = pipe.communicate()

    if pipe.returncode != 0:
        raise Exception(stderr)


def display(epd):
    image = Image.open('image.bmp')

    # for a very unlikely event of keyboard interrupt during communication with epd
    try:
        epd.Init_4Gray()
        epd.display_4Gray(epd.getbuffer_4Gray(image))
        epd.sleep()

    except KeyboardInterrupt:
        print("ctrl + c:")
        epd2in7.epdconfig.module_exit()
        exit()


if __name__ == "__main__":

    epd = epd2in7.EPD()

#   for frequent updates it make sense to initialize epd once and avoid putting it to sleep every time
#    epd.init()
#    epd.Clear(0xFF)
#    epd.Init_4Gray()

    while True:
        get_img()
        convert()
        display(epd)
        time.sleep(900)
