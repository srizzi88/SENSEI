/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCocoaGLView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCocoaGLView
 * @brief   Cocoa OpenGL rendering context
 *
 *
 * This class is a subclass of Cocoa's NSView; it uses Objective-C++.
 * This class overrides several NSView methods.
 * To provide the usual SVTK keyboard user interface, it overrides the
 * following methods: acceptsFirstResponder, keyDown:,
 * keyUp:, and flagsChanged:
 * To provide the usual SVTK mouse user interface, it overrides the
 * following methods: mouseMoved:, mouseEntered:,
 * mouseExited: scrollWheel:, mouseDown:, rightMouseDown:,
 * otherMouseDown:, mouseDragged:, rightMouseDragged:, otherMouseDragged:,
 * and updateTrackingAreas.
 * To provide file dropping support, it implements the following methods:
 * draggingEntered: and performDragOperation:.
 * To be able to render and draw onscreen, it overrides drawRect:.
 *
 * Compatibility notes:
 * - this class was previously a subclass of NSOpenGLView,
 * but starting with SVTK 5.0 is now a subclass of NSView.
 * - starting with SVTK 6.3 this class overrides the more modern
 * updateTrackingAreas instead of resetCursorRects.
 * - starting with SVTK 8.1 this class properly supports Retina
 * displays and implements viewWillMoveToWindow: and
 * viewDidChangeBackingProperties to do so.
 * - starting with SVTK 9.0 this class now also overrides initWithCoder:.
 * It also implements draggingEntered: and performDragOperation: and thus
 * declares conformance to the NSDraggingDestination protocol.
 *
 * @sa
 * svtkCocoaRenderWindow svtkCocoaRenderWindowInteractor
 */

#ifndef svtkCocoaGLView_h
#define svtkCocoaGLView_h
#ifndef __SVTK_WRAP__
#ifndef SVTK_WRAPPING_CXX

#import "svtkRenderingOpenGL2Module.h" // For export macro
#import <Cocoa/Cocoa.h>

// Note: This file should be includable by both pure Objective-C and Objective-C++ source files.
// To achieve this, we use the neat technique below:
#ifdef __cplusplus
// Forward declarations
class svtkCocoaRenderWindow;
class svtkCocoaRenderWindowInteractor;

// Type declarations
typedef svtkCocoaRenderWindow* svtkCocoaRenderWindowRef;
typedef svtkCocoaRenderWindowInteractor* svtkCocoaRenderWindowInteractorRef;
#else
// Type declarations
typedef void* svtkCocoaRenderWindowRef;
typedef void* svtkCocoaRenderWindowInteractorRef;
#endif

SVTKRENDERINGOPENGL2_EXPORT
@interface svtkCocoaGLView : NSView<NSDraggingDestination>
{
@private
  svtkCocoaRenderWindowRef _mySVTKRenderWindow;
  NSTrackingArea* _rolloverTrackingArea;
}

- (svtkCocoaRenderWindowRef)getSVTKRenderWindow;
- (void)setSVTKRenderWindow:(svtkCocoaRenderWindowRef)theSVTKRenderWindow;

- (svtkCocoaRenderWindowInteractorRef)getInteractor;

@end

#endif
#endif
#endif /* svtkCocoaGLView_h */
// SVTK-HeaderTest-Exclude: svtkCocoaGLView.h
