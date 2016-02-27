//
//  OptionViewController.m
//  Redactor
//
//  Created by Christopher Stoll on 2/23/16.
//  Copyright © 2016 Christopher Stoll. All rights reserved.
//

#import "OptionViewController.h"

@interface OptionViewController ()

@end

@implementation OptionViewController

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    if ([[[NSUserDefaults standardUserDefaults] objectForKey:@"redactMode"] intValue] == 2) {
        self.redactMode.selectedSegmentIndex = 2;
    } else if ([[[NSUserDefaults standardUserDefaults] objectForKey:@"redactMode"] intValue] == 1) {
        self.redactMode.selectedSegmentIndex = 1;
    } else {
        self.redactMode.selectedSegmentIndex = 0;
    }
    [self.pixelSize setValue:[[[NSUserDefaults standardUserDefaults] objectForKey:@"pixelSize"] floatValue]];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

- (IBAction)saveTouch:(id)sender {
    NSNumber *tempRedactMode = [[NSNumber alloc] initWithInteger: self.redactMode.selectedSegmentIndex];
    [[NSUserDefaults standardUserDefaults] setObject:tempRedactMode forKey:@"redactMode"];
    
    NSNumber *tempPixelSize = [[NSNumber alloc] initWithFloat:self.pixelSize.value];
    [[NSUserDefaults standardUserDefaults] setObject:tempPixelSize forKey:@"pixelSize"];
    
    [self dismissViewControllerAnimated:YES completion:nil];
}

@end
