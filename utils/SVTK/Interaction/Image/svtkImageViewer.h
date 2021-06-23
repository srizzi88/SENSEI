/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageViewer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageViewer
 * @brief   Display a 2d image.
 *
 * svtkImageViewer is a convenience class for displaying a 2d image.  It
 * packages up the functionality found in svtkRenderWindow, svtkRenderer,
 * svtkActor2D and svtkImageMapper into a single easy to use class.  Behind the
 * scenes these four classes are actually used to to provide the required
 * functionality. svtkImageViewer is simply a wrapper around them.
 *
 * @sa
 * svtkRenderWindow svtkRenderer svtkImageMapper svtkActor2D
 */

#ifndef svtkImageViewer_h
#define svtkImageViewer_h

#include "svtkInteractionImageModule.h" // For export macro
#include "svtkObject.h"

#include "svtkImageMapper.h"  // For all the inline methods
#include "svtkRenderWindow.h" // For all the inline methods

class svtkInteractorStyleImage;

class SVTKINTERACTIONIMAGE_EXPORT svtkImageViewer : public svtkObject
{
public:
  static svtkImageViewer* New();

  svtkTypeMacro(svtkImageViewer, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get name of rendering window
   */
  char* GetWindowName() { return this->RenderWindow->GetWindowName(); }

  /**
   * Render the resulting image.
   */
  virtual void Render(void);

  //@{
  /**
   * Set/Get the input to the viewer.
   */
  void SetInputData(svtkImageData* in) { this->ImageMapper->SetInputData(in); }
  svtkImageData* GetInput() { return this->ImageMapper->GetInput(); }
  virtual void SetInputConnection(svtkAlgorithmOutput* input)
  {
    this->ImageMapper->SetInputConnection(input);
  }
  //@}

  //@{
  /**
   * What is the possible Min/ Max z slices available.
   */
  int GetWholeZMin() { return this->ImageMapper->GetWholeZMin(); }
  int GetWholeZMax() { return this->ImageMapper->GetWholeZMax(); }
  //@}

  //@{
  /**
   * Set/Get the current Z Slice to display
   */
  int GetZSlice() { return this->ImageMapper->GetZSlice(); }
  void SetZSlice(int s) { this->ImageMapper->SetZSlice(s); }
  //@}

  //@{
  /**
   * Sets window/level for mapping pixels to colors.
   */
  double GetColorWindow() { return this->ImageMapper->GetColorWindow(); }
  double GetColorLevel() { return this->ImageMapper->GetColorLevel(); }
  void SetColorWindow(double s) { this->ImageMapper->SetColorWindow(s); }
  void SetColorLevel(double s) { this->ImageMapper->SetColorLevel(s); }
  //@}

  //@{
  /**
   * These are here for using a tk window.
   */
  void SetDisplayId(void* a) { this->RenderWindow->SetDisplayId(a); }
  void SetWindowId(void* a) { this->RenderWindow->SetWindowId(a); }
  void SetParentId(void* a) { this->RenderWindow->SetParentId(a); }
  //@}

  //@{
  /**
   * Get the position (x and y) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetPosition() SVTK_SIZEHINT(2) { return this->RenderWindow->GetPosition(); }

  /**
   * Set the position (x and y) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   */
  void SetPosition(int x, int y) { this->RenderWindow->SetPosition(x, y); }
  virtual void SetPosition(int a[2]);
  //@}

  //@{
  /**
   * Get the size (width and height) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetSize() SVTK_SIZEHINT(2) { return this->RenderWindow->GetSize(); }

  /**
   * Set the size (width and height) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   *
   * If the size has changed, this method will fire
   * svtkCommand::WindowResizeEvent.
   */
  void SetSize(int width, int height) { this->RenderWindow->SetSize(width, height); }
  virtual void SetSize(int a[2]);
  //@}

  //@{
  /**
   * Get the internal objects
   */
  svtkGetObjectMacro(RenderWindow, svtkRenderWindow);
  void SetRenderWindow(svtkRenderWindow* renWin);
  svtkGetObjectMacro(Renderer, svtkRenderer);
  svtkGetObjectMacro(ImageMapper, svtkImageMapper);
  svtkGetObjectMacro(Actor2D, svtkActor2D);
  //@}

  /**
   * Create and attach an interactor for this window
   */
  void SetupInteractor(svtkRenderWindowInteractor*);

  //@{
  /**
   * Create a window in memory instead of on the screen. This may not
   * be supported for every type of window and on some windows you may
   * need to invoke this prior to the first render.
   */
  void SetOffScreenRendering(svtkTypeBool);
  svtkTypeBool GetOffScreenRendering();
  void OffScreenRenderingOn();
  void OffScreenRenderingOff();
  //@}

protected:
  svtkImageViewer();
  ~svtkImageViewer() override;

  svtkRenderWindow* RenderWindow;
  svtkRenderer* Renderer;
  svtkImageMapper* ImageMapper;
  svtkActor2D* Actor2D;
  int FirstRender;
  svtkRenderWindowInteractor* Interactor;
  svtkInteractorStyleImage* InteractorStyle;

  friend class svtkImageViewerCallback;
  svtkAlgorithm* GetInputAlgorithm();

private:
  svtkImageViewer(const svtkImageViewer&) = delete;
  void operator=(const svtkImageViewer&) = delete;
};

#endif
