# ImgReduce
Algorithmic implementation for extracting globally unique pixels from a collection of bitmap images

# What this Project Does:
This project takes a crunchypack builder file (generated via CrunchyUI) or a standalone collection of .bmp files and extracts the pixels that represent each image uniquely among the entire collection of images. The program constrains itself to never use more running memory than the largest image in the set plus all previously unique pixels extracted plus some small constant amount for other operations. These pixels are used to generate a complete crunchypack output file of imgcode-image_identifier pairs that can be read into a program to generate an in-memory map that, when searching an image in O(n) time, can identify if it contains a subimage pixel matching to an image in the collection in O(1) time.

# How to Install this Program:
This project is currently only available as public source code and is undergoing cross-platform rewrites before a public executable is released. This version is currently platform specific to Windows as it relies on Win32 API calls to search for image files and capture image file header information. The source code will compile to an executable on any C++11 standard compliant compiler. 

# Example Usage:
I have a collection of images, redcup.bmp, redtruck.bmp, greentree.bmp, and bluesky.bmp, and a video that may show at some point one or all of these images. I would like to use as little memory as possible and be notified very quickly when one of these images emerges. All of these images are fed into the ImgReduce algorithm and it removes duplicate-with-self pixels (redcup likely has several red pixels that make up the cup, some of which are mathmatically identical reds: only one of these is necessary to identify the cup). Once duplicate-with-self is complete each image is stripped down to the unique pixels it contains and self-with-global is computed. This step makes sure that pixels placed into the final collection are truly unique to an image: if redtruck and redcup share a mathmatically identical red then that pixel should be removed from both of their collections as it contains global level ambiguity (images CAN become empty if an image is made entirely of pixels from another image). Finally, the result of this processing is prepared in a output file with numerical_code-imageID format that can be read into a program to build the map. Lookup of a subimage within the image is as easy as reading the image in O(n) and checking if that value is in the map. 

# Development Enviornment Setup:
 Visual Studio 2015 Community edition, available for free from Microsoft, is the IDE of choice. Follow the standard Microsoft installation instructions making sure Microsoft foundation classes are included in the install. It contains the Win32 API libraries, Win32.h, which will be required to build and run the full project. 
 
# Change Requests:
 Changes should be submitted as plaintext source code with file name and line numbers to project lead: james@leppek.us
