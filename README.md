libjpeg
=======

A complete implementation of 10918-1 (JPEG) coming from jpeg.org (the ISO group) with extensions for HDR standardized as 18477 (JPEG XT)

This release also includes the "JPEG on Steroids" improvements implemented for the ICIP 2016 Grand Challenge on Image Compression. For ideal visual performance, run jpeg as follows:

jpeg -q <quality> -oz -v -qt 3 -h -s 1x1,2x2,2x2 input.ppm output.jpg

