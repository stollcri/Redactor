//
//  Redactor.h
//  Redactor
//
//  Created by Christopher Stoll on 1/21/15.
//  Copyright (c) 2015 Christopher Stoll. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface Redactor : NSObject

@property (strong, nonatomic) NSArray *faces;
@property (strong, nonatomic) NSArray *codes;

- (void)processImage:(UIImage *)image withCallback:(NSString *)callBack;
- (UIImage *)pixelate:(UIImage *)image;

@end
