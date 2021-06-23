//
//  SVTKGestureHandler.h
//  SVTKViewer
//
//  Created by Max Smolens on 6/20/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@class SVTKView;

@interface SVTKGestureHandler : NSObject<UIGestureRecognizerDelegate>

- (id)initWithVtkView:(SVTKView*)svtkView;

- (void)onPinch:(UIPinchGestureRecognizer*)sender;
- (void)onRoll:(UIRotationGestureRecognizer*)sender;
- (void)onPan:(UIPanGestureRecognizer*)sender;

@end
