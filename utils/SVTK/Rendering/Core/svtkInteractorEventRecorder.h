/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorEventRecorder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorEventRecorder
 * @brief   record and play SVTK events passing through a svtkRenderWindowInteractor
 *
 *
 * svtkInteractorEventRecorder records all SVTK events invoked from a
 * svtkRenderWindowInteractor. The events are recorded to a
 * file. svtkInteractorEventRecorder can also be used to play those events
 * back and invoke them on an svtkRenderWindowInteractor. (Note: the events
 * can also be played back from a file or string.)
 *
 * The format of the event file is simple. It is:
 *  EventName X Y ctrl shift keycode repeatCount keySym
 * The format also allows "#" comments.
 *
 * @sa
 * svtkInteractorObserver svtkCallback
 */

#ifndef svtkInteractorEventRecorder_h
#define svtkInteractorEventRecorder_h

#include "svtkInteractorObserver.h"
#include "svtkRenderingCoreModule.h" // For export macro

// The superclass that all commands should be subclasses of
class SVTKRENDERINGCORE_EXPORT svtkInteractorEventRecorder : public svtkInteractorObserver
{
public:
  static svtkInteractorEventRecorder* New();
  svtkTypeMacro(svtkInteractorEventRecorder, svtkInteractorObserver);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Satisfy the superclass API. Enable/disable listening for events.
  void SetEnabled(int) override;
  void SetInteractor(svtkRenderWindowInteractor* iren) override;

  //@{
  /**
   * Set/Get the name of a file events should be written to/from.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  /**
   * Invoke this method to begin recording events. The events will be
   * recorded to the filename indicated.
   */
  void Record();

  /**
   * Invoke this method to begin playing events from the current position.
   * The events will be played back from the filename indicated.
   */
  void Play();

  /**
   * Invoke this method to stop recording/playing events.
   */
  void Stop();

  /**
   * Rewind to the beginning of the file.
   */
  void Rewind();

  //@{
  /**
   * Enable reading from an InputString as compared to the default
   * behavior, which is to read from a file.
   */
  svtkSetMacro(ReadFromInputString, svtkTypeBool);
  svtkGetMacro(ReadFromInputString, svtkTypeBool);
  svtkBooleanMacro(ReadFromInputString, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the string to read from.
   */
  svtkSetStringMacro(InputString);
  svtkGetStringMacro(InputString);
  //@}

protected:
  svtkInteractorEventRecorder();
  ~svtkInteractorEventRecorder() override;

  // file to read/write from
  char* FileName;

  // listens to delete events
  svtkCallbackCommand* DeleteEventCallbackCommand;

  // control whether to read from string
  svtkTypeBool ReadFromInputString;
  char* InputString;

  // for reading and writing
  istream* InputStream;
  ostream* OutputStream;

  // methods for processing events
  static void ProcessCharEvent(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);
  static void ProcessDeleteEvent(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);
  static void ProcessEvents(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  virtual void WriteEvent(
    const char* event, int pos[2], int modifiers, int keyCode, int repeatCount, char* keySym);

  virtual void ReadEvent();

  // Manage the state of the recorder
  int State;
  enum WidgetState
  {
    Start = 0,
    Playing,
    Recording
  };

  // Associate a modifier with a bit
  enum ModifierKey
  {
    ShiftKey = 1,
    ControlKey = 2,
    AltKey = 4
  };

  static float StreamVersion;

private:
  svtkInteractorEventRecorder(const svtkInteractorEventRecorder&) = delete;
  void operator=(const svtkInteractorEventRecorder&) = delete;
};

#endif /* svtkInteractorEventRecorder_h */
