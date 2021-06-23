/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLookupTableItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkLookupTableItem.h"
#include "svtkCallbackCommand.h"
#include "svtkImageData.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkLookupTableItem);

//-----------------------------------------------------------------------------
svtkLookupTableItem::svtkLookupTableItem()
{
  this->Interpolate = false;
  this->LookupTable = nullptr;
}

//-----------------------------------------------------------------------------
svtkLookupTableItem::~svtkLookupTableItem()
{
  if (this->LookupTable)
  {
    this->LookupTable->Delete();
    this->LookupTable = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkLookupTableItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LookupTable: ";
  if (this->LookupTable)
  {
    os << endl;
    this->LookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//-----------------------------------------------------------------------------
void svtkLookupTableItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->LookupTable)
  {
    double* range = this->LookupTable->GetRange();
    bounds[0] = range[0];
    bounds[1] = range[1];
  }
}

//-----------------------------------------------------------------------------
void svtkLookupTableItem::SetLookupTable(svtkLookupTable* t)
{
  if (t == this->LookupTable)
  {
    return;
  }
  if (this->LookupTable)
  {
    this->LookupTable->RemoveObserver(this->Callback);
  }
  svtkSetObjectBodyMacro(LookupTable, svtkLookupTable, t);
  if (t)
  {
    t->AddObserver(svtkCommand::ModifiedEvent, this->Callback);
  }
  this->ScalarsToColorsModified(this->LookupTable, svtkCommand::ModifiedEvent, nullptr);
}

//-----------------------------------------------------------------------------
void svtkLookupTableItem::ComputeTexture()
{
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1] || !this->LookupTable)
  {
    return;
  }
  if (this->Texture == nullptr)
  {
    this->Texture = svtkImageData::New();
  }
  // Could depend of the screen resolution
  const int dimension = 256;
  double values[256];
  // Texture 1D
  this->Texture->SetExtent(0, dimension - 1, 0, 0, 0, 0);
  this->Texture->AllocateScalars(SVTK_UNSIGNED_CHAR, 4);
  // TODO: Support log scale ?
  for (int i = 0; i < dimension; ++i)
  {
    values[i] = bounds[0] + i * (bounds[1] - bounds[0]) / (dimension - 1);
  }
  unsigned char* ptr = reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0, 0, 0));
  this->LookupTable->MapScalarsThroughTable2(values, ptr, SVTK_DOUBLE, dimension, 1, 4);
  if (this->Opacity != 1.)
  {
    for (int i = 0; i < dimension; ++i)
    {
      ptr[3] = static_cast<unsigned char>(this->Opacity * ptr[3]);
      ptr += 4;
    }
  }
}
