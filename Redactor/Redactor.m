//
//  Redactor.m
//  Redactor
//
//  Created by Christopher Stoll on 1/21/15.
//  Copyright (c) 2015 Christopher Stoll. All rights reserved.
//

#import "Redactor.h"
#import "SeamCarve.h"

@implementation Redactor

- (NSArray *)findFacesInImage:(UIImage *)image
{
    NSDictionary *options = @{ CIDetectorAccuracy : CIDetectorAccuracyHigh };
    CIDetector *detector = [CIDetector detectorOfType:CIDetectorTypeFace context:nil options:options];
    CIImage *coreimage = [CIImage imageWithCGImage:image.CGImage];
    NSArray *features = [detector featuresInImage:coreimage];
    return features;
}

//- (NSArray *)findQRCodesInImage:(UIImage *)image
//{
//    NSDictionary *options = @{ CIDetectorAccuracy : CIDetectorAccuracyHigh };
//    CIDetector *detector = [CIDetector detectorOfType:CIDetectorTypeQRCode context:nil options:options];
//    CIImage *coreimage = [CIImage imageWithCGImage:image.CGImage];
//    return [detector featuresInImage:coreimage];
//}

- (void)findObjectsInImage:(UIImage *)image
{
    self.faces = [self findFacesInImage:image];
    //self.codes = [self findQRCodesInImage:image];
}

- (void)processImage:(UIImage *)image withCallback:(NSString *)callBack
{
    CGImageRef imgRef = image.CGImage;
    NSUInteger imgWidth = CGImageGetWidth(imgRef);
    NSUInteger imgHeight = CGImageGetHeight(imgRef);
    
    // TODO: this shouldn't be hard-coded
    NSUInteger bytesPerPixel = 4; //CGImageGetBitsPerPixel(self.image.CGImage) / 16;
    NSUInteger bitsPerComponent = CGImageGetBitsPerComponent(image.CGImage); // 8;
    NSUInteger imgPixelCount = imgWidth * imgHeight;
    NSUInteger imgByteCount = imgPixelCount * bytesPerPixel;
    NSUInteger bytesPerRow = bytesPerPixel * imgWidth;
    
    // char not int -- to get each channel instead of the entire pixel
    unsigned char *rawPixels = (unsigned char*)calloc(imgByteCount, sizeof(unsigned char));
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(rawPixels, imgWidth, imgHeight,
                                                 bitsPerComponent, bytesPerRow, colorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);
    
    if (context) {
        CGContextDrawImage(context, CGRectMake(0, 0, imgWidth, imgHeight), imgRef);
        CGContextRelease(context);
        //imgRef = nil;
        // This causes zombies which when released result in bad access errors
        //CGImageRelease(imgRef);
    } else {
        // There is a problem with the context, so we will not be able to process anything
        // The most likely cause is that there was no image data provided
        // TODO: imporve error handling (below also)
        return;
    }
    
    // check if the image contains any faces
    [self findObjectsInImage:image];
    int faceCount = (int)self.faces.count;
    int *faceCoordinates = (int*)calloc((faceCount * 4), sizeof(int));
    // build c data structures for face information
    if (faceCount > 0) {
        int faceCoordCount = 0;
        for (CIFaceFeature *faceFeature in self.faces) {
            faceCoordinates[faceCoordCount] = (int)faceFeature.bounds.origin.x;
            ++faceCoordCount;
            
            faceCoordinates[faceCoordCount] = (int)faceFeature.bounds.origin.y;
            ++faceCoordCount;
            
            faceCoordinates[faceCoordCount] = (int)faceFeature.bounds.size.width;
            ++faceCoordCount;
            
            faceCoordinates[faceCoordCount] = (int)faceFeature.bounds.size.height;
            ++faceCoordCount;
        }
    }
    
    seamCarve(rawPixels, (int)imgWidth, (int)imgHeight, (int)bytesPerPixel, faceCount, faceCoordinates);
    
    unsigned char *pixelatePixels = (unsigned char*)calloc(imgByteCount, sizeof(unsigned char));
    CGColorSpaceRef pixelateColorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef pixelateContext = CGBitmapContextCreate(pixelatePixels, imgWidth, imgHeight,
                                                 bitsPerComponent, bytesPerRow, pixelateColorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(pixelateColorSpace);
    UIImage *pixelateMask = [self pixelate:image];
    CGImageRef pixelateImgRef = pixelateMask.CGImage;
    if (pixelateContext) {
        CGContextDrawImage(pixelateContext, CGRectMake(0, 0, imgWidth, imgHeight), pixelateImgRef);
        CGContextRelease(pixelateContext);
    }
    
    pixelateImgRef = nil;
    pixelateMask = nil;
    
    int redactMode = [[[NSUserDefaults standardUserDefaults] objectForKey:@"redactMode"] intValue];
    mergeImages(rawPixels, pixelatePixels, (int)imgWidth, (int)imgHeight, (int)bytesPerPixel, faceCount, faceCoordinates, redactMode);
    free(pixelatePixels);
    free(faceCoordinates);
    
    CGColorSpaceRef newColorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef newContext = CGBitmapContextCreate(rawPixels, imgWidth, imgHeight,
                                                    bitsPerComponent, bytesPerRow, newColorSpace,
                                                    kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(newColorSpace);
    
    if (newContext) {
        CGImageRef newImgRef = CGBitmapContextCreateImage(newContext);
        
        UIImage *redactMask = [UIImage imageWithCGImage:newImgRef];
        if (callBack) {
            [[NSNotificationCenter defaultCenter] postNotificationName:callBack object:redactMask];
        }
        
        CGContextRelease(newContext);
        CGImageRelease(newImgRef);
        free(rawPixels);
    }
}

- (UIImage *)pixelate:(UIImage *)image
{
    // CIPixellate
    // CIColorInvert
    // CIMotionBlur
    CIFilter *gaussianBlurFilter = [CIFilter filterWithName:@"CIPixellate"];
    [gaussianBlurFilter setDefaults];
    [gaussianBlurFilter setValue:[CIImage imageWithCGImage:[image CGImage]] forKey:@"inputImage"];
    float pixelSize = [[[NSUserDefaults standardUserDefaults] objectForKey:@"pixelSize"] floatValue];
    [gaussianBlurFilter setValue:[NSNumber numberWithFloat:pixelSize] forKey:@"inputScale"];
    
    //        CIFilter *gaussianBlurFilter = [CIFilter filterWithName: @"CIGaussianBlur"];
    //        [gaussianBlurFilter setDefaults];
    //        [gaussianBlurFilter setValue:[CIImage imageWithCGImage:[self.redactMask CGImage]] forKey:kCIInputImageKey];
    //        [gaussianBlurFilter setValue:[NSNumber numberWithFloat:1] forKey:@"inputRadius"];
    
    CIImage *outputImage = [gaussianBlurFilter outputImage];
    CIContext *context   = [CIContext contextWithOptions:nil];
    CGRect rect          = [outputImage extent];
    
    // these three lines ensure that the final image is the same size
    
    rect.origin.x        += (rect.size.width  - image.size.width ) / 2;
    rect.origin.y        += (rect.size.height - image.size.height) / 2;
    rect.size            = image.size;
    
    CGImageRef cgimg     = [context createCGImage:outputImage fromRect:rect];
    UIImage *resultImage = [UIImage imageWithCGImage:cgimg];
    CGImageRelease(cgimg);
    
    return resultImage;
}

@end
