//
//  ViewController.m
//  Redactor
//
//  Created by Christopher Stoll on 1/21/15.
//  Copyright (c) 2015 Christopher Stoll. All rights reserved.
//

#import <MobileCoreServices/MobileCoreServices.h>
#import "ViewController.h"
#import "Redactor.h"

#define REDACT_MODE_BLACK @0
#define REDACT_MODE_PIXELATE @1
#define PIXEL_SIZE_DEFAULT @5

#define MAXIMUM_IMG_SIZE 1024
#define PAINT_BRUSH_SIZE 20.0
#define PAINT_BRUSH_ALPHA 1.0
#define PAINT_BRUSH_R 0.0
#define PAINT_BRUSH_G 0.0
#define PAINT_BRUSH_B 0.0
#define UNDO_LIMIT 1024

@interface ViewController ()

@property Redactor *redactor;

@property UIImageView *redactImageView;
@property UIImageView *paintImageView;

@property CGFloat brushSize;
@property BOOL mouseSwiped;
@property CGPoint lastPoint;
@property PaintMode paintMode;

@property NSMutableArray *undoSequence; // TODO: use undoManager?

@end

@implementation ViewController

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"redactMode"] == nil) {
        [[NSUserDefaults standardUserDefaults] setObject:REDACT_MODE_PIXELATE forKey:@"redactMode"];
    }
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"pixelSize"] == nil) {
        [[NSUserDefaults standardUserDefaults] setObject:PIXEL_SIZE_DEFAULT forKey:@"pixelSize"];
    }
    
    // settings for story board items
    //self.scrollView.panGestureRecognizer.minimumNumberOfTouches = 2;
    self.scrollView.userInteractionEnabled = NO;
    self.scrollView.delegate = self;
    
    // settings for private class properties
    self.redactor = [[Redactor alloc] init];
    self.brushSize = PAINT_BRUSH_SIZE - (2 * self.scrollView.zoomScale);
    self.paintMode = PaintModePending;
    self.undoSequence = [[NSMutableArray alloc] init];

    
    // notifications
    NSNotificationCenter *defaultNotifCenter = [NSNotificationCenter defaultCenter];
    [defaultNotifCenter addObserver:self selector:@selector(deviceOrientationDidChange:) name:UIDeviceOrientationDidChangeNotification object:nil];
    [defaultNotifCenter addObserver:self selector:@selector(loadRedactMasks:) name:@"org.christopherstoll.squared.maskingComplete" object:nil];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Utilities

- (CGRect)getDisplaySizeOfImageView:(UIImageView *)imageView
{
    CGRect results = CGRectZero;
    CGSize imageSize = imageView.image.size;
    CGSize frameSize = imageView.frame.size;
    //if ((imageSize.width < frameSize.width) && (imageSize.height < frameSize.height)) {
    //    results.size = imageSize;
    //} else {
    CGFloat widthRatio = imageSize.width / frameSize.width;
    CGFloat heightRatio = imageSize.height / frameSize.height;
    CGFloat maxRatio = MAX(widthRatio, heightRatio);
    
    results.size.width = roundf(imageSize.width / maxRatio);
    // deal with odd widths
    if ((results.size.width / 2) != roundf(results.size.width / 2)) {
        results.size.width += 1;
    }
    
    results.size.height = roundf(imageSize.height / maxRatio);
    // deal with odd heights
    if ((results.size.height / 2) != roundf(results.size.height / 2)) {
        results.size.height += 1;
    }
    //}
    
    results.origin.x = imageView.center.x - roundf(results.size.width / 2);
    results.origin.y = imageView.center.y - roundf(results.size.height / 2);
    
    return results;
}

- (void)addUndoData:(CGRect)rect
{
    if (self.undoSequence.count >= UNDO_LIMIT) {
        [self.undoSequence removeObjectAtIndex:0];
    }
    [self.undoSequence addObject:[NSValue valueWithCGRect:rect]];
}

- (UIImage*)imageFromView:(UIView *)view
{
    // Create a graphics context with the target size
    CGSize imageSize = view.frame.size;// [view bounds].size;
    UIGraphicsBeginImageContextWithOptions(imageSize, NO, 0);
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    // -renderInContext: renders in the coordinate space of the layer,
    // so we must first apply the layer's geometry to the graphics context
    CGContextSaveGState(context);
    // Center the context around the view's anchor point
    CGContextTranslateCTM(context, [view center].x, [view center].y);
    // Apply the view's transform about the anchor point
    CGContextConcatCTM(context, [view transform]);
    // Offset by the portion of the bounds left of and above the anchor point
    CGContextTranslateCTM(context,
                          -[view bounds].size.width * [[view layer] anchorPoint].x,
                          -[view bounds].size.height * [[view layer] anchorPoint].y);
    
    // Render the layer hierarchy to the current context
    [[view layer] renderInContext:context];
    
    // Restore the context
    CGContextRestoreGState(context);
    
    // Retrieve the screenshot image
    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    
    UIGraphicsEndImageContext();
    
    return image;
}

#pragma mark - UIImagePickerControllerDelegate methods

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
    UIImage *img = [info objectForKey:UIImagePickerControllerEditedImage];
    if (!img) {
        img = [info objectForKey:UIImagePickerControllerOriginalImage];
    }
    
    if (img) {
        [self.imageView setAlpha:0.4];
        [self.imageView setImage:img];
        
        self.scrollView.contentSize = CGSizeMake(self.imageView.frame.size.width, self.imageView.frame.size.height);
        self.scrollView.clipsToBounds = YES;
        self.scrollView.contentSize = self.imageView.bounds.size;
        
        self.scrollView.minimumZoomScale = 1.0;
        self.scrollView.maximumZoomScale = 4.0;
        self.scrollView.zoomScale = 1.0;
        
        [self.redactImageView removeFromSuperview];
        // launch redact mask algorithm on a background thread
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            // make sure choosen image is less than maximum size, to increase performance
            CGSize newSize;
            if ((img.size.height > MAXIMUM_IMG_SIZE) || (img.size.width > MAXIMUM_IMG_SIZE)) {
                int temp = 0.0;
                float newWidth = 0;
                float newHeight = 0;
                
                // determine new image dimensions
                if (img.size.height > img.size.width) {
                    temp = img.size.width * MAXIMUM_IMG_SIZE / img.size.height;
                    newWidth = temp;
                    newHeight = MAXIMUM_IMG_SIZE;
                } else {
                    temp = img.size.height * MAXIMUM_IMG_SIZE / img.size.width;
                    newWidth = MAXIMUM_IMG_SIZE;
                    newHeight = temp;
                }
                
                newSize = CGSizeMake(newWidth, newHeight);
                UIGraphicsBeginImageContext(newSize);
            } else {
                newSize = CGSizeMake(img.size.width, img.size.height);
                UIGraphicsBeginImageContext(newSize);
            }
            
            [img drawInRect:CGRectMake(0, 0, newSize.width, newSize.height)];
            UIImage* newImage = UIGraphicsGetImageFromCurrentImageContext();
            UIGraphicsEndImageContext();
            
            [self.redactor processImage:newImage withCallback:@"org.christopherstoll.squared.maskingComplete"];
        });
    }
    
    [self.activityIndicator startAnimating];
    [picker dismissViewControllerAnimated:YES completion:nil];
}

- (void)loadRedactMasks:(NSNotification *)notification
{
    // come to the foreground
    dispatch_async(dispatch_get_main_queue(), ^{
        UIImage *redactMask = [notification object];
        if (redactMask) {
            //[self.imageView setImage:redactMask];
            
            CGRect tempFrame = self.imageView.frame;//[self getDisplaySizeOfImageView:self.imageView];
            UIImageView *redactView = [[UIImageView alloc] initWithFrame:tempFrame];
            redactView.contentMode = UIViewContentModeScaleAspectFit;
            redactView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            redactView.autoresizesSubviews = YES;
            [redactView setImage:redactMask];
            
            UIImageView *paintView = [[UIImageView alloc] initWithFrame:tempFrame];
            paintView.contentMode = UIViewContentModeScaleAspectFit;
            // DEBUG ONLY -- TO SEE MASK RESULTS
            //[paintView setImage:redactMask];
            
            [redactView setMaskView:paintView];
            [self.imageView addSubview:redactView];
            // DEBUG ONLY -- TO SEE MASK RESULTS
            //[self.imageView setImage:redactMask];
            
            self.redactImageView = redactView;
            self.paintImageView = paintView;
            
            if (self.paintMode == PaintModePending) {
                self.paintMode = PaintModeBlack;
            }
            
            [self.activityIndicator stopAnimating];
            NSValue *animationDurationValue = @0.2;
            NSTimeInterval animationDuration;
            [animationDurationValue getValue:&animationDuration];
            [UIView beginAnimations:nil context:NULL];
            [UIView setAnimationDuration:animationDuration];
            self.imageView.alpha = 1.0;
            [UIView commitAnimations];
        }
    });
}

#pragma mark - UIScrollViewDelegate

- (UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView
{
    return self.imageView;
}

- (void)scrollViewDidZoom:(UIScrollView *)scrollView
{
    UIView *subView = self.imageView;
    CGFloat offsetX = (scrollView.bounds.size.width > scrollView.contentSize.width)?
    (scrollView.bounds.size.width - scrollView.contentSize.width) * 0.5 : 0.0;
    
    CGFloat offsetY = (scrollView.bounds.size.height > scrollView.contentSize.height)?
    (scrollView.bounds.size.height - scrollView.contentSize.height) * 0.5 : 0.0;
    
    subView.center = CGPointMake(scrollView.contentSize.width * 0.5 + offsetX,
                                 scrollView.contentSize.height * 0.5 + offsetY);
    
    self.brushSize = PAINT_BRUSH_SIZE - (2 * scrollView.zoomScale);
}

#pragma mark - UI Responders

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (self.paintMode == PaintModeBlack) {
        self.mouseSwiped = NO;
        UITouch *touch = [touches anyObject];
        self.lastPoint = [touch locationInView:self.paintImageView];
        
        if (touch.view != self.toolbar) {
            CGRect currentLine = CGRectMake(self.lastPoint.x, self.lastPoint.y, self.lastPoint.x, self.lastPoint.y);
            [self addUndoData:currentLine];
            [self.undoButton setEnabled:YES];
        }
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (self.paintMode == PaintModeBlack) {
        self.mouseSwiped = YES;
        UITouch *touch = [touches anyObject];
        CGPoint currentPoint = [touch locationInView:self.paintImageView];
        
        UIGraphicsBeginImageContext(self.paintImageView.frame.size);
        [self.paintImageView.image drawInRect:CGRectMake(0, 0, self.paintImageView.frame.size.width, self.paintImageView.frame.size.height)];
        CGContextMoveToPoint(UIGraphicsGetCurrentContext(), self.lastPoint.x, self.lastPoint.y);
        CGContextAddLineToPoint(UIGraphicsGetCurrentContext(), currentPoint.x, currentPoint.y);
        CGContextSetLineCap(UIGraphicsGetCurrentContext(), kCGLineCapRound);
        CGContextSetLineWidth(UIGraphicsGetCurrentContext(), self.brushSize);
        CGContextSetRGBStrokeColor(UIGraphicsGetCurrentContext(), PAINT_BRUSH_R, PAINT_BRUSH_G, PAINT_BRUSH_B, 1.0);
        CGContextSetBlendMode(UIGraphicsGetCurrentContext(), kCGBlendModeNormal);
        //CGContextSetShouldAntialias(UIGraphicsGetCurrentContext(), YES);
        CGContextStrokePath(UIGraphicsGetCurrentContext());
        self.paintImageView.image = UIGraphicsGetImageFromCurrentImageContext();
        [self.paintImageView setAlpha:PAINT_BRUSH_ALPHA];
        UIGraphicsEndImageContext();
        
        CGRect currentLine = CGRectMake(self.lastPoint.x, self.lastPoint.y, currentPoint.x, currentPoint.y);
        [self addUndoData:currentLine];
        
        self.lastPoint = currentPoint;
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (self.paintMode == PaintModeBlack) {
        if(!self.mouseSwiped) {
            UIGraphicsBeginImageContext(self.paintImageView.frame.size);
            [self.paintImageView.image drawInRect:CGRectMake(0, 0, self.paintImageView.frame.size.width, self.paintImageView.frame.size.height)];
            CGContextSetLineCap(UIGraphicsGetCurrentContext(), kCGLineCapRound);
            CGContextSetLineWidth(UIGraphicsGetCurrentContext(), self.brushSize);
            CGContextSetRGBStrokeColor(UIGraphicsGetCurrentContext(), PAINT_BRUSH_R, PAINT_BRUSH_G, PAINT_BRUSH_B, 1.0);
            //CGContextSetShouldAntialias(UIGraphicsGetCurrentContext(), YES);
            CGContextMoveToPoint(UIGraphicsGetCurrentContext(), self.lastPoint.x, self.lastPoint.y);
            CGContextAddLineToPoint(UIGraphicsGetCurrentContext(), self.lastPoint.x, self.lastPoint.y);
            CGContextStrokePath(UIGraphicsGetCurrentContext());
            CGContextFlush(UIGraphicsGetCurrentContext());
            self.paintImageView.image = UIGraphicsGetImageFromCurrentImageContext();
            UIGraphicsEndImageContext();
        }
    }
}

- (void)deviceOrientationDidChange:(NSNotification *)notification
{
    if (self.paintImageView) {
        [self.paintImageView setFrame:self.redactImageView.bounds];
    }
}

#pragma mark - IB Actions

- (IBAction)doOpen:(id)sender {
    UIImagePickerController *imagePicker = [[UIImagePickerController alloc] init];
    imagePicker.delegate = self;
    imagePicker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
    imagePicker.mediaTypes = @[(NSString *) kUTTypeImage];
    imagePicker.allowsEditing = NO;
    [self presentViewController:imagePicker animated:YES completion:nil];
}

- (IBAction)changeMode:(id)sender {
    if ((self.paintMode == PaintModeBlack) || (self.paintMode == PaintModePending)) {
        [self.scrollView setUserInteractionEnabled:YES];
        self.paintMode = PaintModeNone;
        [self.modeButton setImage:[UIImage imageNamed:@"Move"]];
    } else {
        [self.scrollView setUserInteractionEnabled:NO];
        self.paintMode = PaintModeBlack;
        [self.modeButton setImage:[UIImage imageNamed:@"Pencil"]];
    }
}

- (IBAction)undo:(id)sender {
    BOOL stopPointFound = NO;
    
    while (self.undoSequence && !stopPointFound) {
        CGRect currentUndo = [[self.undoSequence lastObject] CGRectValue];
        [self.undoSequence removeLastObject];
        
        CGPoint startPoint = CGPointMake(currentUndo.origin.x, currentUndo.origin.y);
        CGPoint endPoint = CGPointMake(currentUndo.size.width, currentUndo.size.height);
        
        if ((startPoint.x == endPoint.x) && (startPoint.y == endPoint.y)) {
            stopPointFound = YES;
        }
        
        UIGraphicsBeginImageContext(self.paintImageView.frame.size);
        [self.paintImageView.image drawInRect:CGRectMake(0, 0, self.paintImageView.frame.size.width, self.paintImageView.frame.size.height)];
        CGContextSetLineCap(UIGraphicsGetCurrentContext(), kCGLineCapRound);
        CGContextSetLineWidth(UIGraphicsGetCurrentContext(), (self.brushSize + 1));
        CGContextSetRGBStrokeColor(UIGraphicsGetCurrentContext(), PAINT_BRUSH_R, PAINT_BRUSH_G, PAINT_BRUSH_B, 1.0);
        CGContextSetBlendMode(UIGraphicsGetCurrentContext(), kCGBlendModeClear);
        CGContextMoveToPoint(UIGraphicsGetCurrentContext(), endPoint.x, endPoint.y);
        CGContextAddLineToPoint(UIGraphicsGetCurrentContext(), startPoint.x, startPoint.y);
        CGContextStrokePath(UIGraphicsGetCurrentContext());
        CGContextFlush(UIGraphicsGetCurrentContext());
        self.paintImageView.image = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
    }
    
    if (self.undoSequence.count <= 0) {
        [self.undoButton setEnabled:NO];
    }
}

- (IBAction)doSave:(id)sender {
    UIImage *imagetoshare = [self imageFromView:self.imageView];
    if (imagetoshare) {
        NSArray *activityItems = @[imagetoshare];
        UIActivityViewController *activityVC = [[UIActivityViewController alloc] initWithActivityItems:activityItems applicationActivities:nil];
        activityVC.excludedActivityTypes = @[UIActivityTypeAssignToContact, UIActivityTypePrint];
        activityVC.popoverPresentationController.sourceView = self.popoverAnchor;
        [self presentViewController:activityVC animated:TRUE completion:nil];
    }
}

@end
