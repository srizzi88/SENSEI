/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataItem
 * @brief   Filter that translate a svtkPolyData 2D mesh into svtkContextItems.
 *
 * @warning
 * The input svtkPolyData should be a 2D mesh.
 *
 */

#ifndef svtkPolyDataItem_h
#define svtkPolyDataItem_h

#include "svtkContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro

class svtkPolyData;
class svtkUnsignedCharArray;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkPolyDataItem : public svtkContextItem
{
public:
  svtkTypeMacro(svtkPolyDataItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPolyDataItem* New();

  /**
   * Paint event for the item.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Set the PolyData of the item.
   */
  void SetPolyData(svtkPolyData* polyData);

  /**
   * Set mapped colors. User-selected scalars are mapped to a color lookup
   * table externally.
   */
  void SetMappedColors(svtkUnsignedCharArray* colors);

  /**
   * Get the image of the item.
   */
  svtkGetObjectMacro(PolyData, svtkPolyData);

  /**
   * Set the position of the bottom corner of the image.
   */
  svtkSetVector2Macro(Position, float);

  /**
   * Set the data scalar mode.
   */
  svtkSetMacro(ScalarMode, int);

protected:
  svtkPolyDataItem();
  ~svtkPolyDataItem() override;

  class DrawHintsHelper;
  DrawHintsHelper* HintHelper;

  float Position[2];

  svtkPolyData* PolyData;

  svtkUnsignedCharArray* MappedColors;

  int ScalarMode;

private:
  svtkPolyDataItem(const svtkPolyDataItem&) = delete;
  void operator=(const svtkPolyDataItem&) = delete;
};

#endif
