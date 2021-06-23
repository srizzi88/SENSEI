/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPropItem
 * @brief   Embed a svtkProp in a svtkContextScene.
 *
 *
 * This class allows svtkProp objects to be drawn inside a svtkContextScene.
 * This is especially useful for constructing layered scenes that need to ignore
 * depth testing.
 */

#ifndef svtkPropItem_h
#define svtkPropItem_h

#include "svtkAbstractContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro

class svtkProp;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkPropItem : public svtkAbstractContextItem
{
public:
  static svtkPropItem* New();
  svtkTypeMacro(svtkPropItem, svtkAbstractContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  bool Paint(svtkContext2D* painter) override;
  void ReleaseGraphicsResources() override;

  /**
   * The actor to render.
   */
  virtual void SetPropObject(svtkProp* PropObject);
  svtkGetObjectMacro(PropObject, svtkProp);

protected:
  svtkPropItem();
  ~svtkPropItem() override;

  // Sync the active svtkCamera with the GL state set by the painter.
  virtual void UpdateTransforms();

  // Restore the svtkCamera state.
  virtual void ResetTransforms();

private:
  svtkProp* PropObject;

  svtkPropItem(const svtkPropItem&) = delete;
  void operator=(const svtkPropItem&) = delete;
};

#endif // svtkPropItem_h
