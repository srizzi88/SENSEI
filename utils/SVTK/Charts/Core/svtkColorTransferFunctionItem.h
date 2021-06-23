/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkColorTransferFunctionItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkColorTransferFunctionItem_h
#define svtkColorTransferFunctionItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkScalarsToColorsItem.h"

class svtkColorTransferFunction;
class svtkImageData;

// Description:
// svtkPlot::Color, svtkPlot::Brush, svtkScalarsToColors::DrawPolyLine,
// svtkScalarsToColors::MaskAboveCurve have no effect here.
class SVTKCHARTSCORE_EXPORT svtkColorTransferFunctionItem : public svtkScalarsToColorsItem
{
public:
  static svtkColorTransferFunctionItem* New();
  svtkTypeMacro(svtkColorTransferFunctionItem, svtkScalarsToColorsItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void SetColorTransferFunction(svtkColorTransferFunction* t);
  svtkGetObjectMacro(ColorTransferFunction, svtkColorTransferFunction);

protected:
  svtkColorTransferFunctionItem();
  ~svtkColorTransferFunctionItem() override;

  // Description:
  // Reimplemented to return the range of the lookup table
  void ComputeBounds(double bounds[4]) override;

  void ComputeTexture() override;
  svtkColorTransferFunction* ColorTransferFunction;

  /**
   * Override the histogram plotbar configuration
   * in order to set the color transfer function on it
   */
  bool ConfigurePlotBar() override;

private:
  svtkColorTransferFunctionItem(const svtkColorTransferFunctionItem&) = delete;
  void operator=(const svtkColorTransferFunctionItem&) = delete;
};

#endif
