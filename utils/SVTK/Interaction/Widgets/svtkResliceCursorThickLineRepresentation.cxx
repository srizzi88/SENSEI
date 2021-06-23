/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorThickLineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResliceCursorThickLineRepresentation.h"
#include "svtkImageData.h"
#include "svtkImageMapToColors.h"
#include "svtkImageReslice.h"
#include "svtkImageSlabReslice.h"
#include "svtkObjectFactory.h"
#include "svtkResliceCursor.h"
#include <algorithm>
#include <cmath>

#include <sstream>

svtkStandardNewMacro(svtkResliceCursorThickLineRepresentation);

//----------------------------------------------------------------------
svtkResliceCursorThickLineRepresentation::svtkResliceCursorThickLineRepresentation()
{
  this->CreateDefaultResliceAlgorithm();
}

//----------------------------------------------------------------------
svtkResliceCursorThickLineRepresentation::~svtkResliceCursorThickLineRepresentation() = default;

//----------------------------------------------------------------------
void svtkResliceCursorThickLineRepresentation::CreateDefaultResliceAlgorithm()
{
  if (this->Reslice)
  {
    this->Reslice->Delete();
  }

  // Override superclass implementation to create a svtkImageSlabReslice here.
  this->Reslice = svtkImageSlabReslice::New();
}

//----------------------------------------------------------------------------
void svtkResliceCursorThickLineRepresentation ::SetResliceParameters(
  double outputSpacingX, double outputSpacingY, int extentX, int extentY)
{
  svtkImageSlabReslice* thickReslice = svtkImageSlabReslice::SafeDownCast(this->Reslice);

  if (thickReslice)
  {

    // Set the default color the minimum scalar value
    double range[2];
    svtkImageData::SafeDownCast(thickReslice->GetInput())->GetScalarRange(range);
    thickReslice->SetBackgroundLevel(range[0]);

    // Set the usual parameters.

    this->ColorMap->SetInputConnection(thickReslice->GetOutputPort());
    thickReslice->TransformInputSamplingOff();
    thickReslice->SetResliceAxes(this->ResliceAxes);
    thickReslice->SetOutputSpacing(outputSpacingX, outputSpacingY, 1);
    thickReslice->SetOutputOrigin(0.5 * outputSpacingX, 0.5 * outputSpacingY, 0);
    thickReslice->SetOutputExtent(0, extentX - 1, 0, extentY - 1, 0, 0);

    svtkResliceCursor* rc = this->GetResliceCursor();
    thickReslice->SetSlabThickness(rc->GetThickness()[0]);
    double spacing[3];
    rc->GetImage()->GetSpacing(spacing);

    // Perhaps we should multiply this by 0.5 for nyquist
    const double minSpacing = std::min(std::min(spacing[0], spacing[1]), spacing[2]);

    // Set the slab resolution the minimum spacing. Reasonable default
    thickReslice->SetSlabResolution(minSpacing);
  }
}

//----------------------------------------------------------------------
void svtkResliceCursorThickLineRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
