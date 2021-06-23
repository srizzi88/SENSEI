/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSynchronizedRenderWindows.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSynchronizedRenderWindows
 * @brief   synchronizes render windows across
 * processes.
 *
 * svtkSynchronizedRenderWindows is used to synchronize render windows across
 * processes for parallel rendering.
 */

#ifndef svtkSynchronizedRenderWindows_h
#define svtkSynchronizedRenderWindows_h

#include "svtkObject.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkRenderWindow;
class svtkMultiProcessController;
class svtkCommand;
class svtkMultiProcessStream;

class SVTKRENDERINGPARALLEL_EXPORT svtkSynchronizedRenderWindows : public svtkObject
{
public:
  static svtkSynchronizedRenderWindows* New();
  svtkTypeMacro(svtkSynchronizedRenderWindows, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the render window to be synchronized by this
   * svtkSynchronizedRenderWindows instance. A svtkSynchronizedRenderWindows can
   * be used to synchronize exactly 1 svtkRenderWindow on each process.
   */
  void SetRenderWindow(svtkRenderWindow*);
  svtkGetObjectMacro(RenderWindow, svtkRenderWindow);
  //@}

  //@{
  /**
   * Set the parallel message communicator. This is used to communicate among
   * processes.
   */
  void SetParallelController(svtkMultiProcessController*);
  svtkGetObjectMacro(ParallelController, svtkMultiProcessController);
  //@}

  //@{
  /**
   * It's acceptable to have multiple instances on svtkSynchronizedRenderWindows
   * on each processes to synchronize different render windows. In that case
   * there's no way to each of the svtkSynchronizedRenderWindows instance to know
   * how they correspond across processes. To enable that identification, a
   * svtkSynchronizedRenderWindows can be assigned a unique id. All
   * svtkSynchronizedRenderWindows across different processes that have the same
   * id are "linked" together for synchronization. It's critical that the id is
   * set before any rendering happens.
   */
  void SetIdentifier(unsigned int id);
  svtkGetMacro(Identifier, unsigned int);
  //@}

  //@{
  /**
   * Enable/Disable parallel rendering. Unless ParallelRendering is ON, no
   * synchronization of svtkRenderWindow::Render() calls between processes
   * happens. ON by default.
   */
  svtkSetMacro(ParallelRendering, bool);
  svtkGetMacro(ParallelRendering, bool);
  svtkBooleanMacro(ParallelRendering, bool);
  //@}

  // Turns on/off render event propagation.  When on (the default) and
  // ParallelRendering is on, process 0 will send an RMI call to all remote
  // processes to perform a synchronized render.  When off, render must be
  // manually called on each process.
  svtkSetMacro(RenderEventPropagation, bool);
  svtkGetMacro(RenderEventPropagation, bool);
  svtkBooleanMacro(RenderEventPropagation, bool);

  /**
   * This method call be called while a render is in progress to abort the
   * rendering. It should be called on the root node (or client).
   */
  virtual void AbortRender();

  //@{
  /**
   * Get/Set the root-process id. This is required when the ParallelController
   * is a svtkSocketController. Set to 0 by default (which will not work when
   * using a svtkSocketController but will work for svtkMPIController).
   */
  svtkSetMacro(RootProcessId, int);
  svtkGetMacro(RootProcessId, int);
  //@}

  enum
  {
    SYNC_RENDER_TAG = 15001,
  };

protected:
  svtkSynchronizedRenderWindows();
  ~svtkSynchronizedRenderWindows() override;

  struct RenderWindowInfo
  {
    int WindowSize[2];
    int TileScale[2];
    double TileViewport[4];
    double DesiredUpdateRate;

    // Save/restore the struct to/from a stream.
    void Save(svtkMultiProcessStream& stream);
    bool Restore(svtkMultiProcessStream& stream);
    void CopyFrom(svtkRenderWindow*);
    void CopyTo(svtkRenderWindow*);
  };

  // These methods are called on all processes as a consequence of corresponding
  // events being called on the render window.
  virtual void HandleStartRender();
  virtual void HandleEndRender() {}
  virtual void HandleAbortRender() {}

  virtual void MasterStartRender();
  virtual void SlaveStartRender();

  unsigned int Identifier;
  bool ParallelRendering;
  bool RenderEventPropagation;
  int RootProcessId;

  svtkRenderWindow* RenderWindow;
  svtkMultiProcessController* ParallelController;

private:
  svtkSynchronizedRenderWindows(const svtkSynchronizedRenderWindows&) = delete;
  void operator=(const svtkSynchronizedRenderWindows&) = delete;

  class svtkObserver;
  svtkObserver* Observer;
  friend class svtkObserver;
};

#endif
