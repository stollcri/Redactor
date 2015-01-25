//
//  SeamCarve.h
//  Redactor
//
//  Created by Christopher Stoll on 1/21/15.
//  Copyright (c) 2015 Christopher Stoll. All rights reserved.
//

#ifndef __Redactor__SeamCarve__
#define __Redactor__SeamCarve__

#include <stdio.h>

struct pixel {
    int bright;
    int seamval;
    int usecount;
};

void seamCarve(unsigned char *imageVector, int imageWidth, int imageHeight, int imageDepth, int faceCount, int *faceBounds);
void mergeImages(unsigned char *imageVector, unsigned char *pixelatePixels, int imgWidth, int imgHeight, int imgDepth, int faceCount, int *faceBounds);

#endif /* defined(__Redactor__SeamCarve__) */
