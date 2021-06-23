/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTexture.h"

#include "svtkDataSetAttributes.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"

svtkCxxSetObjectMacro(svtkTexture, LookupTable, svtkScalarsToColors);
//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
svtkAbstractObjectFactoryNewMacro(svtkTexture);
//----------------------------------------------------------------------------

// Construct object and initialize.
svtkTexture::svtkTexture()
{
  this->Mipmap = false;
  this->Repeat = 1;
  this->Interpolate = 0;
  this->EdgeClamp = 0;
  this->MaximumAnisotropicFiltering = 4.0;
  this->Quality = SVTK_TEXTURE_QUALITY_DEFAULT;
  this->PremultipliedAlpha = false;
  this->CubeMap = false;
  this->UseSRGBColorSpace = false;

  this->LookupTable = nullptr;
  this->MappedScalars = nullptr;
  this->ColorMode = SVTK_COLOR_MODE_DEFAULT;
  this->Transform = nullptr;

  this->SelfAdjustingTableRange = 0;

  this->SetNumberOfOutputPorts(0);

  this->BlendingMode = SVTK_TEXTURE_BLENDING_MODE_NONE;

  this->RestrictPowerOf2ImageSmaller = 0;

  // By default select active point scalars.
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkTexture::~svtkTexture()
{
  if (this->MappedScalars)
  {
    this->MappedScalars->Delete();
  }

  if (this->LookupTable != nullptr)
  {
    this->LookupTable->UnRegister(this);
  }

  if (this->Transform != nullptr)
  {
    this->Transform->UnRegister(this);
  }
}

//----------------------------------------------------------------------------
svtkImageData* svtkTexture::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void svtkTexture::SetCubeMap(bool val)
{
  if (val == this->CubeMap)
  {
    return;
  }

  if (val)
  {
    this->SetNumberOfInputPorts(6);
    for (int i = 0; i < 6; ++i)
    {
      this->SetInputArrayToProcess(
        i, i, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, svtkDataSetAttributes::SCALARS);
    }
  }
  else
  {
    this->SetNumberOfInputPorts(1);
  }
  this->CubeMap = val;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkTexture::SetTransform(svtkTransform* transform)
{
  if (transform == this->Transform)
  {
    return;
  }

  if (this->Transform)
  {
    this->Transform->Delete();
    this->Transform = nullptr;
  }

  if (transform)
  {
    this->Transform = transform;
    this->Transform->Register(this);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkTexture::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaximumAnisotropicFiltering: " << this->MaximumAnisotropicFiltering << "\n";
  os << indent << "Mipmap: " << (this->Mipmap ? "On\n" : "Off\n");
  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");
  os << indent << "Repeat:      " << (this->Repeat ? "On\n" : "Off\n");
  os << indent << "EdgeClamp:   " << (this->EdgeClamp ? "On\n" : "Off\n");
  os << indent << "CubeMap:   " << (this->CubeMap ? "On\n" : "Off\n");
  os << indent << "UseSRGBColorSpace:   " << (this->UseSRGBColorSpace ? "On\n" : "Off\n");
  os << indent << "Quality:     ";
  switch (this->Quality)
  {
    case SVTK_TEXTURE_QUALITY_DEFAULT:
      os << "Default\n";
      break;
    case SVTK_TEXTURE_QUALITY_16BIT:
      os << "16Bit\n";
      break;
    case SVTK_TEXTURE_QUALITY_32BIT:
      os << "32Bit\n";
      break;
  }
  os << indent << "ColorMode: ";
  switch (this->ColorMode)
  {
    case SVTK_COLOR_MODE_DEFAULT:
      os << "SVTK_COLOR_MODE_DEFAULT";
      break;
    case SVTK_COLOR_MODE_MAP_SCALARS:
      os << "SVTK_COLOR_MODE_MAP_SCALARS";
      break;
    case SVTK_COLOR_MODE_DIRECT_SCALARS:
    default:
      os << "SVTK_COLOR_MODE_DIRECT_SCALARS";
      break;
  }
  os << "\n";

  os << indent << "PremultipliedAlpha: " << (this->PremultipliedAlpha ? "On\n" : "Off\n");

  if (this->GetInput())
  {
    os << indent << "Input: (" << static_cast<void*>(this->GetInput()) << ")\n";
  }
  else
  {
    os << indent << "Input: (none)\n";
  }
  if (this->LookupTable)
  {
    os << indent << "LookupTable:\n";
    this->LookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "LookupTable: (none)\n";
  }

  if (this->MappedScalars)
  {
    os << indent << "Mapped Scalars: " << this->MappedScalars << "\n";
  }
  else
  {
    os << indent << "Mapped Scalars: (none)\n";
  }

  if (this->Transform)
  {
    os << indent << "Transform: " << this->Transform << "\n";
  }
  else
  {
    os << indent << "Transform: (none)\n";
  }
  os << indent << "MultiTexture Blending Mode:     ";
  switch (this->BlendingMode)
  {
    case SVTK_TEXTURE_BLENDING_MODE_NONE:
      os << "None\n";
      break;
    case SVTK_TEXTURE_BLENDING_MODE_REPLACE:
      os << "Replace\n";
      break;
    case SVTK_TEXTURE_BLENDING_MODE_MODULATE:
      os << "Modulate\n";
      break;
    case SVTK_TEXTURE_BLENDING_MODE_ADD:
      os << "Add\n";
      break;
    case SVTK_TEXTURE_BLENDING_MODE_ADD_SIGNED:
      os << "Add Signed\n";
      break;
    case SVTK_TEXTURE_BLENDING_MODE_INTERPOLATE:
      os << "Interpolate\n";
      break;
    case SVTK_TEXTURE_BLENDING_MODE_SUBTRACT:
      os << "Subtract\n";
      break;
  }
  os << indent << "RestrictPowerOf2ImageSmaller:   "
     << (this->RestrictPowerOf2ImageSmaller ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
unsigned char* svtkTexture::MapScalarsToColors(svtkDataArray* scalars)
{
  // if there is no lookup table, create one
  if (this->LookupTable == nullptr)
  {
    this->LookupTable = svtkLookupTable::New();
    this->LookupTable->Register(this);
    this->LookupTable->Delete();
    this->LookupTable->Build();
    this->SelfAdjustingTableRange = 1;
  }
  else
  {
    this->SelfAdjustingTableRange = 0;
  }
  // Delete old colors
  if (this->MappedScalars)
  {
    this->MappedScalars->Delete();
    this->MappedScalars = nullptr;
  }

  // if the texture created its own lookup table, set the Table Range
  // to the range of the scalar data.
  if (this->SelfAdjustingTableRange)
  {
    this->LookupTable->SetRange(scalars->GetRange(0));
  }

  // map the scalars to colors
  this->MappedScalars = this->LookupTable->MapScalars(scalars, this->ColorMode, -1);

  return this->MappedScalars
    ? reinterpret_cast<unsigned char*>(this->MappedScalars->GetVoidPointer(0))
    : nullptr;
}

//----------------------------------------------------------------------------
void svtkTexture::Render(svtkRenderer* ren)
{
  for (int i = 0; i < this->GetNumberOfInputPorts(); ++i)
  {
    svtkAlgorithm* inputAlg = this->GetInputAlgorithm(i, 0);

    if (inputAlg) // load texture map
    {
      svtkInformation* inInfo = this->GetInputInformation();
      // We do not want more than requested.
      inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

      // Updating the whole extent may not be necessary.
      inputAlg->UpdateWholeExtent();
    }
  }
  this->Load(ren);
}

//----------------------------------------------------------------------------
int svtkTexture::IsTranslucent()
{
  if (this->GetMTime() <= this->TranslucentComputationTime &&
    (this->GetInput() == nullptr ||
      (this->GetInput()->GetMTime() <= this->TranslucentComputationTime)))
    return this->TranslucentCachedResult;

  if (this->GetInputAlgorithm())
  {
    svtkAlgorithm* inpAlg = this->GetInputAlgorithm();
    inpAlg->UpdateWholeExtent();
  }

  if (this->GetInput() == nullptr || this->GetInput()->GetPointData()->GetScalars() == nullptr ||
    this->GetInput()->GetPointData()->GetScalars()->GetNumberOfComponents() % 2)
  {
    this->TranslucentCachedResult = 0;
  }
  else
  {
    svtkDataArray* scal = this->GetInput()->GetPointData()->GetScalars();
    // the alpha component is the last one
    int alphaid = scal->GetNumberOfComponents() - 1;
    bool hasTransparentPixel = false;
    bool hasOpaquePixel = false;
    bool hasTranslucentPixel = false;
    for (svtkIdType i = 0; i < scal->GetNumberOfTuples(); i++)
    {
      double alpha = scal->GetTuple(i)[alphaid];
      if (alpha <= 0)
      {
        hasTransparentPixel = true;
      }
      else if (((scal->GetDataType() == SVTK_FLOAT || scal->GetDataType() == SVTK_DOUBLE) &&
                 alpha >= 1.0) ||
        alpha == scal->GetDataTypeMax())
      {
        hasOpaquePixel = true;
      }
      else
      {
        hasTranslucentPixel = true;
      }
      // stop the computation if there are translucent pixels
      if (hasTranslucentPixel || (this->Interpolate && hasTransparentPixel && hasOpaquePixel))
        break;
    }
    if (hasTranslucentPixel || (this->Interpolate && hasTransparentPixel && hasOpaquePixel))
    {
      this->TranslucentCachedResult = 1;
    }
    else
    {
      this->TranslucentCachedResult = 0;
    }
  }

  this->TranslucentComputationTime.Modified();
  return this->TranslucentCachedResult;
}
