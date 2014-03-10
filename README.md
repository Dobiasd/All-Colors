All Colors
==========

A small programm that tries to [draw as many different as possible colors](http://codegolf.stackexchange.com/questions/22144/images-with-all-colors), evenly distributed in the RGB cube, into an pleasent-looking image.
It is heavily based on a great [idea of József Fejes](http://joco.name/2014/03/02/all-rgb-colors-in-one-image/).

[![(Picture missing, uh oh)](doc/all_colors_s0_4_golden_158.jpg)][output]
[output]: doc/all_colors_s0_4_golden_158.jpg

[video on youtube](https://www.youtube.com/watch?v=aVV7a8ueHEo)


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
mkdir output
./release/AllColors 4
```

Images will be written to the `output` directory. (But prepare to wait quite some time.)

In case you want to create a video from all the images afterwards:
```
ffmpeg -r 50 -i output/image%04d.png -vcodec libx264 -preset veryslow -qp 0 output/video.mp4
```

How does it work?
-----------------

First, the [RGB cube](http://en.wikipedia.org/wiki/RGB_color_space) is walked with three nested loops, to produce a list of all possible colors. This List is then shuffled and sorted by [hue](http://en.wikipedia.org/wiki/Hue).
Additionally a set of all posssible next positions in the destination image is initalized with some fixed points (2, 3 or 4) or a binary input image.

The colors are then placed into the image one after another to one of the possible positions. This position list is constantly updated after every iteration.

The decision which next possible position to use for the color just popped from the queue is where all the fun happens. The position is chosen by selecting the one with the smallest average euclidian RGB difference to the [8-neighbourhood](http://en.wikipedia.org/wiki/Pixel_connectivity#8-connected) (only aleady filled pixels) divided by the count of filled neighbours. The division avoids coral like growing and ensures a more compact shape to emerge.

The output images are finally embellished by filling the remaining black gaps with a half transparent version of a [dilated](http://en.wikipedia.org/wiki/Dilation_(morphology)) and [median-filtered](http://en.wikipedia.org/wiki/Median_filter) version of itself. This way the borders and gaps become more smooth.

Outlook
-------
The determination of the best fitting position from the list of the next possible ones for a given colour is now `O(n)`, `n` being the number of possible positions, since just a linear search is performed.
If the next positions were stored in [R-trees](http://en.wikipedia.org/wiki/R-tree), one for all with a specific neighbour count, an nearest neighbour search could be performed in O(log(n)). The search space would have 3*number_of_neighbours dimensions. E.g. the one for the two-neighbour-pixels would have `r1,g1,b1,r2,g2,b2`. Since the divisor (number of neighbours) would be equal for all entries on one search space, only the best fitting positions from every one of the eight trees must be compared linearly. A speedup of about three orders of magnitude can be expected by complexity reduction.