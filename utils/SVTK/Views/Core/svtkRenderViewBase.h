/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderViewBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkRenderViewBase
 * @brief   A base view containing a renderer.
 *
 *
 * svtkRenderViewBase is a view which contains a svtkRenderer.  You may add
 * svtkActors directly to the renderer.
 *
 * This class is also the parent class for any more specialized view which uses
 * a renderer.
 *
 */

#ifndef svtkRenderViewBase_h
#define svtkRenderViewBase_h

#include "svtkSmartPointer.h" // For SP ivars
#include "svtkView.h"
#include "svtkViewsCoreModule.h" // For export macro

class svtkInteractorObserver;
class svtkRenderer;
class svtkRenderWindow;
class svtkRenderWindowInteractor;

class SVTKVIEWSCORE_EXPORT svtkRenderViewBase : public svtkView
{
public:
  static svtkRenderViewBase* New();
  svtkTypeMacro(svtkRenderViewBase, svtkView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Gets the renderer for this view.
   */
  virtual svtkRenderer* GetRenderer();

  // Sets the renderer for this view.
  virtual void SetRenderer(svtkRenderer* ren);

  /**
   * Get a handle to the render window.
   */
  virtual svtkRenderWindow* GetRenderWindow();

  /**
   * Set the render window for this view. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of svtkRenderViewBase.
   */
  virtual void SetRenderWindow(svtkRenderWindow* win);

  //@{
  /**
   * The render window interactor. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of svtkRenderViewBase.
   */
  virtual svtkRenderWindowInteractor* GetInteractor();
  virtual void SetInteractor(svtkRenderWindowInteractor*);
  //@}

  /**
   * Updates the representations, then calls Render() on the render window
   * associated with this view.
   */
  virtual void Render();

  /**
   * Updates the representations, then calls ResetCamera() on the renderer
   * associated with this view.
   */
  virtual void ResetCamera();

  /**
   * Updates the representations, then calls ResetCameraClippingRange() on the
   * renderer associated with this view.
   */
  virtual void ResetCameraClippingRange();

protected:
  svtkRenderViewBase();
  ~svtkRenderViewBase() override;

  /**
   * Called by the view when the renderer is about to render.
   */
  virtual void PrepareForRendering();

  svtkSmartPointer<svtkRenderer> Renderer;
  svtkSmartPointer<svtkRenderWindow> RenderWindow;

private:
  svtkRenderViewBase(const svtkRenderViewBase&) = delete;
  void operator=(const svtkRenderViewBase&) = delete;
};

#endif
