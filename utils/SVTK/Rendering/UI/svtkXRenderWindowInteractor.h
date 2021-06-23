/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXRenderWindowInteractor
 * @brief   an X event driven interface for a RenderWindow
 *
 * svtkXRenderWindowInteractor is a convenience object that provides event
 * bindings to common graphics functions. For example, camera and actor
 * functions such as zoom-in/zoom-out, azimuth, roll, and pan. IT is one of
 * the window system specific subclasses of svtkRenderWindowInteractor. Please
 * see svtkRenderWindowInteractor documentation for event bindings.
 *
 * @sa
 * svtkRenderWindowInteractor
 */

#ifndef svtkXRenderWindowInteractor_h
#define svtkXRenderWindowInteractor_h

//===========================================================
// now we define the C++ class

#include "svtkRenderWindowInteractor.h"
#include "svtkRenderingUIModule.h" // For export macro
#include <X11/Xlib.h>             // Needed for X types in the public interface

class svtkCallbackCommand;
class svtkXRenderWindowInteractorInternals;

class SVTKRENDERINGUI_EXPORT svtkXRenderWindowInteractor : public svtkRenderWindowInteractor
{
public:
  static svtkXRenderWindowInteractor* New();
  svtkTypeMacro(svtkXRenderWindowInteractor, svtkRenderWindowInteractor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Initializes the event handlers without an XtAppContext.  This is
   * good for when you don't have a user interface, but you still
   * want to have mouse interaction.
   */
  void Initialize() override;

  /**
   * Break the event loop on 'q','e' keypress. Want more ???
   */
  void TerminateApp() override;

  /**
   * Run the event loop and return. This is provided so that you can
   * implement your own event loop but yet use the svtk event handling as
   * well.
   */
  void ProcessEvents() override;

  //@{
  /**
   * The BreakLoopFlag is checked in the Start() method.
   * Setting it to anything other than zero will cause
   * the interactor loop to terminate and return to the
   * calling function.
   */
  svtkGetMacro(BreakLoopFlag, int);
  void SetBreakLoopFlag(int);
  void BreakLoopFlagOff();
  void BreakLoopFlagOn();
  //@}

  //@{
  /**
   * Enable/Disable interactions.  By default interactors are enabled when
   * initialized.  Initialize() must be called prior to enabling/disabling
   * interaction. These methods are used when a window/widget is being
   * shared by multiple renderers and interactors.  This allows a "modal"
   * display where one interactor is active when its data is to be displayed
   * and all other interactors associated with the widget are disabled
   * when their data is not displayed.
   */
  void Enable() override;
  void Disable() override;
  //@}

  /**
   * Update the Size data member and set the associated RenderWindow's
   * size.
   */
  void UpdateSize(int, int) override;

  /**
   * Re-defines virtual function to get mouse position by querying X-server.
   */
  void GetMousePosition(int* x, int* y) override;

  void DispatchEvent(XEvent*);

protected:
  svtkXRenderWindowInteractor();
  ~svtkXRenderWindowInteractor() override;

  /**
   * Update the Size data member and set the associated RenderWindow's
   * size but do not resize the XWindow.
   */
  void UpdateSizeNoXResize(int, int);

  // Using static here to avoid destroying context when many apps are open:
  static int NumAppInitialized;

  Display* DisplayId;
  Window WindowId;
  Atom KillAtom;
  int PositionBeforeStereo[2];
  svtkXRenderWindowInteractorInternals* Internal;

  // Drag and drop related
  Window XdndSource;
  Atom XdndPositionAtom;
  Atom XdndDropAtom;
  Atom XdndActionCopyAtom;
  Atom XdndStatusAtom;
  Atom XdndFinishedAtom;

  //@{
  /**
   * X-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  //@}

  void FireTimers();

  static int BreakLoopFlag;

  /**
   * This will start up the X event loop and never return. If you
   * call this method it will loop processing X events until the
   * application is exited.
   */
  void StartEventLoop() override;

private:
  svtkXRenderWindowInteractor(const svtkXRenderWindowInteractor&) = delete;
  void operator=(const svtkXRenderWindowInteractor&) = delete;
};

#endif
