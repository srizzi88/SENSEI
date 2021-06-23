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

// Forward declarations
class svtkIOSRenderWindow;
class svtkRenderer;
class svtkIOSRenderWindowInteractor;
class svtkPlaneWidget;
class svtkPolyDataMapper;
class svtkProbeFilter;
class svtkTPWCallback;

@interface SVTKViewController : GLKViewController
{
@private
  svtkIOSRenderWindow* RenderWindow;
  svtkPlaneWidget* PlaneWidget;
  svtkRenderer* Renderer;
  svtkProbeFilter* Probe;
  svtkPolyDataMapper* OutlineMapper;
  svtkPolyDataMapper* ProbeMapper;
  svtkTPWCallback* PlaneCallback;
}

@property (nonatomic, strong) UIWindow* window;

- (void)setProbeEnabled:(bool)val;
- (bool)getProbeEnabled;

- (void)setNewDataFile:(NSURL*)url;

- (svtkIOSRenderWindowInteractor*)getInteractor;

@end
