//
//  SeamCarve.c
//  Redactor
//
//  Created by Christopher Stoll on 1/21/15.
//  Copyright (c) 2015 Christopher Stoll. All rights reserved.
//

#include "SeamCarve.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#define SEAM_TRACE_INCREMENT 1
#define THRESHHOLD_USECOUNT 64

#pragma mark - inlines (min/max)

static inline int max(int a, int b)
{
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

static inline int min(int a, int b)
{
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

static inline int min3(int a, int b, int c)
{
    if (a < b) {
        if (a < c) {
            return a;
        } else {
            return c;
        }
    } else {
        if (b < c) {
            return b;
        } else {
            return c;
        }
    }
}

#pragma mark - edge detectors

static int getPixelGaussian(struct pixel *imageVector, int imageWidth, int imageHeight, int pixelDepth, int currentPixel, int sigma)
{
    int imageByteWidth = imageWidth * pixelDepth;
    int points[25];
    double pointValues[25];
    
    points[0] = currentPixel - imageByteWidth - imageByteWidth - pixelDepth - pixelDepth;
    points[1] = currentPixel - imageByteWidth - imageByteWidth - pixelDepth;
    points[2] = currentPixel - imageByteWidth - imageByteWidth;
    points[3] = currentPixel - imageByteWidth - imageByteWidth + pixelDepth;
    points[4] = currentPixel - imageByteWidth - imageByteWidth + pixelDepth + pixelDepth;
    
    points[5] = currentPixel - imageByteWidth - pixelDepth - pixelDepth;
    points[6] = currentPixel - imageByteWidth - pixelDepth;
    points[7] = currentPixel - imageByteWidth;
    points[8] = currentPixel - imageByteWidth + pixelDepth;
    points[9] = currentPixel - imageByteWidth + pixelDepth + pixelDepth;
    
    points[10] = currentPixel - pixelDepth - pixelDepth;
    points[11] = currentPixel - pixelDepth;
    points[12] = currentPixel;
    points[13] = currentPixel + pixelDepth;
    points[14] = currentPixel + pixelDepth + pixelDepth;
    
    points[15] = currentPixel + imageByteWidth - pixelDepth - pixelDepth;
    points[16] = currentPixel + imageByteWidth - pixelDepth;
    points[17] = currentPixel + imageByteWidth;
    points[18] = currentPixel + imageByteWidth + pixelDepth;
    points[19] = currentPixel + imageByteWidth + pixelDepth + pixelDepth;
    
    points[20] = currentPixel + imageByteWidth + imageByteWidth - pixelDepth - pixelDepth;
    points[21] = currentPixel + imageByteWidth + imageByteWidth - pixelDepth;
    points[22] = currentPixel + imageByteWidth + imageByteWidth;
    points[23] = currentPixel + imageByteWidth + imageByteWidth + pixelDepth;
    points[24] = currentPixel + imageByteWidth + imageByteWidth + pixelDepth + pixelDepth;
    
    // TODO: this is wrong, fix it
    for (int i = 0; i < 25; ++i) {
        if (points[i] < 0) {
            points[i] = 0;
        } else if (points[i] >= (imageHeight * imageWidth * pixelDepth)) {
            points[i] = (imageHeight * imageWidth * pixelDepth) - 1;
        }
    }
    
    // get the pixel values from the image array
    pointValues[0] = (double)imageVector[points[0]].bright;
    pointValues[1] = (double)imageVector[points[1]].bright;
    pointValues[2] = (double)imageVector[points[2]].bright;
    pointValues[3] = (double)imageVector[points[3]].bright;
    pointValues[4] = (double)imageVector[points[4]].bright;
    pointValues[5] = (double)imageVector[points[5]].bright;
    pointValues[6] = (double)imageVector[points[6]].bright;
    pointValues[7] = (double)imageVector[points[7]].bright;
    pointValues[8] = (double)imageVector[points[8]].bright;
    pointValues[9] = (double)imageVector[points[9]].bright;
    pointValues[10] = (double)imageVector[points[10]].bright;
    pointValues[11] = (double)imageVector[points[11]].bright;
    pointValues[12] = (double)imageVector[points[12]].bright;
    pointValues[13] = (double)imageVector[points[13]].bright;
    pointValues[14] = (double)imageVector[points[14]].bright;
    pointValues[15] = (double)imageVector[points[15]].bright;
    pointValues[16] = (double)imageVector[points[16]].bright;
    pointValues[17] = (double)imageVector[points[17]].bright;
    pointValues[18] = (double)imageVector[points[18]].bright;
    pointValues[19] = (double)imageVector[points[19]].bright;
    pointValues[20] = (double)imageVector[points[20]].bright;
    pointValues[21] = (double)imageVector[points[21]].bright;
    pointValues[22] = (double)imageVector[points[22]].bright;
    pointValues[23] = (double)imageVector[points[23]].bright;
    pointValues[24] = (double)imageVector[points[24]].bright;
    
    double gaussL1 = 0.0;
    double gaussL2 = 0.0;
    double gaussL3 = 0.0;
    double gaussL4 = 0.0;
    double gaussL5 = 0.0;
    double gaussAll = 0.0;
    double gaussDvsr = 1.0;
    double weights[25];
    
    if (sigma == 80) {
        // scaling factor / standard deviation / sigma = 8.0
        weights[0]  = 0.038764;
        weights[1]  = 0.039682;
        weights[2]  = 0.039993;
        weights[6]  = 0.040622;
        weights[7]  = 0.040940;
        weights[12] = 0.041261;
    } else if (sigma == 16) {
        // scaling factor / standard deviation / sigma = 1.6
        weights[0]  = 0.017056;
        weights[1]  = 0.030076;
        weights[2]  = 0.036334;
        weights[6]  = 0.053035;
        weights[7]  = 0.064071;
        weights[12] = 0.077404;
    } else if (sigma == 14) {
        // scaling factor / standard deviation / sigma = 1.4
        gaussDvsr = 159;
        weights[0]  = 2;
        weights[1]  = 4;
        weights[2]  = 5;
        weights[6]  = 9;
        weights[7]  = 12;
        weights[12] = 15;
    }
    // line 1 has 2 duplicated values
    weights[3] = weights[1];
    weights[4] = weights[0];
    // line 2 has 3 duplicated values
    weights[5] = weights[1];
    weights[8] = weights[6];
    weights[9] = weights[5];
    // line 3 has 4 duplicated values
    weights[10] = weights[2];
    weights[11] = weights[7];
    weights[13] = weights[11];
    weights[14] = weights[10];
    // line 4 is the same as line 2
    weights[15] = weights[5];
    weights[16] = weights[6];
    weights[17] = weights[7];
    weights[18] = weights[8];
    weights[19] = weights[9];
    // line 5 is the  same as line 1
    weights[20] = weights[0];
    weights[21] = weights[1];
    weights[22] = weights[2];
    weights[23] = weights[3];
    weights[24] = weights[4];
    
    gaussL1 = (weights[1]  * pointValues[0])  + (weights[1]  * pointValues[1])  + (weights[2]  * pointValues[2])  + (weights[3]  * pointValues[3])  + (weights[4]  * pointValues[4]);
    gaussL2 = (weights[5]  * pointValues[5])  + (weights[6]  * pointValues[6])  + (weights[7]  * pointValues[7])  + (weights[8]  * pointValues[8])  + (weights[9]  * pointValues[9]);
    gaussL3 = (weights[10] * pointValues[10]) + (weights[11] * pointValues[11]) + (weights[12] * pointValues[12]) + (weights[13] * pointValues[13]) + (weights[14] * pointValues[14]);
    gaussL4 = (weights[15] * pointValues[15]) + (weights[16] * pointValues[16]) + (weights[17] * pointValues[17]) + (weights[18] * pointValues[18]) + (weights[19] * pointValues[19]);
    gaussL5 = (weights[20] * pointValues[20]) + (weights[21] * pointValues[21]) + (weights[22] * pointValues[22]) + (weights[23] * pointValues[23]) + (weights[24] * pointValues[24]);
    gaussAll = (gaussL1 + gaussL2 + gaussL3 + gaussL4 + gaussL5) / gaussDvsr;
    return min(max((int)gaussAll, 0), 255);
}

static int getPixelEnergyDoG(int gaussianValue1, int gaussianValue2)
{
    double greyPixel = 0.0;

    if (gaussianValue1 > gaussianValue2) {
        greyPixel = (gaussianValue1 - gaussianValue2);
    } else {
        greyPixel = (gaussianValue2 - gaussianValue1);
    }
    
    return min(max(greyPixel, 0), 255);
}

#pragma mark - seam carving

/*
 * Trace all the seams
 * The least signifigant pixels will be traced multiple times and have a higher value (whiter)
 * The most signifigant pixels will not be traced at all and have a value of zero (black)
 */
static void findSeams(struct pixel *imageVector, int imageWidth, int imageHeight, int direction)
{
    // TODO: create macro definition
    int directionVertical = 0;
    int directionHorizontal = 1;
    if ((direction != directionVertical) && (direction != directionHorizontal)) {
        return;
    }
    
    int imageSize = 0; // width when going horizontal, height when going vertical
    int loopBeg = 0; // where the outer loop begins
    int loopEnd = 0; // where the outer loop ends
    int loopInc = 0; // the increment of the outer loop
    
    int nextPixelR = 0; // next pixel to the right
    int nextPixelC = 0; // next pixel to the center
    int nextPixelL = 0; // next pixel to the left
    int currentMin = 0; // the minimum of nextPixelR, nextPixelC, and nextPixelL
    int countGoR = 0; // how many times the seam diverged upward
    int countGoL = 0; // how many times the seam diverged downward
    
    int nextPixelDistR = 0; // memory distance to the next pixel to the right
    int nextPixelDistC = 0; // memory distance to the next pixel to the center
    int nextPixelDistL = 0; // memory distance to the next pixel to the left
    
    // loop conditions depend upon the direction
    if (direction == directionVertical) {
        loopBeg = (imageWidth * imageHeight) - 1 - imageWidth;
        loopEnd = (imageWidth * imageHeight) - 1;
        loopInc = 1;
        
        // also set the next pixel distances
        nextPixelDistR = imageWidth - 1;
        nextPixelDistC = imageWidth;
        nextPixelDistL = imageWidth + 1;
        
        imageSize = imageHeight;
    } else {
        loopBeg = imageWidth - 1;
        loopEnd = (imageWidth * imageHeight) - 0;//1;
        loopInc = imageWidth;
        
        // also set the next pixel distances
        nextPixelDistR = imageWidth + 1;
        nextPixelDistC = 1;
        nextPixelDistL = (imageWidth - 1) * -1;
        
        imageSize = imageWidth;
    }
    
    int minValueLocation = 0;
    // for every pixel in the right-most or bottom-most column of the image
    for (int k = loopBeg; k < loopEnd; k += loopInc) {
        minValueLocation = k;
        countGoR = 0;
        countGoL = 0;
        
        // move right-to-left ot bottom-to-top across/up the image
        for (int j = (imageSize - 1); j > 0; --j) {
            // if this happens there is a bug in the program!
            if (minValueLocation < 0) {
                minValueLocation = 0;
                printf("stop: %d (%d, %d) %d \n", minValueLocation, j, k, direction);
            }
            
            // THIS IS THE CRUCIAL PART
            if (imageVector[minValueLocation].usecount < (255-SEAM_TRACE_INCREMENT)) {
                imageVector[minValueLocation].usecount += SEAM_TRACE_INCREMENT;
            }
            
            // get the possible next pixles
            if ((minValueLocation - nextPixelDistR) > 0) {
                nextPixelR = imageVector[minValueLocation - nextPixelDistR].seamval;
            } else {
                nextPixelR = INT_MAX;
            }
            if ((minValueLocation - nextPixelDistC) > 0) {
                nextPixelC = imageVector[minValueLocation - nextPixelDistC].seamval;
            } else {
                nextPixelC = INT_MAX;
            }
            if (((minValueLocation - nextPixelDistL) > 0) && ((minValueLocation - nextPixelDistL) < loopEnd)) {
                nextPixelL = imageVector[minValueLocation - nextPixelDistL].seamval;
            } else {
                nextPixelL = INT_MAX;
            }
            
            // use the minimum of the possible pixels
            currentMin = min3(nextPixelR, nextPixelC, nextPixelL);
            
            // attempt to make the seam go back down if it was forced up and ice versa
            // the goal is to end on the same line which the seam started on, this
            // minimizes crazy diagonal seams which cut out important information
            if (countGoR == countGoL) {
                if (currentMin == nextPixelC) {
                    minValueLocation -= nextPixelDistC;
                } else if (currentMin == nextPixelR) {
                    minValueLocation -= nextPixelDistR;
                    ++countGoR;
                } else if (currentMin == nextPixelL) {
                    minValueLocation -= nextPixelDistL;
                    ++countGoL;
                }
            } else if (countGoR > countGoL) {
                if (currentMin == nextPixelL) {
                    minValueLocation -= nextPixelDistL;
                    ++countGoL;
                } else if (currentMin == nextPixelC) {
                    minValueLocation -= nextPixelDistC;
                } else if (currentMin == nextPixelR) {
                    minValueLocation -= nextPixelDistR;
                    ++countGoR;
                }
            } else if (countGoR < countGoL) {
                if (currentMin == nextPixelR) {
                    minValueLocation -= nextPixelDistR;
                    ++countGoR;
                } else if (currentMin == nextPixelC) {
                    minValueLocation -= nextPixelDistC;
                } else if (currentMin == nextPixelL) {
                    minValueLocation -= nextPixelDistL;
                    ++countGoL;
                }
            }
        }
    }
}

static void setPixelPathHorizontal(struct pixel *imageVector, int imageWidth, int imageHeight, int currentPixel, int currentCol)
{
    // avoid falling off the right
    if (currentCol < imageWidth) {
        int pixelLeft = 0;
        int leftT = 0;
        int leftM = 0;
        int leftB = 0;
        int newValue = 0;
        
        pixelLeft = currentPixel - 1;
        // avoid falling off the top
        if (currentPixel > imageWidth) {
            // avoid falling off the bottom
            if (currentPixel < ((imageWidth * imageHeight) - imageWidth)) {
                leftT = imageVector[pixelLeft - imageWidth].seamval;
                leftM = imageVector[pixelLeft].seamval;
                leftB = imageVector[pixelLeft + imageWidth].seamval;
                newValue = min3(leftT, leftM, leftB);
            } else {
                leftT = imageVector[pixelLeft - imageWidth].seamval;
                leftM = imageVector[pixelLeft].seamval;
                //leftB = INT_MAX;
                newValue = min(leftT, leftM);
            }
        } else {
            //leftT = INT_MAX;
            leftM = imageVector[pixelLeft].seamval;
            leftB = imageVector[pixelLeft + imageWidth].seamval;
            newValue = min(leftM, leftB);
        }
        imageVector[currentPixel].seamval += newValue;
        //
        // This (below) is kinda a big deal
        //
        if (imageVector[currentPixel].seamval > 0) {
            imageVector[currentPixel].seamval -= 1;
        }
    }
}

static int fillSeamMatrixHorizontal(struct pixel *imageVector, int imageWidth, int imageHeight)
{
    int result = 0;
    int currentPixel = 0;
    // do not process the first row, start with j=1
    // must be in reverse order from verticle seam, calulate colums as we move across (top down, left to right)
    for (int i = 0; i < imageWidth; ++i) {
        for (int j = 1; j < imageHeight; ++j) {
            currentPixel = (j * imageWidth) + i;
            setPixelPathHorizontal(imageVector, imageWidth, imageHeight, currentPixel, i);
            
            if (imageVector[currentPixel].seamval != 0) {
                ++result;
            }
        }
    }
    return result;
}

static void findSeamsHorizontal(struct pixel *imageVector, int imageWidth, int imageHeight)
{
    findSeams(imageVector, imageWidth, imageHeight, 1);
}

static void fudge(unsigned char *imageVector, int imageWidth, int imageHeight, int imageDepth)
{
    // /----=----=----=----=----=----=----\
    // |    |    |    | 03 |    |    |    |
    // |----+----+----+----+----+----+----|
    // |    |    | 07 | 08 | 09 |    |    |
    // |----+----+----+----+----+----+----|
    // | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
    // |----+----+----+----+----+----+----|
    // |    |    | 17 | 18 | 19 |    |    |
    // |----+----+----+----+----+----+----|
    // |    |    |    | 23 |    |    |    |
    // \----=----=----=----=----=----=----/
    
    int pixel03on = 0;
    
    int pixel07on = 0;
    int pixel08on = 0;
    int pixel09on = 0;
    
    int pixel10on = 0;
    int pixel11on = 0;
    int pixel12on = 0;
    int pixel13on = 0;
    int pixel14on = 0;
    int pixel15on = 0;
    int pixel16on = 0;
    
    int pixel17on = 0;
    int pixel18on = 0;
    int pixel19on = 0;
    
    int pixel23on = 0;
    
    int pixelOnA = 0;
    int pixelOnB = 0;
    int pixelOnC = 0;
    
    int currentPixelNumber = 0;
    int currentPixel = 0;
    int leadingPixel = 0;
    int pixelOnSum = 0;
    for (int j = 2; j < (imageHeight - 2); ++j) {
        for (int i = 2; i < (imageWidth - 2); ++i) {
            currentPixelNumber = (j * imageWidth) + i;
            currentPixel = currentPixelNumber * imageDepth;
            pixelOnSum = 0;
            pixelOnA = 0;
            pixelOnB = 0;
            pixelOnC = 0;
            
            leadingPixel = currentPixel - ((imageWidth + imageWidth)  * imageDepth);
            pixel03on = imageVector[leadingPixel+3] ? 1 : 0;
            pixelOnSum  += pixel03on;
            pixelOnA  += pixel03on;
            
            leadingPixel = currentPixel - ((imageWidth - 1)  * imageDepth);
            pixel07on = pixel08on;
            pixel08on = pixel09on;
            pixel09on = imageVector[leadingPixel+3] ? 1 : 0;
            pixelOnSum  += pixel07on + pixel08on + pixel09on;
            pixelOnA  += pixel07on + pixel08on + pixel09on;
            
            leadingPixel = currentPixel + (3 * imageDepth);
            pixel10on = pixel11on;
            pixel11on = pixel12on;
            pixel12on = pixel13on;
            pixel13on = pixel14on;
            pixel14on = pixel15on;
            pixel15on = pixel16on;
            pixel16on = imageVector[leadingPixel+3] ? 1 : 0;
            pixelOnSum  += pixel10on + pixel11on + pixel12on + pixel13on + pixel14on + pixel15on + pixel16on;
            pixelOnB  += pixel10on + pixel11on + pixel12on + pixel13on + pixel14on + pixel15on + pixel16on;
            
            leadingPixel = currentPixel + ((imageWidth + 1)  * imageDepth);
            pixel17on = pixel18on;
            pixel18on = pixel19on;
            pixel19on = imageVector[leadingPixel+3] ? 1 : 0;
            pixelOnSum  += pixel17on + pixel18on + pixel19on;
            pixelOnC  += pixel17on + pixel18on + pixel19on;
            
            leadingPixel = currentPixel + ((imageWidth + imageWidth)  * imageDepth);
            pixel23on = imageVector[leadingPixel+3] ? 1 : 0;
            pixelOnSum  += pixel23on;
            pixelOnC  += pixel23on;
            
            if (!pixel13on) {
                if (pixelOnSum > 4) {
                    imageVector[currentPixel+3] = 255;
                } else if (pixelOnA && pixelOnC) {
                    imageVector[currentPixel+3] = 255;
                } else if (pixelOnB > 1) {
                    imageVector[currentPixel+3] = 255;
                }
            }
        }
    }
}

#pragma mark - public functions

void seamCarve(unsigned char *imageVector, int imageWidth, int imageHeight, int imageDepth, int faceCount, int *faceBounds)
{
    if ((imageWidth <= 0) || (imageHeight <= 0) || (imageDepth <= 0)) {
        return;
    }
    
    struct pixel *workingImageH = (struct pixel*)malloc((unsigned long)imageWidth * (unsigned long)imageHeight * sizeof(struct pixel));
    
    int inputPixel = 0;
    int outputPixel = 0;
    int currentPixel = 0;
    int currentBrightness = 0;
    double currentRadians = 0;
    // fill initial data structures
    for (int j = 0; j < imageHeight; ++j) {
        for (int i = 0; i < imageWidth; ++i) {
            currentPixel = (j * imageWidth) + i;
            inputPixel = currentPixel * imageDepth;
            
            currentBrightness = (imageVector[inputPixel] * 0.299) + (imageVector[inputPixel+1] * 0.587) + (imageVector[inputPixel+2] * 0.114);
            currentRadians = ((double)currentBrightness / 255.0) * 3.14159265359;
            currentBrightness = (int)(((1.0 - cos(currentRadians)) / 2.0) * 255.0);
            
            struct pixel newPixelH;
            newPixelH.bright = currentBrightness;
            newPixelH.seamval = 0;
            newPixelH.usecount = 0;
            workingImageH[currentPixel] = newPixelH;
        }
    }
    
    //
    // energy calculation
    //
    
    int gaussA = 0;
    int gaussB = 0;
    for (int j = 0; j < imageHeight; ++j) {
        for (int i = 0; i < imageWidth; ++i) {
            currentPixel = (j * imageWidth) + i;
            gaussA = getPixelGaussian(workingImageH, imageWidth, imageHeight, 1, currentPixel, 14);
            gaussB = getPixelGaussian(workingImageH, imageWidth, imageHeight, 1, currentPixel, 16);
            workingImageH[currentPixel].seamval = getPixelEnergyDoG(gaussA, gaussB);
        }
    }
    
    //
    // seam carving
    //
    
    fillSeamMatrixHorizontal(workingImageH, imageWidth, imageHeight);
    findSeamsHorizontal(workingImageH, imageWidth, imageHeight);

    //
    // output preparation
    //
    
    int currentUseCount = 0;
    for (int j = 0; j < imageHeight; ++j) {
        for (int i = 0; i < imageWidth; ++i) {
            currentPixel = (j * imageWidth) + i;
            outputPixel = currentPixel * imageDepth;
            
            currentUseCount = workingImageH[currentPixel].usecount;
            
            // untraced areas contain data (set alpha to 100%)
            if ((currentUseCount < SEAM_TRACE_INCREMENT) && (i > 0) && (j > 0)) {
                imageVector[outputPixel] = 0;
                imageVector[outputPixel+1] = 0;
                imageVector[outputPixel+2] = 0;
                imageVector[outputPixel+3] = 255;
            
            // traced areas are void (set alpha to 0%)
            } else {
                imageVector[outputPixel] = 0;
                imageVector[outputPixel+1] = 0;
                imageVector[outputPixel+2] = 0;
                imageVector[outputPixel+3] = 0;
            }
        }
    }
    
    //
    // handle faces
    //
    
    int faceBeginX = 0;
    int faceBeginY = 0;
    int faceWidth = 0;
    int faceHeight = 0;
    int faceBoundLoc = 0;
    for (int i = 0; i < faceCount; ++i) {
        faceBeginX = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        faceBeginY = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        faceWidth = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        faceHeight = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        int xLoopBegin = faceBeginX;
        int yLoopBegin = imageHeight - (faceBeginY + faceHeight);
        int xLoopEnd = (faceBeginX + faceWidth);
        int yLoopEnd = imageHeight - faceBeginY;
        
        if (xLoopBegin > 20) {
            xLoopBegin -= 20;
        }
        if (yLoopBegin > 20) {
            yLoopBegin -= 20;
        }
        if (xLoopEnd < (imageWidth - 20)) {
            xLoopEnd += 20;
        }
        if (yLoopEnd < (imageHeight - 20)) {
            yLoopEnd += 20;
        }
        
        for (int j = yLoopBegin; j < yLoopEnd; ++j) {
            for (int k = xLoopBegin; k < xLoopEnd; ++k) {
                currentPixel = (j * imageWidth) + k;
                outputPixel = currentPixel * imageDepth;
                imageVector[outputPixel+3] = 0;
            }
        }
    }
    
    //
    // fudge the redactions
    //
    
    fudge(imageVector, imageWidth, imageHeight, imageDepth);
    fudge(imageVector, imageWidth, imageHeight, imageDepth);
    fudge(imageVector, imageWidth, imageHeight, imageDepth);
    
    free(workingImageH);
}

void mergeImages(unsigned char *imageVector, unsigned char *pixelatePixels, int imgWidth, int imgHeight, int imgDepth, int faceCount, int *faceBounds)
{
    int outputPixel = 0;
    int currentPixel = 0;
    // fill initial data structures
    for (int j = 0; j < imgHeight; ++j) {
        for (int i = 0; i < imgWidth; ++i) {
            currentPixel = (j * imgWidth) + i;
            outputPixel = currentPixel * imgDepth;
            
            imageVector[outputPixel] = imageVector[outputPixel];
            imageVector[outputPixel+1] = imageVector[outputPixel+1];
            imageVector[outputPixel+2] = imageVector[outputPixel+2];
            imageVector[outputPixel+3] = imageVector[outputPixel+3];
        }
    }
    
    int faceBeginX = 0;
    int faceBeginY = 0;
    int faceWidth = 0;
    int faceHeight = 0;
    int faceBoundLoc = 0;
    for (int i = 0; i < faceCount; ++i) {
        faceBeginX = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        faceBeginY = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        faceWidth = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        faceHeight = faceBounds[faceBoundLoc];
        ++faceBoundLoc;
        
        int xLoopBegin = faceBeginX;
        int yLoopBegin = imgHeight - (faceBeginY + faceHeight);
        int xLoopEnd = (faceBeginX + faceWidth);
        int yLoopEnd = imgHeight - faceBeginY;
        
        if (xLoopBegin > 20) {
            xLoopBegin -= 20;
        }
        if (yLoopBegin > 20) {
            yLoopBegin -= 20;
        }
        if (xLoopEnd < (imgWidth - 20)) {
            xLoopEnd += 20;
        }
        if (yLoopEnd < (imgHeight - 20)) {
            yLoopEnd += 20;
        }
        
        for (int j = yLoopBegin; j < yLoopEnd; ++j) {
            for (int k = xLoopBegin; k < xLoopEnd; ++k) {
                currentPixel = (j * imgWidth) + k;
                outputPixel = currentPixel * imgDepth;
                
                imageVector[outputPixel] = pixelatePixels[outputPixel];
                imageVector[outputPixel+1] = pixelatePixels[outputPixel+1];
                imageVector[outputPixel+2] = pixelatePixels[outputPixel+2];
                imageVector[outputPixel+3] = pixelatePixels[outputPixel+3];
            }
        }
    }
}
