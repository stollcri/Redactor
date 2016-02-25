//
//  OptionViewController.h
//  Redactor
//
//  Created by Christopher Stoll on 2/23/16.
//  Copyright © 2016 Christopher Stoll. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface OptionViewController : UIViewController

@property (weak, nonatomic) IBOutlet UISegmentedControl *redactMode;
@property (weak, nonatomic) IBOutlet UISlider *pixelSize;

- (IBAction)saveTouch:(id)sender;

@end
