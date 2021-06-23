/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCocoaRenderWindowInteractor.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#import "svtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs
#import <Cocoa/Cocoa.h>

#import "svtkCocoaRenderWindowInteractor.h"
#import "svtkCommand.h"
#import "svtkObjectFactory.h"
#import "svtkRenderWindow.h"

#ifdef SVTK_USE_TDX
#import "svtkTDxMacDevice.h"
#endif

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkCocoaRenderWindowInteractor);

//----------------------------------------------------------------------------
void (*svtkCocoaRenderWindowInteractor::ClassExitMethod)(void*) = nullptr;
void* svtkCocoaRenderWindowInteractor::ClassExitMethodArg = nullptr;
void (*svtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)(void*) = nullptr;

//----------------------------------------------------------------------------
// This is a private class and an implementation detail, do not use it.
//----------------------------------------------------------------------------
// As svtk is both crossplatform and a library, we don't know if it is being
// used in a 'regular Cocoa application' or as a 'pure svtk application'.
// By the former we mean a regular Cocoa application that happens to have
// a svtkCocoaGLView, by the latter we mean an application that only uses
// SVTK APIs (which happen to use Cocoa as an implementation detail).
// Specifically, we can't know if NSApplicationMain() was ever called
// (which is usually done in main()), nor whether the NSApplication exists.
//
// Any code that uses Cocoa needs to be sure that NSAutoreleasePools are
// correctly setup.  Specifically, an NSAutoreleasePool needs to exist before
// any Cocoa objects are autoreleased, which may happen as a side effect of
// pretty much any Cocoa use.  Once we start the event loop by calling
// either NSApplicationMain() or [NSApp run] then a new pool will be created
// for every event.  In a 'regular Cocoa application' NSApplicationMain() is
// called at the start of main() and so we know a pool will be in place before
// any svtk code is used.  But in a 'pure svtk application' the event loop does
// not start until [NSApp run] is called by this class's Start() method. The
// problem thus is that other SVTK code may be called before Start() and that
// code may call Cocoa and thus autorelease objects with no autorelease pool
// in place.  The (ugly) solution is to create a 'pool of last resort' so
// that we know a pool is always in place.
// See: <http://lists.apple.com/archives/cocoa-dev/2006/Sep/msg00222.html>
//
// With garbage collection (GC), autorelease pools are a thing of the past,
// and this hack is not needed.  However, Obj-C code can be compiled in
// 1 of 4 memory management modes: Manual Retain Release (MRR), GC supported,
// GC required, or Automatic Reference Counting (ARC).  Library code like
// SVTK should work with all 4; currently all are supported except ARC.
class svtkEarlyCocoaSetup
{
public:
  svtkEarlyCocoaSetup() { this->CreatePoolOfLastResort(); }

  virtual ~svtkEarlyCocoaSetup() { this->DestroyPoolOfLastResort(); }

protected:
  void DestroyPoolOfLastResort()
  {
    [Pool drain];
    Pool = nil;
  }

  void CreatePoolOfLastResort() { Pool = [[NSAutoreleasePool alloc] init]; }

private:
  NSAutoreleasePool* Pool;
};

// We create a global/static instance of this class to ensure that we have an
// autorelease pool before main() starts (the C++ constructor for a global
// object runs before main).  Note: I am unable to find a place to delete this
// object safely, but having it around for the lifetime of the process is ok.
static svtkEarlyCocoaSetup* gEarlyCocoaSetup = new svtkEarlyCocoaSetup();

//----------------------------------------------------------------------------
// This is a private class and an implementation detail, do not use it.
//----------------------------------------------------------------------------
@interface svtkCocoaTimer : NSObject
{
@private
  NSTimer* _timer;
  svtkCocoaRenderWindowInteractor* _interactor;
  int _timerId;
}

- (id)initWithInteractor:(svtkCocoaRenderWindowInteractor*)myInteractor timerId:(int)myTimerId;
- (void)startTimerWithInterval:(NSTimeInterval)interval repeating:(BOOL)repeating;
- (void)stopTimer;
- (void)timerFired:(NSTimer*)myTimer;

@end

//----------------------------------------------------------------------------
@implementation svtkCocoaTimer

//----------------------------------------------------------------------------
- (id)initWithInteractor:(svtkCocoaRenderWindowInteractor*)myInteractor timerId:(int)myTimerId
{
  self = [super init];
  if (self)
  {
    _interactor = myInteractor;
    _timerId = myTimerId;
  }
  return self;
}

//----------------------------------------------------------------------------
- (void)timerFired:(NSTimer*)myTimer
{
  (void)myTimer;
  _interactor->InvokeEvent(svtkCommand::TimerEvent, &_timerId);
}

//----------------------------------------------------------------------------
- (void)startTimerWithInterval:(NSTimeInterval)interval repeating:(BOOL)repeating
{
  _timer = [NSTimer timerWithTimeInterval:interval
                                   target:self
                                 selector:@selector(timerFired:)
                                 userInfo:nil
                                  repeats:repeating];
#if !SVTK_OBJC_IS_ARC
  [_timer retain];
#endif

  NSRunLoop* runLoop = [NSRunLoop currentRunLoop];
  [runLoop addTimer:_timer forMode:NSDefaultRunLoopMode];
  [runLoop addTimer:_timer forMode:NSEventTrackingRunLoopMode];
}

//----------------------------------------------------------------------------
- (void)stopTimer
{
  [_timer invalidate];
#if !SVTK_OBJC_IS_ARC
  [_timer release];
#endif
  _timer = nil;
}

@end

//----------------------------------------------------------------------------
// Private
static void SVTKStartNSApplicationEventLoop()
{
  // Start the NSApplication's run loop
  NSApplication* application = [NSApplication sharedApplication];
  [application run];
}

//----------------------------------------------------------------------------
// Private
static void SVTKStopNSApplicationEventLoop()
{
  // Stop the current run loop. Do not terminate as it will not return to
  // main. However, the stop message puts the run loop asleep. Let's send
  // some event to wake it up so it can get out of the loop.
  NSApplication* application = [NSApplication sharedApplication];
  [application stop:application];

  NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                      location:NSZeroPoint
                                 modifierFlags:0
                                     timestamp:0
                                  windowNumber:-1
                                       context:nil
                                       subtype:0
                                         data1:0
                                         data2:0];
  [application postEvent:event atStart:YES];
}

//----------------------------------------------------------------------------
svtkCocoaRenderWindowInteractor::svtkCocoaRenderWindowInteractor()
{
  // First, create the cocoa objects manager. The dictionary is empty so
  // essentially all objects are initialized to NULL.
  NSMutableDictionary* cocoaManager = [NSMutableDictionary dictionary];

  // SetCocoaManager works like an Obj-C setter, so do like Obj-C and
  // init the ivar to null first.
  this->CocoaManager = nullptr;
  this->SetCocoaManager(reinterpret_cast<void*>(cocoaManager));
  [cocoaManager self]; // prevent premature collection under GC.

  NSMutableDictionary* timerDictionary = [NSMutableDictionary dictionary];
  this->SetTimerDictionary(reinterpret_cast<void*>(timerDictionary));
  [timerDictionary self]; // prevent premature collection under GC.

#ifdef SVTK_USE_TDX
  this->Device = svtkTDxMacDevice::New();
#endif
}

//----------------------------------------------------------------------------
svtkCocoaRenderWindowInteractor::~svtkCocoaRenderWindowInteractor()
{
  this->Enabled = 0;
  this->SetTimerDictionary(nullptr);
  this->SetCocoaManager(nullptr);
#ifdef SVTK_USE_TDX
  this->Device->Delete();
#endif
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::ProcessEvents()
{
  NSApplication* application = [NSApplication sharedApplication];
  NSEvent* event = [application nextEventMatchingMask:NSEventMaskAny
                                            untilDate:nil
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
  while (event)
  {
    [application sendEvent:event];
    event = [application nextEventMatchingMask:NSEventMaskAny
                                     untilDate:nil
                                        inMode:NSDefaultRunLoopMode
                                       dequeue:YES];
  }
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::StartEventLoop()
{
  SVTKStartNSApplicationEventLoop();
}

//----------------------------------------------------------------------------
// Begin processing keyboard strokes.
void svtkCocoaRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if (!this->RenderWindow)
  {
    svtkErrorMacro(<< "No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }
  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  svtkRenderWindow* renWin = this->RenderWindow;
  renWin->Start();
  renWin->End();
  const int* size = renWin->GetSize();

  renWin->GetPosition(); // update values of this->Position[2]

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::Enable()
{
  if (this->Enabled)
  {
    return;
  }

  // Set the RenderWindow's interactor so that when the svtkCocoaGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(this);

#ifdef SVTK_USE_TDX
  if (this->UseTDx)
  {
    this->Device->SetInteractor(this);
    this->Device->Initialize();
  }
#endif

  this->Enabled = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
  {
    return;
  }

#ifdef SVTK_USE_TDX
  if (this->Device->GetInitialized())
  {
    this->Device->Close();
  }
#endif

  // Set the RenderWindow's interactor so that when the svtkCocoaGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(nullptr);

  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::TerminateApp()
{
  this->Done = true;
  SVTKStopNSApplicationEventLoop();
}

//----------------------------------------------------------------------------
int svtkCocoaRenderWindowInteractor::InternalCreateTimer(
  int timerId, int timerType, unsigned long duration)
{
  BOOL repeating = NO;

  if (RepeatingTimer == timerType)
  {
    repeating = YES;
  }

  // Create a svtkCocoaTimer and add it to a dictionary using the timerId
  // as key, this will let us find the svtkCocoaTimer later by timerId
  svtkCocoaTimer* cocoaTimer = [[svtkCocoaTimer alloc] initWithInteractor:this timerId:timerId];
  NSString* timerIdAsStr = [NSString stringWithFormat:@"%i", timerId];
  NSMutableDictionary* timerDict = (NSMutableDictionary*)(this->GetTimerDictionary());
  [timerDict setObject:cocoaTimer forKey:timerIdAsStr];
  [cocoaTimer startTimerWithInterval:((NSTimeInterval)duration / 1000.0) repeating:repeating];
#if !SVTK_OBJC_IS_ARC
  [cocoaTimer release];
#endif

  // In this implementation, timerId and platformTimerId are the same
  int platformTimerId = timerId;

  return platformTimerId;
}

//----------------------------------------------------------------------------
int svtkCocoaRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  // In this implementation, timerId and platformTimerId are the same;
  // but calling this anyway is more correct
  int timerId = this->GetSVTKTimerId(platformTimerId);

  NSString* timerIdAsStr = [NSString stringWithFormat:@"%i", timerId];
  NSMutableDictionary* timerDict = (NSMutableDictionary*)(this->GetTimerDictionary());
  svtkCocoaTimer* cocoaTimer = [timerDict objectForKey:timerIdAsStr];

  if (nil != cocoaTimer)
  {
    [cocoaTimer stopTimer];
    [timerDict removeObjectForKey:timerIdAsStr];
    return 1; // success
  }
  else
  {
    return 0; // fail
  }
}

//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void svtkCocoaRenderWindowInteractor::SetClassExitMethod(void (*f)(void*), void* arg)
{
  if (f != svtkCocoaRenderWindowInteractor::ClassExitMethod ||
    arg != svtkCocoaRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((svtkCocoaRenderWindowInteractor::ClassExitMethodArg) &&
      (svtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*svtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)(
        svtkCocoaRenderWindowInteractor::ClassExitMethodArg);
    }
    svtkCocoaRenderWindowInteractor::ClassExitMethod = f;
    svtkCocoaRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void svtkCocoaRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void*))
{
  if (f != svtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    svtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(svtkCommand::ExitEvent))
  {
    this->InvokeEvent(svtkCommand::ExitEvent, nullptr);
  }
  else if (this->ClassExitMethod)
  {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
  }
  this->TerminateApp();
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::SetTimerDictionary(void* dictionary)
{
  if (dictionary != nullptr)
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(dictionary) forKey:@"TimerDictionary"];
  }
  else
  {
    NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
    [manager removeObjectForKey:@"TimerDictionary"];
  }
}

//----------------------------------------------------------------------------
void* svtkCocoaRenderWindowInteractor::GetTimerDictionary()
{
  NSMutableDictionary* manager = reinterpret_cast<NSMutableDictionary*>(this->GetCocoaManager());
  return reinterpret_cast<void*>([manager objectForKey:@"TimerDictionary"]);
}

//----------------------------------------------------------------------------
void svtkCocoaRenderWindowInteractor::SetCocoaManager(void* manager)
{
  NSMutableDictionary* currentCocoaManager =
    reinterpret_cast<NSMutableDictionary*>(this->CocoaManager);
  NSMutableDictionary* newCocoaManager = reinterpret_cast<NSMutableDictionary*>(manager);

  if (currentCocoaManager != newCocoaManager)
  {
    // Why not use Cocoa's retain and release?  Without garbage collection
    // (GC), the two are equivalent anyway because of 'toll free bridging',
    // so no problem there.  With GC, retain and release do nothing, but
    // CFRetain and CFRelease still manipulate the internal reference count.
    // We need that, since we are not using strong references (we don't want
    // it collected out from under us!).
    if (currentCocoaManager)
    {
      CFRelease(currentCocoaManager);
    }
    if (newCocoaManager)
    {
      this->CocoaManager = const_cast<void*>(CFRetain(newCocoaManager));
    }
    else
    {
      this->CocoaManager = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void* svtkCocoaRenderWindowInteractor::GetCocoaManager()
{
  return this->CocoaManager;
}
