/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableElectronicData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkProgrammableElectronicData.h"

#include "svtkDataSetCollection.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

#include <vector>

// PIMPL'd std::vector
class StdVectorOfImageDataPointers : public std::vector<svtkSmartPointer<svtkImageData> >
{
};

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkProgrammableElectronicData);
svtkCxxSetObjectMacro(svtkProgrammableElectronicData, ElectronDensity, svtkImageData);

//----------------------------------------------------------------------------
svtkProgrammableElectronicData::svtkProgrammableElectronicData()
  : NumberOfElectrons(0)
  , MOs(new StdVectorOfImageDataPointers)
  , ElectronDensity(nullptr)
{
}

//----------------------------------------------------------------------------
svtkProgrammableElectronicData::~svtkProgrammableElectronicData()
{
  delete this->MOs;
  this->MOs = nullptr;

  this->SetElectronDensity(nullptr);
}

//----------------------------------------------------------------------------
void svtkProgrammableElectronicData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfElectrons: " << this->NumberOfElectrons << "\n";

  os << indent << "MOs: (std::vector<svtkImageData*>) @" << this->MOs << "\n";
  os << indent.GetNextIndent() << "size: " << this->MOs->size() << "\n";
  for (size_t i = 0; i < this->MOs->size(); ++i)
  {
    svtkImageData* current = this->MOs->at(i);
    os << indent.GetNextIndent() << "MO #" << i + 1 << " @" << current << "\n";
    if (current)
      current->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  }

  os << indent << "ElectronDensity: @" << this->ElectronDensity << "\n";
  if (this->ElectronDensity)
    this->ElectronDensity->PrintSelf(os, indent.GetNextIndent().GetNextIndent());

  os << indent << "Padding: " << this->Padding << "\n";
}

//----------------------------------------------------------------------------
svtkIdType svtkProgrammableElectronicData::GetNumberOfMOs()
{
  return static_cast<svtkIdType>(this->MOs->size());
}

//----------------------------------------------------------------------------
void svtkProgrammableElectronicData::SetNumberOfMOs(svtkIdType size)
{
  if (size == static_cast<svtkIdType>(this->MOs->size()))
  {
    return;
  }

  svtkDebugMacro(<< "Resizing MO vector from " << this->MOs->size() << " to " << size << ".");
  this->MOs->resize(size);

  this->Modified();
}

//----------------------------------------------------------------------------
svtkImageData* svtkProgrammableElectronicData::GetMO(svtkIdType orbitalNumber)
{
  if (orbitalNumber <= 0)
  {
    svtkWarningMacro(<< "Request for invalid orbital number " << orbitalNumber);
    return nullptr;
  }
  if (orbitalNumber > static_cast<svtkIdType>(this->MOs->size()))
  {
    svtkWarningMacro(<< "Request for orbital number " << orbitalNumber
                    << ", which exceeds the number of MOs (" << this->MOs->size() << ")");
    return nullptr;
  }

  svtkImageData* result = this->MOs->at(orbitalNumber - 1);

  svtkDebugMacro(<< "Returning '" << result << "' for MO '" << orbitalNumber << "'");
  return result;
}

//----------------------------------------------------------------------------
void svtkProgrammableElectronicData::SetMO(svtkIdType orbitalNumber, svtkImageData* data)
{
  if (orbitalNumber <= 0)
  {
    svtkErrorMacro("Cannot set invalid orbital number " << orbitalNumber);
    return;
  }
  if (orbitalNumber > static_cast<svtkIdType>(this->MOs->size()))
  {
    this->SetNumberOfMOs(orbitalNumber);
  }

  svtkImageData* previous = this->MOs->at(orbitalNumber - 1);
  if (data == previous)
    return;

  svtkDebugMacro(<< "Changing MO " << orbitalNumber << " from @" << previous << " to @" << data
                << ".");

  this->MOs->at(orbitalNumber - 1) = data;

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkProgrammableElectronicData::DeepCopy(svtkDataObject* obj)
{
  svtkProgrammableElectronicData* source = svtkProgrammableElectronicData::SafeDownCast(obj);
  if (!source)
  {
    svtkErrorMacro("Can only deep copy from svtkProgrammableElectronicData "
                  "or subclass.");
    return;
  }

  // Call superclass
  this->Superclass::DeepCopy(source);

  this->NumberOfElectrons = source->NumberOfElectrons;

  // Grow vector if needed
  this->SetNumberOfMOs(source->GetNumberOfMOs());

  for (size_t i = 0; i < source->MOs->size(); ++i)
  {
    svtkImageData* current = source->MOs->at(i);
    if (current)
    {
      svtkNew<svtkImageData> newImage;
      newImage->DeepCopy(current);
      this->SetMO(static_cast<svtkIdType>(i), newImage);
    }
  }

  if (source->ElectronDensity)
  {
    svtkNew<svtkImageData> newImage;
    newImage->DeepCopy(source->ElectronDensity);
    this->SetElectronDensity(newImage);
  }
}
