/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeTransferFunctionItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkCompositeTransferFunctionItem_h
#define svtkCompositeTransferFunctionItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkColorTransferFunctionItem.h"

class svtkPiecewiseFunction;

// Description:
// svtkPlot::Color and svtkPlot::Brush have no effect here.
class SVTKCHARTSCORE_EXPORT svtkCompositeTransferFunctionItem : public svtkColorTransferFunctionItem
{
public:
  static svtkCompositeTransferFunctionItem* New();
  svtkTypeMacro(svtkCompositeTransferFunctionItem, svtkColorTransferFunctionItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void SetOpacityFunction(svtkPiecewiseFunction* opacity);
  svtkGetObjectMacro(OpacityFunction, svtkPiecewiseFunction);

protected:
  svtkCompositeTransferFunctionItem();
  ~svtkCompositeTransferFunctionItem() override;

  // Description:
  // Reimplemented to return the range of the piecewise function
  void ComputeBounds(double bounds[4]) override;

  void ComputeTexture() override;
  svtkPiecewiseFunction* OpacityFunction;

private:
  svtkCompositeTransferFunctionItem(const svtkCompositeTransferFunctionItem&) = delete;
  void operator=(const svtkCompositeTransferFunctionItem&) = delete;
};

#endif
