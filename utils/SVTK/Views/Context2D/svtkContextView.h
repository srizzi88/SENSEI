/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContextView
 * @brief   provides a view of the svtkContextScene.
 *
 *
 * This class is derived from svtkRenderViewBase and provides a view of a
 * svtkContextScene, with a default interactor style, renderer etc. It is
 * the simplest way to create a svtkRenderWindow and display a 2D scene inside
 * of it.
 *
 * By default the scene has a white background.
 */

#ifndef svtkContextView_h
#define svtkContextView_h

#include "svtkRenderViewBase.h"
#include "svtkSmartPointer.h"         // Needed for SP ivars
#include "svtkViewsContext2DModule.h" // For export macro

class svtkContext2D;
class svtkContextScene;

class SVTKVIEWSCONTEXT2D_EXPORT svtkContextView : public svtkRenderViewBase
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkContextView, svtkRenderViewBase);

  static svtkContextView* New();

  /**
   * Set the svtkContext2D for the view.
   */
  virtual void SetContext(svtkContext2D* context);

  /**
   * Get the svtkContext2D for the view.
   */
  virtual svtkContext2D* GetContext();

  /**
   * Set the scene object for the view.
   */
  virtual void SetScene(svtkContextScene* scene);

  /**
   * Get the scene of the view.
   */
  virtual svtkContextScene* GetScene();

protected:
  svtkContextView();
  ~svtkContextView() override;

  svtkSmartPointer<svtkContextScene> Scene;
  svtkSmartPointer<svtkContext2D> Context;

private:
  svtkContextView(const svtkContextView&) = delete;
  void operator=(const svtkContextView&) = delete;
};

#endif
