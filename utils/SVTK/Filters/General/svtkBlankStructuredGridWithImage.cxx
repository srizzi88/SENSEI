/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlankStructuredGridWithImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBlankStructuredGridWithImage.h"

#include "svtkCellData.h"
#include "svtkExecutive.h"
#include "svtkFieldData.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStructuredGrid.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkBlankStructuredGridWithImage);

//----------------------------------------------------------------------------
svtkBlankStructuredGridWithImage::svtkBlankStructuredGridWithImage()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkBlankStructuredGridWithImage::~svtkBlankStructuredGridWithImage() = default;

//----------------------------------------------------------------------------
// Specify the input data or filter.
void svtkBlankStructuredGridWithImage::SetBlankingInputData(svtkImageData* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
svtkImageData* svtkBlankStructuredGridWithImage::GetBlankingInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int svtkBlankStructuredGridWithImage::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* imageInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkStructuredGrid* grid =
    svtkStructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* image = svtkImageData::SafeDownCast(imageInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkStructuredGrid* output =
    svtkStructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int gridDims[3], imageDims[3];

  svtkDebugMacro(<< "Adding image blanking");

  // Perform error checking
  grid->GetDimensions(gridDims);
  image->GetDimensions(imageDims);
  if (gridDims[0] != imageDims[0] || gridDims[1] != imageDims[1] || gridDims[2] != imageDims[2])
  {
    svtkErrorMacro("Blanking dimensions must be identical with grid dimensions. "
                  "Blanking dimensions are "
      << imageDims[0] << " " << imageDims[1] << " " << imageDims[2] << ". Grid dimensions are "
      << gridDims[0] << " " << gridDims[1] << " " << gridDims[2] << ".");
    return 1;
  }

  if (image->GetScalarType() != SVTK_UNSIGNED_CHAR || image->GetNumberOfScalarComponents() != 1)
  {
    svtkErrorMacro(<< "This filter requires unsigned char images with one component");
    return 1;
  }

  // Get the image, set it as the blanking array.
  unsigned char* data = static_cast<unsigned char*>(image->GetScalarPointer());
  svtkUnsignedCharArray* visibility = svtkUnsignedCharArray::New();
  const svtkIdType numberOfValues = gridDims[0] * gridDims[1] * gridDims[2];
  visibility->SetArray(data, numberOfValues, 1);
  svtkUnsignedCharArray* ghosts = svtkUnsignedCharArray::New();
  ghosts->SetNumberOfValues(numberOfValues);
  ghosts->SetName(svtkDataSetAttributes::GhostArrayName());
  for (svtkIdType ptId = 0; ptId < numberOfValues; ++ptId)
  {
    unsigned char value = 0;
    if (visibility->GetValue(ptId) == 0)
    {
      value |= svtkDataSetAttributes::HIDDENPOINT;
    }
    ghosts->SetValue(ptId, value);
  }
  output->CopyStructure(grid);
  output->GetPointData()->PassData(grid->GetPointData());
  output->GetCellData()->PassData(grid->GetCellData());
  output->GetPointData()->AddArray(ghosts);
  ghosts->Delete();
  visibility->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkBlankStructuredGridWithImage::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    return this->Superclass::FillInputPortInformation(port, info);
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void svtkBlankStructuredGridWithImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
