/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenQubeElectronicData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenQubeElectronicData.h"

#include "svtkDataSetCollection.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"

#include <openqube/basisset.h>
#include <openqube/cube.h>

// Internal class to store queue/qube information along with the image
class OQEDImageData : public svtkImageData
{
public:
  svtkTypeMacro(OQEDImageData, svtkImageData);
  static OQEDImageData* New();

  svtkSetMacro(OrbitalNumber, svtkIdType);
  svtkGetMacro(OrbitalNumber, svtkIdType);

  svtkSetMacro(ImageType, OpenQube::Cube::Type);
  svtkGetMacro(ImageType, OpenQube::Cube::Type);

  svtkSetMacro(MetaSpacing, float);
  svtkGetMacro(MetaSpacing, float);

  svtkSetMacro(MetaPadding, float);
  svtkGetMacro(MetaPadding, float);

  virtual void DeepCopy(svtkDataObject* src)
  {
    this->Superclass::DeepCopy(src);
    OQEDImageData* o = OQEDImageData::SafeDownCast(src);
    if (!o)
    {
      // Just return without copying metadata
      return;
    }
    this->OrbitalNumber = o->OrbitalNumber;
    this->ImageType = o->ImageType;
    this->MetaSpacing = o->MetaSpacing;
    this->MetaPadding = o->MetaPadding;
  }

protected:
  OQEDImageData() {}
  ~OQEDImageData() {}

  svtkIdType OrbitalNumber;
  OpenQube::Cube::Type ImageType;

  float MetaSpacing;
  float MetaPadding;

private:
  OQEDImageData(const OQEDImageData&) = delete;
  void operator=(const OQEDImageData&) = delete;
};
svtkStandardNewMacro(OQEDImageData);

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenQubeElectronicData);

//----------------------------------------------------------------------------
svtkOpenQubeElectronicData::svtkOpenQubeElectronicData()
  : BasisSet(nullptr)
  , Spacing(0.1)
{
  this->Padding = 2.0;
}

//----------------------------------------------------------------------------
svtkOpenQubeElectronicData::~svtkOpenQubeElectronicData() {}

//----------------------------------------------------------------------------
void svtkOpenQubeElectronicData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BasisSet: @" << this->BasisSet << "\n";

  // Dump Images
  os << indent << "Images: @" << this->Images << "\n";
  svtkCollectionSimpleIterator cookie;
  this->Images->InitTraversal(cookie);
  while (svtkDataSet* dataset = this->Images->GetNextDataSet(cookie))
  {
    svtkImageData* data = svtkImageData::SafeDownCast(dataset);
    // If this is somehow not a svtkImageData, print a warning and skip it
    if (!data)
    {
      svtkWarningMacro(<< "svtkDataSet in this->Images is not a svtkImageData"
                         " object. This should not happen...");
      continue;
    }

    OQEDImageData* oqeddata = OQEDImageData::SafeDownCast(dataset);
    // Identify the type of image
    if (oqeddata)
    {
      switch (oqeddata->GetImageType())
      {
        case OpenQube::Cube::MO:
          os << indent << "this->Images has molecular orbital #" << oqeddata->GetOrbitalNumber()
             << " imagedata @" << oqeddata << ":\n";
          break;
        case OpenQube::Cube::ElectronDensity:
          os << indent
             << "this->Images has electron density imagedata "
                "@"
             << oqeddata << ":\n";
          break;
        case OpenQube::Cube::VdW:
          os << indent
             << "this->Images has van der Waals imagedata"
                " @"
             << oqeddata << ":\n";
          break;
        case OpenQube::Cube::ESP:
          os << indent
             << "this->Images has electrostatic potential imagedata"
                " @"
             << oqeddata << ":\n";
          break;
        case OpenQube::Cube::FromFile:
          os << indent
             << "this->Images has file-loaded imagedata"
                " @"
             << oqeddata << ":\n";
          break;
        default:
        case OpenQube::Cube::None:
          os << indent
             << "this->Images has imagedata from an unknown source"
                " @"
             << oqeddata << ":\n";
          break;
      }
    }
    else // oqeddata null
    {
      os << indent
         << "this->Images has imagedata that was externally "
            "generated @"
         << data << ":\n";
    }
    data->PrintSelf(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkOpenQubeElectronicData::GetNumberOfMOs()
{
  if (!this->BasisSet || !this->BasisSet->isValid())
  {
    return 0;
  }

  return this->BasisSet->numMOs();
}

//----------------------------------------------------------------------------
unsigned int svtkOpenQubeElectronicData::GetNumberOfElectrons()
{
  if (!this->BasisSet || !this->BasisSet->isValid())
  {
    return 0;
  }

  return this->BasisSet->numElectrons();
}

//----------------------------------------------------------------------------
svtkImageData* svtkOpenQubeElectronicData::GetMO(svtkIdType orbitalNumber)
{
  svtkDebugMacro(<< "Searching for MO " << orbitalNumber);
  // First check if there is an existing image for this orbital
  svtkCollectionSimpleIterator cookie;
  this->Images->InitTraversal(cookie);
  while (svtkDataSet* dataset = this->Images->GetNextDataSet(cookie))
  {
    OQEDImageData* data = OQEDImageData::SafeDownCast(dataset);
    if (!data)
    {
      continue;
    }

    if (data->GetImageType() != OpenQube::Cube::MO)
    {
      continue;
    }

    if (data->GetOrbitalNumber() != orbitalNumber)
    {
      continue;
    }

    if (data->GetMetaSpacing() != this->Spacing || data->GetMetaPadding() != this->Padding)
    {
      continue;
    }

    svtkDebugMacro(<< "Found MO " << orbitalNumber);
    return static_cast<svtkImageData*>(data);
  }

  // If there is no existing image, calculate it
  svtkDebugMacro(<< "MO " << orbitalNumber << " not found. Calculating...");
  return this->CalculateMO(orbitalNumber);
}

//----------------------------------------------------------------------------
svtkImageData* svtkOpenQubeElectronicData::GetElectronDensity()
{
  // First check if there is an existing image for this orbital
  svtkCollectionSimpleIterator cookie;
  this->Images->InitTraversal(cookie);
  while (svtkDataSet* dataset = this->Images->GetNextDataSet(cookie))
  {
    OQEDImageData* data = OQEDImageData::SafeDownCast(dataset);
    if (!data)
    {
      continue;
    }

    if (data->GetImageType() != OpenQube::Cube::ElectronDensity)
    {
      continue;
    }

    if (data->GetMetaSpacing() != this->Spacing || data->GetMetaPadding() != this->Padding)
    {
      continue;
    }

    return static_cast<svtkImageData*>(data);
  }

  // If there is no existing image, calculate it
  return this->CalculateElectronDensity();
}

//----------------------------------------------------------------------------
void svtkOpenQubeElectronicData::DeepCopy(svtkDataObject* obj)
{
  svtkOpenQubeElectronicData* oqed = svtkOpenQubeElectronicData::SafeDownCast(obj);
  if (!oqed)
  {
    svtkErrorMacro("Can only deep copy from svtkOpenQubeElectronicData "
                  "or subclass.");
    return;
  }

  // Call superclass
  this->Superclass::DeepCopy(oqed);

  // Copy all images over by hand to get OQEDImageData metadata right
  svtkCollectionSimpleIterator cookie;
  oqed->Images->InitTraversal(cookie);
  while (svtkDataSet* dataset = oqed->Images->GetNextDataSet(cookie))
  {
    // Only copy valid OQEDImageData objects
    if (OQEDImageData* oqedImage = OQEDImageData::SafeDownCast(dataset))
    {
      svtkNew<OQEDImageData> thisImage;
      thisImage->DeepCopy(oqedImage);
      this->Images->AddItem(thisImage);
    }
  }

  // Copy other ivars
  if (oqed->BasisSet)
  {
    this->BasisSet = oqed->BasisSet->clone();
  }
  this->Spacing = oqed->Spacing;
}

//----------------------------------------------------------------------------
svtkImageData* svtkOpenQubeElectronicData::CalculateMO(svtkIdType orbitalNumber)
{
  svtkDebugMacro(<< "Calculating MO " << orbitalNumber);
  if (!this->BasisSet)
  {
    svtkWarningMacro(<< "No OpenQube::BasisSet set.");
    return 0;
  }
  else if (!this->BasisSet->isValid())
  {
    svtkWarningMacro(<< "Invalid OpenQube::BasisSet set.");
    return 0;
  }

  // Create and calculate cube
  OpenQube::Cube* cube = new OpenQube::Cube();
  cube->setLimits(this->BasisSet->moleculeRef(), this->Spacing, this->Padding);

  svtkDebugMacro(<< "Calculating OpenQube::Cube for MO " << orbitalNumber);
  if (!this->BasisSet->blockingCalculateCubeMO(cube, orbitalNumber))
  {
    svtkWarningMacro(<< "Unable to calculateMO for orbital " << orbitalNumber << " in OpenQube.");
    return 0;
  }

  // Create image and set metadata
  svtkNew<OQEDImageData> image;
  image->SetMetaSpacing(this->Spacing);
  image->SetMetaPadding(this->Padding);
  svtkDebugMacro(<< "Converting OpenQube::Cube to svtkImageData for MO " << orbitalNumber);

  // Copy cube --> image
  this->FillImageDataFromQube(cube, image);
  image->SetOrbitalNumber(orbitalNumber);

  svtkDebugMacro(<< "Adding svtkImageData to this->Images for MO " << orbitalNumber);
  this->Images->AddItem(image);

  return image;
}

//----------------------------------------------------------------------------
svtkImageData* svtkOpenQubeElectronicData::CalculateElectronDensity()
{
  svtkDebugMacro(<< "Calculating electron density...");
  if (!this->BasisSet)
  {
    svtkErrorMacro(<< "No OpenQube::BasisSet set.");
    return 0;
  }
  else if (!this->BasisSet->isValid())
  {
    svtkErrorMacro(<< "Invalid OpenQube::BasisSet set.");
    return 0;
  }

  // Create and calculate cube
  OpenQube::Cube* cube = new OpenQube::Cube();
  cube->setLimits(this->BasisSet->moleculeRef(), this->Spacing, this->Padding);

  svtkDebugMacro(<< "Calculating OpenQube::Cube...");
  if (!this->BasisSet->blockingCalculateCubeDensity(cube))
  {
    svtkWarningMacro(<< "Unable to calculate density in OpenQube.");
    return 0;
  }

  // Create image and set metadata
  svtkNew<OQEDImageData> image;
  image->SetMetaSpacing(this->Spacing);
  image->SetMetaPadding(this->Padding);
  svtkDebugMacro(<< "Converting OpenQube::Cube to svtkImageData.");

  // Copy cube --> image
  this->FillImageDataFromQube(cube, image);

  svtkDebugMacro(<< "Adding svtkImageData to this->Images");
  this->Images->AddItem(image);

  return image;
}

//----------------------------------------------------------------------------
void svtkOpenQubeElectronicData::FillImageDataFromQube(OpenQube::Cube* qube, svtkImageData* image)
{
  Eigen::Vector3i dim = qube->dimensions();
  Eigen::Vector3d min = qube->min();
  Eigen::Vector3d max = qube->max();
  Eigen::Vector3d spacing = qube->spacing();

  svtkDebugMacro(<< "Converting OpenQube::Cube to svtkImageData:"
                << "\n\tDimensions: " << dim[0] << "  " << dim[1] << " " << dim[2]
                << "\n\tMinimum: " << min[0] << "  " << min[1] << " " << min[2]
                << "\n\tMaximum: " << max[0] << "  " << max[1] << " " << max[2]
                << "\n\tSpacing: " << spacing[0] << "  " << spacing[1] << " " << spacing[2]);

  if (OQEDImageData* oqedImage = OQEDImageData::SafeDownCast(image))
  {
    svtkDebugMacro("Setting cube type to " << qube->cubeType());
    oqedImage->SetImageType(qube->cubeType());
  }
  else
  {
    svtkWarningMacro(<< "Cannot cast input image to internal OQEDImageData type.");
  }

  image->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  image->SetOrigin(min.data());
  image->SetSpacing(spacing.data());
  image->AllocateScalars(SVTK_DOUBLE, 1);

  double* dataPtr = static_cast<double*>(image->GetScalarPointer());
  std::vector<double>* qubeVec = qube->data();

  const size_t qubeSize = qubeVec->size();

  if (qubeSize != static_cast<size_t>(dim[0]) * dim[1] * dim[2])
  {
    svtkWarningMacro(<< "Size of qube (" << qubeSize << ") does not equal"
                    << " product of dimensions (" << dim[0] * dim[1] * dim[2]
                    << "). Image may not be accurate.");
  }

  long int qubeInd = -1; // Incremented to 0 on first use.
  for (int i = 0; i < dim[0]; ++i)
  {
    for (int j = 0; j < dim[1]; ++j)
    {
      for (int k = 0; k < dim[2]; ++k)
      {
        dataPtr[(k * dim[1] + j) * dim[0] + i] = (*qubeVec)[++qubeInd];
      }
    }
  }

  svtkDebugMacro(<< "Copied " << qubeSize << " (actual: " << qubeInd + 1
                << ") points from qube to svtkImageData.");
}
