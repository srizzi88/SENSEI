/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewiseFunctionItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkPiecewiseFunctionItem_h
#define svtkPiecewiseFunctionItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkScalarsToColorsItem.h"

class svtkPiecewiseFunction;
class svtkImageData;

/// svtkPiecewiseFunctionItem internal uses svtkPlot::Color, white by default
class SVTKCHARTSCORE_EXPORT svtkPiecewiseFunctionItem : public svtkScalarsToColorsItem
{
public:
  static svtkPiecewiseFunctionItem* New();
  svtkTypeMacro(svtkPiecewiseFunctionItem, svtkScalarsToColorsItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void SetPiecewiseFunction(svtkPiecewiseFunction* t);
  svtkGetObjectMacro(PiecewiseFunction, svtkPiecewiseFunction);

protected:
  svtkPiecewiseFunctionItem();
  ~svtkPiecewiseFunctionItem() override;

  // Description:
  // Reimplemented to return the range of the piecewise function
  void ComputeBounds(double bounds[4]) override;

  // Description
  // Compute the texture from the PiecewiseFunction
  void ComputeTexture() override;

  svtkPiecewiseFunction* PiecewiseFunction;

private:
  svtkPiecewiseFunctionItem(const svtkPiecewiseFunctionItem&) = delete;
  void operator=(const svtkPiecewiseFunctionItem&) = delete;
};

#endif
