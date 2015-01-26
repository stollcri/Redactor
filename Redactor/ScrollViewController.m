//
//  ScrollViewController.m
//  Redactor
//
//  Created by Christopher Stoll on 1/25/15.
//  Copyright (c) 2015 Christopher Stoll. All rights reserved.
//

#import "ScrollViewController.h"

@implementation ScrollViewController

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ((touches.count < 2) && !self.dragging) {
        [self.nextResponder touchesBegan:touches withEvent:event];
    } else {
        [super touchesBegan:touches withEvent:event];
    }
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ((touches.count < 2) && !self.dragging) {
        // Need the two "nextResponder" references, why?
        [[self.nextResponder nextResponder] touchesMoved:touches withEvent:event];
    } else {
        [super touchesMoved:touches withEvent:event];
    }
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ((touches.count < 2) && !self.dragging) {
        // Need the two "nextResponder" references, why?
        [[self.nextResponder nextResponder] touchesEnded:touches withEvent:event];
    } else {
        [super touchesEnded:touches withEvent:event];
    }
}

@end
