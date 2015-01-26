//
//  ViewController.h
//  Redactor
//
//  Created by Christopher Stoll on 1/21/15.
//  Copyright (c) 2015 Christopher Stoll. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, PaintMode) {
    PaintModeNone,
    PaintModeBlack,
    PaintModePending
};

@interface ViewController : UIViewController <UINavigationControllerDelegate, UIImagePickerControllerDelegate, UIScrollViewDelegate>

@property (weak, nonatomic) IBOutlet UIScrollView *scrollView;
@property (weak, nonatomic) IBOutlet UIImageView *imageView;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *openButton;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *modeButton;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *blurButton;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *blackButton;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *whiteButton;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *saveButton;
@property (weak, nonatomic) IBOutlet UIView *popoverAnchor;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *activityIndicator;

- (IBAction)doOpen:(id)sender;
- (IBAction)changeMode:(id)sender;
- (IBAction)doBlur:(id)sender;
- (IBAction)doBlack:(id)sender;
- (IBAction)doWhite:(id)sender;
- (IBAction)doSave:(id)sender;

@end

