All Colors
==========

A small programm that tries to [draw as many different as possible colors](http://codegolf.stackexchange.com/questions/22144/images-with-all-colors), evenly distributed in the RGB cube, into an pleasent-looking image.
It is heavily based on a great [idea of József Fejes](http://joco.name/2014/03/02/all-rgb-colors-in-one-image/).

[![(Picture missing, uh oh)](doc/all_colors_s0_4_golden_158.jpg)][output]
[output]: doc/all_colors_s0_4_golden_158.jpg

[video on youtube](http://img.youtube.com/vi/aVV7a8ueHEo/0.jpg)


Dependencies
------------
- [scons](http://www.scons.org/)
- [OpenCV](http://opencv.org/)

```
sudo apt-get install g++ libopencv-dev scons
```

Compilation
-----------
```
scons
```

Usage
-----
```
./release/AllColors 4
```

Images will be written to the `output` directory. (But prepare to wait quite some time.)

In case you want to create a video from all the images afterwards:
```
ffmpeg -r 50 -i output/image%04d.png -vcodec libx264 -preset veryslow -qp 0 output/video.mp4
```