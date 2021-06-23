/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLookupTableItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkLookupTableItem_h
#define svtkLookupTableItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkScalarsToColorsItem.h"

class svtkLookupTable;

// Description:
// svtkPlot::Color, svtkPlot::Brush, svtkScalarsToColors::DrawPolyLine,
// svtkScalarsToColors::MaskAboveCurve have no effect here.
class SVTKCHARTSCORE_EXPORT svtkLookupTableItem : public svtkScalarsToColorsItem
{
public:
  static svtkLookupTableItem* New();
  svtkTypeMacro(svtkLookupTableItem, svtkScalarsToColorsItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void SetLookupTable(svtkLookupTable* t);
  svtkGetObjectMacro(LookupTable, svtkLookupTable);

protected:
  svtkLookupTableItem();
  ~svtkLookupTableItem() override;

  // Description:
  // Reimplemented to return the range of the lookup table
  void ComputeBounds(double bounds[4]) override;

  void ComputeTexture() override;
  svtkLookupTable* LookupTable;

private:
  svtkLookupTableItem(const svtkLookupTableItem&) = delete;
  void operator=(const svtkLookupTableItem&) = delete;
};

#endif
