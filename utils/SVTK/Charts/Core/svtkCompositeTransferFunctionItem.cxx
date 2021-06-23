/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeTransferFunctionItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCompositeTransferFunctionItem.h"
#include "svtkAxis.h"
#include "svtkCallbackCommand.h"
#include "svtkColorTransferFunction.h"
#include "svtkCommand.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPoints2D.h"

// STD includes
#include <algorithm>
#include <cassert>
#include <vector>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkCompositeTransferFunctionItem);

//-----------------------------------------------------------------------------
svtkCompositeTransferFunctionItem::svtkCompositeTransferFunctionItem()
{
  this->PolyLinePen->SetLineType(svtkPen::SOLID_LINE);
  this->OpacityFunction = nullptr;
}

//-----------------------------------------------------------------------------
svtkCompositeTransferFunctionItem::~svtkCompositeTransferFunctionItem()
{
  if (this->OpacityFunction)
  {
    this->OpacityFunction->RemoveObserver(this->Callback);
    this->OpacityFunction->Delete();
    this->OpacityFunction = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeTransferFunctionItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CompositeTransferFunction: ";
  if (this->OpacityFunction)
  {
    os << endl;
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeTransferFunctionItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->OpacityFunction)
  {
    double unused;
    double opacityRange[2];
    this->OpacityFunction->GetRange(opacityRange);
    this->TransformDataToScreen(opacityRange[0], 1, bounds[0], unused);
    this->TransformDataToScreen(opacityRange[1], 1, bounds[1], unused);
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeTransferFunctionItem::SetOpacityFunction(svtkPiecewiseFunction* opacity)
{
  if (opacity == this->OpacityFunction)
  {
    return;
  }
  if (this->OpacityFunction)
  {
    this->OpacityFunction->RemoveObserver(this->Callback);
  }
  svtkSetObjectBodyMacro(OpacityFunction, svtkPiecewiseFunction, opacity);
  if (opacity)
  {
    opacity->AddObserver(svtkCommand::ModifiedEvent, this->Callback);
  }
  this->ScalarsToColorsModified(this->OpacityFunction, svtkCommand::ModifiedEvent, nullptr);
}

//-----------------------------------------------------------------------------
void svtkCompositeTransferFunctionItem::ComputeTexture()
{
  this->Superclass::ComputeTexture();
  double screenBounds[4];
  this->GetBounds(screenBounds);
  if (screenBounds[0] == screenBounds[1] || !this->OpacityFunction)
  {
    return;
  }
  if (this->Texture == nullptr)
  {
    this->Texture = svtkImageData::New();
  }

  double dataBounds[4];
  this->TransformScreenToData(screenBounds[0], screenBounds[2], dataBounds[0], dataBounds[2]);
  this->TransformScreenToData(screenBounds[1], screenBounds[3], dataBounds[1], dataBounds[3]);

  const bool logX = this->GetXAxis()->GetLogScaleActive();
  const bool logY = this->GetYAxis()->GetLogScaleActive();

  const int dimension = this->GetTextureWidth();
  std::vector<double> values(dimension);
  this->OpacityFunction->GetTable(
    dataBounds[0], dataBounds[1], dimension, values.data(), 1, logX ? 1 : 0);
  unsigned char* ptr = reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0, 0, 0));

  // TBD: maybe the shape should be defined somewhere else...
  if (this->MaskAboveCurve || this->PolyLinePen->GetLineType() != svtkPen::SOLID_LINE)
  {
    this->Shape->SetNumberOfPoints(dimension);
    const double step = (dataBounds[1] - dataBounds[0]) / dimension;

    for (int i = 0; i < dimension; ++i)
    {
      if (values[i] < 0. || values[i] > 1.)
      {
        svtkWarningMacro(<< "Opacity at point " << i << " is " << values[i]
                        << " which is outside the valid range of [0,1]");
      }
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);

      double xValue = dataBounds[0] + step * i;
      double yValue = values[i];
      if (logY)
      {
        yValue = std::log10(yValue);
      }
      this->Shape->SetPoint(i, xValue, yValue);
      ptr += 4;
    }
  }
  else
  {
    for (int i = 0; i < dimension; ++i)
    {
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
      assert(values[i] <= 1. && values[i] >= 0.);
      ptr += 4;
    }
  }
}
