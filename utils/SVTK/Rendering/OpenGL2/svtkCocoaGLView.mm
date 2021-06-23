/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCocoaGLView.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import "svtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs
#import <Cocoa/Cocoa.h>

#import "svtkCocoaGLView.h"
#import "svtkCocoaRenderWindow.h"
#import "svtkCocoaRenderWindowInteractor.h"
#import "svtkCommand.h"
#import "svtkNew.h"
#import "svtkStringArray.h"

//----------------------------------------------------------------------------
// Private
@interface svtkCocoaGLView ()
@property (readwrite, retain, nonatomic) NSTrackingArea* rolloverTrackingArea;
@end

@implementation svtkCocoaGLView

//----------------------------------------------------------------------------
// Private
- (void)emptyMethod:(id)sender
{
  (void)sender;
}

//----------------------------------------------------------------------------
@synthesize rolloverTrackingArea = _rolloverTrackingArea;

//------------------------------------------------------------------------------
- (void)commonInit
{
  // Force Cocoa into "multi threaded mode" because SVTK spawns pthreads.
  // Apple's docs say: "If you intend to use Cocoa calls, you must force
  // Cocoa into its multithreaded mode before detaching any POSIX threads.
  // To do this, simply detach an NSThread and have it promptly exit.
  // This is enough to ensure that the locks needed by the Cocoa
  // frameworks are put in place"
  if ([NSThread isMultiThreaded] == NO)
  {
    [NSThread detachNewThreadSelector:@selector(emptyMethod:) toTarget:self withObject:nil];
  }
  [self registerForDraggedTypes:@[ (NSString*)kUTTypeFileURL ]];
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
// designated initializer
- (id)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self)
  {
    [self commonInit];
  }
  return self;
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
// designated initializer
- (/*nullable*/ id)initWithCoder:(NSCoder*)decoder
{
  self = [super initWithCoder:decoder];
  if (self)
  {
    [self commonInit];
  }
  return self;
}

//----------------------------------------------------------------------------
#if !SVTK_OBJC_IS_ARC
- (void)dealloc
{
  [super dealloc];
  [_rolloverTrackingArea release];
}
#endif

//----------------------------------------------------------------------------
- (svtkCocoaRenderWindow*)getSVTKRenderWindow
{
  return _mySVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (void)setSVTKRenderWindow:(svtkCocoaRenderWindow*)theSVTKRenderWindow
{
  _mySVTKRenderWindow = theSVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (svtkCocoaRenderWindowInteractor*)getInteractor
{
  if (_mySVTKRenderWindow)
  {
    return (svtkCocoaRenderWindowInteractor*)_mySVTKRenderWindow->GetInteractor();
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)drawRect:(NSRect)theRect
{
  (void)theRect;

  if (_mySVTKRenderWindow && _mySVTKRenderWindow->GetMapped())
  {
    _mySVTKRenderWindow->Render();
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)updateTrackingAreas
{
  // clear out the old tracking area
  NSTrackingArea* trackingArea = [self rolloverTrackingArea];
  if (trackingArea)
  {
    [self removeTrackingArea:trackingArea];
  }

  // create a new tracking area
  NSRect rect = [self visibleRect];
  NSTrackingAreaOptions opts =
    (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways);
  trackingArea = [[NSTrackingArea alloc] initWithRect:rect options:opts owner:self userInfo:nil];
  [self addTrackingArea:trackingArea];
  [self setRolloverTrackingArea:trackingArea];
#if !SVTK_OBJC_IS_ARC
  [trackingArea release];
#endif

  [super updateTrackingAreas];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (BOOL)acceptsFirstResponder
{
  return YES;
}

//----------------------------------------------------------------------------
// For generating keysyms that are compatible with other SVTK interactors
static const char* svtkMacCharCodeToKeySymTable[128] = { nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, "space", "exclam", "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quoteright", "parenleft", "parenright", "asterisk", "plus",
  "comma", "minus", "period", "slash", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "colon",
  "semicolon", "less", "equal", "greater", "question", "at", "A", "B", "C", "D", "E", "F", "G", "H",
  "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
  "bracketleft", "backslash", "bracketright", "asciicircum", "underscore", "quoteleft", "a", "b",
  "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u",
  "v", "w", "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Delete" };

//----------------------------------------------------------------------------
// For generating keysyms that are compatible with other SVTK interactors
static const char* svtkMacKeyCodeToKeySymTable[128] = { nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "Return",
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  "Tab", nullptr, nullptr, "Backspace", nullptr, "Escape", nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "period", nullptr, "asterisk",
  nullptr, "plus", nullptr, "Clear", nullptr, nullptr, nullptr, "slash", "KP_Enter", nullptr,
  "minus", nullptr, nullptr, nullptr, "KP_0", "KP_1", "KP_2", "KP_3", "KP_4", "KP_5", "KP_6",
  "KP_7", nullptr, "KP_8", "KP_9", nullptr, nullptr, nullptr, "F5", "F6", "F7", "F3", "F8", nullptr,
  nullptr, nullptr, nullptr, "Snapshot", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, "Help", "Home", "Prior", "Delete", "F4", "End", "F2", "Next", "F1", "Left",
  "Right", "Down", "Up", nullptr };

//----------------------------------------------------------------------------
// Convert a Cocoa key event into a SVTK key event
- (void)invokeSVTKKeyEvent:(unsigned long)theEventId cocoaEvent:(NSEvent*)theEvent
{
  svtkCocoaRenderWindowInteractor* interactor = [self getInteractor];
  svtkCocoaRenderWindow* renWin = svtkCocoaRenderWindow::SafeDownCast([self getSVTKRenderWindow]);

  if (!interactor || !renWin)
  {
    return;
  }

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner.  Since this is NOT a mouse event, we can not use
  // locationInWindow.  Instead we get the mouse location at this instant,
  // which may not be the exact location of the mouse at the time of the
  // keypress, but should be quite close.  There seems to be no better way.
  // And, yes, svtk does sometimes need the mouse location even for key
  // events, example: pressing 'p' to pick the actor under the mouse.
  // Also note that 'mouseLoc' may have nonsense values if a key is pressed
  // while the mouse in not actually in the svtk view but the view is
  // first responder.
  NSPoint windowLoc = [[self window] mouseLocationOutsideOfEventStream];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];

  NSUInteger flags = [theEvent modifierFlags];
  int shiftDown = ((flags & NSEventModifierFlagShift) != 0);
  int controlDown = ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0);
  int altDown = ((flags & NSEventModifierFlagOption) != 0);

  unsigned char charCode = '\0';
  const char* keySym = nullptr;

  NSEventType type = [theEvent type];
  BOOL isPress = (type == NSEventTypeKeyDown);

  if (type == NSEventTypeKeyUp || type == NSEventTypeKeyDown)
  {
    // Try to get the characters associated with the key event as an ASCII string.
    const char* keyedChars = [[theEvent characters] cStringUsingEncoding:NSASCIIStringEncoding];
    if (keyedChars)
    {
      charCode = static_cast<unsigned char>(keyedChars[0]);
    }
    // Get the virtual key code and convert it to a keysym as best we can.
    unsigned short macKeyCode = [theEvent keyCode];
    if (macKeyCode < 128)
    {
      keySym = svtkMacKeyCodeToKeySymTable[macKeyCode];
    }
    if (keySym == nullptr && charCode < 128)
    {
      keySym = svtkMacCharCodeToKeySymTable[charCode];
    }
  }
  else if (type == NSEventTypeFlagsChanged)
  {
    // Check to see what modifier flag changed.
    if (controlDown != interactor->GetControlKey())
    {
      keySym = "Control_L";
      isPress = (controlDown != 0);
    }
    else if (shiftDown != interactor->GetShiftKey())
    {
      keySym = "Shift_L";
      isPress = (shiftDown != 0);
    }
    else if (altDown != interactor->GetAltKey())
    {
      keySym = "Alt_L";
      isPress = (altDown != 0);
    }
    else
    {
      return;
    }

    theEventId = (isPress ? svtkCommand::KeyPressEvent : svtkCommand::KeyReleaseEvent);
  }
  else // No info from which to generate a SVTK key event!
  {
    return;
  }

  if (keySym == nullptr)
  {
    keySym = "None";
  }

  interactor->SetEventInformation(static_cast<int>(backingLoc.x), static_cast<int>(backingLoc.y),
    controlDown, shiftDown, charCode, 1, keySym);
  interactor->SetAltKey(altDown);

  interactor->InvokeEvent(theEventId, nullptr);
  if (isPress && charCode != '\0')
  {
    interactor->InvokeEvent(svtkCommand::CharEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
// Convert a Cocoa motion event into a SVTK button event
- (void)invokeSVTKMoveEvent:(unsigned long)theEventId cocoaEvent:(NSEvent*)theEvent
{
  svtkCocoaRenderWindowInteractor* interactor = [self getInteractor];
  svtkCocoaRenderWindow* renWin = svtkCocoaRenderWindow::SafeDownCast([self getSVTKRenderWindow]);

  if (!interactor || !renWin)
  {
    return;
  }

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouse event, we can use locationInWindow.
  NSPoint windowLoc = [theEvent locationInWindow];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];

  NSUInteger flags = [theEvent modifierFlags];
  int shiftDown = ((flags & NSEventModifierFlagShift) != 0);
  int controlDown = ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0);
  int altDown = ((flags & NSEventModifierFlagOption) != 0);

  interactor->SetEventInformation(
    static_cast<int>(backingLoc.x), static_cast<int>(backingLoc.y), controlDown, shiftDown);
  interactor->SetAltKey(altDown);
  interactor->InvokeEvent(theEventId, nullptr);
}

//----------------------------------------------------------------------------
// Convert a Cocoa motion event into a SVTK button event
- (void)invokeSVTKButtonEvent:(unsigned long)theEventId cocoaEvent:(NSEvent*)theEvent
{
  svtkCocoaRenderWindowInteractor* interactor = [self getInteractor];
  svtkCocoaRenderWindow* renWin = svtkCocoaRenderWindow::SafeDownCast([self getSVTKRenderWindow]);

  if (!interactor || !renWin)
  {
    return;
  }

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouse event, we can use locationInWindow.
  NSPoint windowLoc = [theEvent locationInWindow];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];

  int clickCount = static_cast<int>([theEvent clickCount]);
  int repeatCount = ((clickCount > 1) ? clickCount - 1 : 0);

  NSUInteger flags = [theEvent modifierFlags];
  int shiftDown = ((flags & NSEventModifierFlagShift) != 0);
  int controlDown = ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0);
  int altDown = ((flags & NSEventModifierFlagOption) != 0);

  interactor->SetEventInformation(static_cast<int>(backingLoc.x), static_cast<int>(backingLoc.y),
    controlDown, shiftDown, 0, repeatCount);
  interactor->SetAltKey(altDown);
  interactor->InvokeEvent(theEventId, nullptr);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)keyDown:(NSEvent*)theEvent
{
  [self invokeSVTKKeyEvent:svtkCommand::KeyPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)keyUp:(NSEvent*)theEvent
{
  [self invokeSVTKKeyEvent:svtkCommand::KeyReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)flagsChanged:(NSEvent*)theEvent
{
  // what kind of event it is will be decided by invokeSVTKKeyEvent
  [self invokeSVTKKeyEvent:svtkCommand::AnyEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseMoved:(NSEvent*)theEvent
{
  // Note: this method will only be called if this view's NSWindow
  // is set to receive mouse moved events.  See setAcceptsMouseMovedEvents:
  // An NSWindow created by svtk automatically does accept such events.

  // Ignore motion outside the view in order to mimic other interactors
  NSPoint windowLoc = [theEvent locationInWindow];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  if (NSPointInRect(viewLoc, [self visibleRect]))
  {
    [self invokeSVTKMoveEvent:svtkCommand::MouseMoveEvent cocoaEvent:theEvent];
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseDragged:(NSEvent*)theEvent
{
  [self invokeSVTKMoveEvent:svtkCommand::MouseMoveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)rightMouseDragged:(NSEvent*)theEvent
{
  [self invokeSVTKMoveEvent:svtkCommand::MouseMoveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)otherMouseDragged:(NSEvent*)theEvent
{
  [self invokeSVTKMoveEvent:svtkCommand::MouseMoveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseEntered:(NSEvent*)theEvent
{
  // Note: the mouseEntered/mouseExited events depend on the maintenance of
  // the tracking area (updateTrackingAreas).
  [self invokeSVTKMoveEvent:svtkCommand::EnterEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseExited:(NSEvent*)theEvent
{
  [self invokeSVTKMoveEvent:svtkCommand::LeaveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)scrollWheel:(NSEvent*)theEvent
{
  CGFloat dy = [theEvent deltaY];

  unsigned long eventId = 0;

  if (dy > 0)
  {
    eventId = svtkCommand::MouseWheelForwardEvent;
  }
  else if (dy < 0)
  {
    eventId = svtkCommand::MouseWheelBackwardEvent;
  }

  if (eventId != 0)
  {
    [self invokeSVTKMoveEvent:eventId cocoaEvent:theEvent];
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseDown:(NSEvent*)theEvent
{
  [self invokeSVTKButtonEvent:svtkCommand::LeftButtonPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)rightMouseDown:(NSEvent*)theEvent
{
  [self invokeSVTKButtonEvent:svtkCommand::RightButtonPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)otherMouseDown:(NSEvent*)theEvent
{
  [self invokeSVTKButtonEvent:svtkCommand::MiddleButtonPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseUp:(NSEvent*)theEvent
{
  [self invokeSVTKButtonEvent:svtkCommand::LeftButtonReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)rightMouseUp:(NSEvent*)theEvent
{
  [self invokeSVTKButtonEvent:svtkCommand::RightButtonReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)otherMouseUp:(NSEvent*)theEvent
{
  [self invokeSVTKButtonEvent:svtkCommand::MiddleButtonReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// From NSDraggingDestination protocol.
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
  NSArray* types = [[sender draggingPasteboard] types];
  if ([types containsObject:(NSString*)kUTTypeFileURL])
  {
    return NSDragOperationCopy;
  }
  return NSDragOperationNone;
}

//----------------------------------------------------------------------------
// From NSDraggingDestination protocol.
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
  svtkCocoaRenderWindowInteractor* interactor = [self getInteractor];

  NSPoint pt = [sender draggingLocation];
  NSPoint viewLoc = [self convertPoint:pt fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];
  double location[2];
  location[0] = backingLoc.x;
  location[1] = backingLoc.y;
  interactor->InvokeEvent(svtkCommand::UpdateDropLocationEvent, location);

  svtkNew<svtkStringArray> filePaths;
  NSPasteboard* pboard = [sender draggingPasteboard];
  NSArray* fileURLs = [pboard readObjectsForClasses:@ [[NSURL class]] options:nil];
  for (NSURL* fileURL in fileURLs)
  {
    const char* filePath = [fileURL fileSystemRepresentation];
    filePaths->InsertNextValue(filePath);
  }

  if (filePaths->GetNumberOfTuples() > 0)
  {
    interactor->InvokeEvent(svtkCommand::DropFilesEvent, filePaths);
    return YES;
  }

  return NO;
}

//----------------------------------------------------------------------------
// Private
- (void)modifyDPIForBackingScaleFactorOfWindow:(/*nullable*/ NSWindow*)window
{
  if (window)
  {
    CGFloat backingScaleFactor = [window backingScaleFactor];
    assert(backingScaleFactor >= 1.0);

    svtkCocoaRenderWindow* renderWindow = [self getSVTKRenderWindow];
    if (renderWindow)
    {
      // Ordinarily, DPI is hardcoded to 72, but in order for svtkTextActors
      // to have the correct apparent size, we adjust it per the NSWindow's
      // scaling factor.
      renderWindow->SetDPI(lround(72.0 * backingScaleFactor));
    }
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)viewWillMoveToWindow:(/*nullable*/ NSWindow*)inNewWindow
{
  [super viewWillMoveToWindow:inNewWindow];

  [self modifyDPIForBackingScaleFactorOfWindow:inNewWindow];
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)viewDidChangeBackingProperties
{
  [super viewDidChangeBackingProperties];

  NSWindow* window = [self window];
  [self modifyDPIForBackingScaleFactorOfWindow:window];
}

@end
