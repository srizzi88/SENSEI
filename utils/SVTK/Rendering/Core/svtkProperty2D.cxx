/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProperty2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProperty2D.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkProperty2D);

// Creates a svtkProperty2D with the following default values:
// Opacity 1, Color (1,0,0), CompositingOperator SVTK_SRC
svtkProperty2D::svtkProperty2D()
{
  this->Opacity = 1.0;
  this->PointSize = 1.0;
  this->LineWidth = 1.0;
  this->LineStipplePattern = 0xFFFF;
  this->LineStippleRepeatFactor = 1;
  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;
  this->DisplayLocation = SVTK_FOREGROUND_LOCATION;
}

svtkProperty2D::~svtkProperty2D() = default;

// Assign one property to another.
void svtkProperty2D::DeepCopy(svtkProperty2D* p)
{
  if (p != nullptr)
  {
    this->SetColor(p->GetColor());
    this->SetOpacity(p->GetOpacity());
    this->SetPointSize(p->GetPointSize());
    this->SetLineWidth(p->GetLineWidth());
    this->SetLineStipplePattern(p->GetLineStipplePattern());
    this->SetLineStippleRepeatFactor(p->GetLineStippleRepeatFactor());
    this->SetDisplayLocation(p->GetDisplayLocation());
  }
}

void svtkProperty2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Color: (" << this->Color[0] << ", " << this->Color[1] << ", " << this->Color[2]
     << ")\n";
  os << indent << "Point size: " << this->PointSize << "\n";
  os << indent << "Line width: " << this->LineWidth << "\n";
  os << indent << "Line stipple pattern: " << this->LineStipplePattern << "\n";
  os << indent << "Line stipple repeat factor: " << this->LineStippleRepeatFactor << "\n";
  switch (this->DisplayLocation)
  {
    case SVTK_FOREGROUND_LOCATION:
      os << indent << "Display location: foreground\n";
      break;
    case SVTK_BACKGROUND_LOCATION:
      os << indent << "Display location: background\n";
      break;
    default:
      os << indent << "Display location: invalid\n";
      break;
  }
}
