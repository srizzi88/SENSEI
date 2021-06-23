/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageProperty.h"

#include "svtkColorTransferFunction.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkImageProperty);

svtkCxxSetObjectMacro(svtkImageProperty, LookupTable, svtkScalarsToColors);

//----------------------------------------------------------------------------
// Construct a new svtkImageProperty with default values
svtkImageProperty::svtkImageProperty()
{
  this->ColorWindow = 255.0;
  this->ColorLevel = 127.5;

  this->LookupTable = nullptr;
  this->UseLookupTableScalarRange = 0;

  this->Opacity = 1.0;
  this->Ambient = 1.0;
  this->Diffuse = 0.0;

  this->InterpolationType = SVTK_LINEAR_INTERPOLATION;

  this->LayerNumber = 0;

  this->Checkerboard = 0;
  this->CheckerboardSpacing[0] = 10.0;
  this->CheckerboardSpacing[1] = 10.0;
  this->CheckerboardOffset[0] = 0.0;
  this->CheckerboardOffset[1] = 0.0;

  this->Backing = 0;
  this->BackingColor[0] = 0.0;
  this->BackingColor[1] = 0.0;
  this->BackingColor[2] = 0.0;
}

//----------------------------------------------------------------------------
// Destruct a svtkImageProperty
svtkImageProperty::~svtkImageProperty()
{
  if (this->LookupTable != nullptr)
  {
    this->LookupTable->Delete();
  }
}

//----------------------------------------------------------------------------
const char* svtkImageProperty::GetInterpolationTypeAsString()
{
  switch (this->InterpolationType)
  {
    case SVTK_NEAREST_INTERPOLATION:
      return "Nearest";
    case SVTK_LINEAR_INTERPOLATION:
      return "Linear";
    case SVTK_CUBIC_INTERPOLATION:
      return "Cubic";
  }
  return "";
}

//----------------------------------------------------------------------------
void svtkImageProperty::DeepCopy(svtkImageProperty* p)
{
  if (p != nullptr)
  {
    this->SetColorWindow(p->GetColorWindow());
    this->SetColorLevel(p->GetColorLevel());
    svtkScalarsToColors* lut = p->GetLookupTable();
    if (lut == nullptr)
    {
      this->SetLookupTable(nullptr);
    }
    else
    {
      svtkScalarsToColors* nlut = lut->NewInstance();
      nlut->DeepCopy(lut);
      this->SetLookupTable(nlut);
      nlut->Delete();
    }
    this->SetUseLookupTableScalarRange(p->GetUseLookupTableScalarRange());
    this->SetOpacity(p->GetOpacity());
    this->SetAmbient(p->GetAmbient());
    this->SetDiffuse(p->GetDiffuse());
    this->SetInterpolationType(p->GetInterpolationType());
    this->SetCheckerboard(p->GetCheckerboard());
    this->SetCheckerboardSpacing(p->GetCheckerboardSpacing());
    this->SetCheckerboardOffset(p->GetCheckerboardOffset());
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkImageProperty::GetMTime()
{
  svtkMTimeType mTime = this->svtkObject::GetMTime();
  svtkMTimeType time;

  if (this->LookupTable)
  {
    time = this->LookupTable->GetMTime();
    mTime = (mTime > time ? mTime : time);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkImageProperty::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ColorWindow: " << this->ColorWindow << "\n";
  os << indent << "ColorLevel: " << this->ColorLevel << "\n";
  os << indent
     << "UseLookupTableScalarRange: " << (this->UseLookupTableScalarRange ? "On\n" : "Off\n");
  os << indent << "LookupTable: " << this->LookupTable << "\n";
  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Ambient: " << this->Ambient << "\n";
  os << indent << "Diffuse: " << this->Diffuse << "\n";
  os << indent << "InterpolationType: " << this->GetInterpolationTypeAsString() << "\n";
  os << indent << "LayerNumber: " << this->LayerNumber << "\n";
  os << indent << "Checkerboard: " << (this->Checkerboard ? "On\n" : "Off\n");
  os << indent << "CheckerboardSpacing: " << this->CheckerboardSpacing[0] << " "
     << this->CheckerboardSpacing[1] << "\n";
  os << indent << "CheckerboardOffset: " << this->CheckerboardOffset[0] << " "
     << this->CheckerboardOffset[1] << "\n";
  os << indent << "Backing: " << (this->Backing ? "On\n" : "Off\n");
  os << indent << "BackingColor: " << this->BackingColor[0] << " " << this->BackingColor[1] << " "
     << this->BackingColor[2] << "\n";
}
