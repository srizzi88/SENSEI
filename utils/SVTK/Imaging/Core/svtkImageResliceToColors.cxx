/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageResliceToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageResliceToColors.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTypeTraits.h"

#include "svtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types
#undef SVTK_USE_INT64
#define SVTK_USE_INT64 0
#undef SVTK_USE_UINT64
#define SVTK_USE_UINT64 0

#include <cfloat>
#include <climits>
#include <cmath>

svtkStandardNewMacro(svtkImageResliceToColors);
svtkCxxSetObjectMacro(svtkImageResliceToColors, LookupTable, svtkScalarsToColors);

//----------------------------------------------------------------------------
svtkImageResliceToColors::svtkImageResliceToColors()
{
  this->HasConvertScalars = 1;
  this->LookupTable = nullptr;
  this->DefaultLookupTable = nullptr;
  this->OutputFormat = SVTK_RGBA;
  this->Bypass = 0;
}

//----------------------------------------------------------------------------
svtkImageResliceToColors::~svtkImageResliceToColors()
{
  if (this->LookupTable)
  {
    this->LookupTable->Delete();
  }
  if (this->DefaultLookupTable)
  {
    this->DefaultLookupTable->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkImageResliceToColors::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LookupTable: " << this->GetLookupTable() << "\n";
  os << indent << "OutputFormat: "
     << (this->OutputFormat == SVTK_RGBA
            ? "RGBA"
            : (this->OutputFormat == SVTK_RGB
                  ? "RGB"
                  : (this->OutputFormat == SVTK_LUMINANCE_ALPHA
                        ? "LuminanceAlpha"
                        : (this->OutputFormat == SVTK_LUMINANCE ? "Luminance" : "Unknown"))))
     << "\n";
  os << indent << "Bypass: " << (this->Bypass ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
svtkMTimeType svtkImageResliceToColors::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->LookupTable && !this->Bypass)
  {
    svtkMTimeType time = this->LookupTable->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkImageResliceToColors::SetBypass(int bypass)
{
  bypass = (bypass != 0);
  if (bypass != this->Bypass)
  {
    this->Bypass = bypass;
    if (bypass)
    {
      this->HasConvertScalars = 0;
      this->OutputScalarType = SVTK_FLOAT;
    }
    else
    {
      this->HasConvertScalars = 1;
      this->OutputScalarType = -1;
    }
  }
}

//----------------------------------------------------------------------------
int svtkImageResliceToColors::ConvertScalarInfo(int& scalarType, int& numComponents)
{
  switch (this->OutputFormat)
  {
    case SVTK_LUMINANCE:
      numComponents = 1;
      break;
    case SVTK_LUMINANCE_ALPHA:
      numComponents = 2;
      break;
    case SVTK_RGB:
      numComponents = 3;
      break;
    case SVTK_RGBA:
      numComponents = 4;
      break;
  }

  scalarType = SVTK_UNSIGNED_CHAR;

  // This is always called before ConvertScalars, and is
  // not called multi-threaded, so set up default table here
  if (this->LookupTable)
  {
    this->LookupTable->Build();
  }
  else if (!this->DefaultLookupTable)
  {
    // Build a default greyscale lookup table
    this->DefaultLookupTable = svtkScalarsToColors::New();
    this->DefaultLookupTable->SetRange(0.0, 255.0);
    this->DefaultLookupTable->SetVectorModeToRGBColors();
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkImageResliceToColors::ConvertScalars(void* inPtr, void* outPtr, int inputType,
  int inputComponents, int count, int svtkNotUsed(idX), int svtkNotUsed(idY), int svtkNotUsed(idZ),
  int svtkNotUsed(threadId))
{
  svtkScalarsToColors* table = this->LookupTable;
  if (!table)
  {
    table = this->DefaultLookupTable;
  }

  if (inputComponents == 1 && this->LookupTable)
  {
    table->MapScalarsThroughTable(inPtr, static_cast<unsigned char*>(outPtr), inputType, count,
      inputComponents, this->OutputFormat);
  }
  else
  {
    table->MapVectorsThroughTable(inPtr, static_cast<unsigned char*>(outPtr), inputType, count,
      inputComponents, this->OutputFormat);
  }
}
