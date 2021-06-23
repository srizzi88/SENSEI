/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import <GLKit/GLKit.h>
#import <UIKit/UIKit.h>

// Note: This file should be includable by both pure Objective-C and Objective-C++ source files.
// To achieve this, we use the neat technique below:
#ifdef __cplusplus
// Forward declarations
class svtkIOSRenderWindow;
class svtkIOSRenderWindowInteractor;

// Type declarations
typedef svtkIOSRenderWindow* svtkIOSRenderWindowRef;
typedef svtkIOSRenderWindowInteractor* svtkIOSRenderWindowInteractorRef;
#else
// Type declarations
typedef void* svtkIOSRenderWindowRef;
typedef void* svtkIOSRenderWindowInteractorRef;
#endif

@interface MyGLKViewController : GLKViewController
{
@private
  svtkIOSRenderWindowRef _mySVTKRenderWindow;
}

@property (nonatomic, strong) UIWindow* window;

- (svtkIOSRenderWindowRef)getSVTKRenderWindow;
- (void)setSVTKRenderWindow:(svtkIOSRenderWindowRef)theSVTKRenderWindow;

- (svtkIOSRenderWindowInteractorRef)getInteractor;

- (void)initializeParametricObjects;
- (void)setupPipeline;

@end
