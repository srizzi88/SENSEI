//
//  SVTKView.h
//
//  Created by Alexis Girault on 4/3/17.
//

#import <GLKit/GLKit.h>

class svtkIOSRenderWindowInteractor;
class svtkRenderWindow;

@interface SVTKView : GLKView

- (void)displayCoordinates:(int*)coordinates ofTouch:(CGPoint)touchPoint;
- (void)normalizedCoordinates:(double*)coordinates ofTouch:(CGPoint)touch;

@property (assign, readonly) svtkRenderWindow* renderWindow;
@property (assign, readonly) svtkIOSRenderWindowInteractor* interactor;

@end
