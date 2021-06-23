/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextItem
 * @brief   base class for items that are part of a svtkContextScene.
 *
 *
 * Derive from this class to create custom items that can be added to a
 * svtkContextScene.
 */

#ifndef svtkContextItem_h
#define svtkContextItem_h

#include "svtkAbstractContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextItem : public svtkAbstractContextItem
{
public:
  svtkTypeMacro(svtkContextItem, svtkAbstractContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the opacity of the item.
   */
  svtkGetMacro(Opacity, double);
  //@}

  //@{
  /**
   * Set the opacity of the item.
   * 1.0 by default.
   */
  svtkSetMacro(Opacity, double);
  //@}

protected:
  svtkContextItem();
  ~svtkContextItem() override;

  double Opacity;

private:
  svtkContextItem(const svtkContextItem&) = delete;
  void operator=(const svtkContextItem&) = delete;
};

#endif // svtkContextItem_h
