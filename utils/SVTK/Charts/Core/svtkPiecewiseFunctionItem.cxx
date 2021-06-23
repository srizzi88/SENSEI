/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewiseFunctionItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPiecewiseFunctionItem.h"
#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkContext2D.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPoints2D.h"

#include <cassert>
#include <vector>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPiecewiseFunctionItem);

//-----------------------------------------------------------------------------
svtkPiecewiseFunctionItem::svtkPiecewiseFunctionItem()
{
  this->PolyLinePen->SetLineType(svtkPen::SOLID_LINE);
  this->PiecewiseFunction = nullptr;
  this->SetColor(1., 1., 1.);
}

//-----------------------------------------------------------------------------
svtkPiecewiseFunctionItem::~svtkPiecewiseFunctionItem()
{
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
    this->PiecewiseFunction->Delete();
    this->PiecewiseFunction = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkPiecewiseFunctionItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PiecewiseFunction: ";
  if (this->PiecewiseFunction)
  {
    os << endl;
    this->PiecewiseFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//-----------------------------------------------------------------------------
void svtkPiecewiseFunctionItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->PiecewiseFunction)
  {
    double* range = this->PiecewiseFunction->GetRange();
    bounds[0] = range[0];
    bounds[1] = range[1];
  }
}

//-----------------------------------------------------------------------------
void svtkPiecewiseFunctionItem::SetPiecewiseFunction(svtkPiecewiseFunction* t)
{
  if (t == this->PiecewiseFunction)
  {
    return;
  }
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
  }
  svtkSetObjectBodyMacro(PiecewiseFunction, svtkPiecewiseFunction, t);
  if (t)
  {
    t->AddObserver(svtkCommand::ModifiedEvent, this->Callback);
  }
  this->ScalarsToColorsModified(this->PiecewiseFunction, svtkCommand::ModifiedEvent, nullptr);
}

//-----------------------------------------------------------------------------
void svtkPiecewiseFunctionItem::ComputeTexture()
{
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1] || !this->PiecewiseFunction)
  {
    return;
  }
  if (this->Texture == nullptr)
  {
    this->Texture = svtkImageData::New();
  }

  const int dimension = this->GetTextureWidth();
  std::vector<double> values(dimension);
  // should depends on the true size on screen
  this->Texture->SetExtent(0, dimension - 1, 0, 0, 0, 0);
  this->Texture->AllocateScalars(SVTK_UNSIGNED_CHAR, 4);

  this->PiecewiseFunction->GetTable(bounds[0], bounds[1], dimension, values.data());
  unsigned char* ptr = reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0, 0, 0));
  if (this->MaskAboveCurve || this->PolyLinePen->GetLineType() != svtkPen::NO_PEN)
  {
    this->Shape->SetNumberOfPoints(dimension);
    double step = (bounds[1] - bounds[0]) / dimension;
    for (int i = 0; i < dimension; ++i)
    {
      this->Pen->GetColor(ptr);
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255 + 0.5);
      assert(values[i] <= 1. && values[i] >= 0.);
      this->Shape->SetPoint(i, bounds[0] + step * i, values[i]);
      ptr += 4;
    }
    this->Shape->Modified();
  }
  else
  {
    for (int i = 0; i < dimension; ++i)
    {
      this->Pen->GetColor(ptr);
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255 + 0.5);
      assert(values[i] <= 1. && values[i] >= 0.);
      ptr += 4;
    }
  }
}
