All Colors
==========

A small programm that tries to [draw as many different as possible colors](http://codegolf.stackexchange.com/questions/22144/images-with-all-colors), evenly distributed in the RGB cube, into an pleasent-looking image.
It is heavily based on a great [Idea of József Fejes](http://joco.name/2014/03/02/all-rgb-colors-in-one-image/).

[![(Picture missing, uh oh)](doc/all_colors_s0_4_golden_158.jpg)][output]
[output]: doc/all_colors_s0_4_golden_158.jpg

[![Video](http://img.youtube.com/vi/aVV7a8ueHEo/0.jpg)](http://www.youtube.com/watch?v=aVV7a8ueHEo)


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