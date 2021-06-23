/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageConnector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageConnector.h"

#include "svtkImageData.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkImageConnector);

//----------------------------------------------------------------------------
svtkImageConnector::svtkImageConnector()
{
  this->Seeds = nullptr;
  this->LastSeed = nullptr;
  this->ConnectedValue = 255;
  this->UnconnectedValue = 128;
}

//----------------------------------------------------------------------------
svtkImageConnector::~svtkImageConnector()
{
  this->RemoveAllSeeds();
}

//----------------------------------------------------------------------------
void svtkImageConnector::RemoveAllSeeds()
{
  svtkImageConnectorSeed* temp;

  while (this->Seeds)
  {
    temp = this->Seeds;
    this->Seeds = temp->Next;
    delete temp;
  }
  this->LastSeed = nullptr;
}

//----------------------------------------------------------------------------
svtkImageConnectorSeed* svtkImageConnector::NewSeed(int index[3], void* ptr)
{
  svtkImageConnectorSeed* seed = svtkImageConnectorSeed::New();
  int idx;

  for (idx = 0; idx < 3; ++idx)
  {
    seed->Index[idx] = index[idx];
  }
  seed->Pointer = ptr;
  seed->Next = nullptr;

  return seed;
}

//----------------------------------------------------------------------------
// Add a new seed to the end of the seed list.
void svtkImageConnector::AddSeedToEnd(svtkImageConnectorSeed* seed)
{
  // Add the seed to the end of the list
  if (this->LastSeed == nullptr)
  { // no seeds yet
    this->LastSeed = this->Seeds = seed;
  }
  else
  {
    this->LastSeed->Next = seed;
    this->LastSeed = seed;
  }
}

//----------------------------------------------------------------------------
// Add a new seed to the start of the seed list.
void svtkImageConnector::AddSeed(svtkImageConnectorSeed* seed)
{
  seed->Next = this->Seeds;
  this->Seeds = seed;
  if (!this->LastSeed)
  {
    this->LastSeed = seed;
  }
}

//----------------------------------------------------------------------------
// Removes a seed from the start of the seed list, and returns the seed.
svtkImageConnectorSeed* svtkImageConnector::PopSeed()
{
  svtkImageConnectorSeed* seed;

  seed = this->Seeds;
  this->Seeds = seed->Next;
  if (this->Seeds == nullptr)
  {
    this->LastSeed = nullptr;
  }
  return seed;
}

//----------------------------------------------------------------------------
// Input a data of 0's and "UnconnectedValue"s. Seeds of this object are
// used to find connected pixels.
// All pixels connected to seeds are set to ConnectedValue.
// The data has to be unsigned char.
void svtkImageConnector::MarkData(svtkImageData* data, int numberOfAxes, int extent[6])
{
  svtkIdType incs[3], *pIncs;
  int* pExtent;
  svtkImageConnectorSeed* seed;
  unsigned char* ptr;
  int newIndex[3], *pIndex, idx;
  long count = 0;

  data->GetIncrements(incs);
  while (this->Seeds)
  {
    ++count;
    seed = this->PopSeed();
    // just in case the seed has not been marked visited.
    *(static_cast<unsigned char*>(seed->Pointer)) = this->ConnectedValue;
    // Add neighbors
    newIndex[0] = seed->Index[0];
    newIndex[1] = seed->Index[1];
    newIndex[2] = seed->Index[2];
    pExtent = extent;
    pIncs = incs;
    pIndex = newIndex;
    for (idx = 0; idx < numberOfAxes; ++idx)
    {
      // check pixel below
      if (*pExtent < *pIndex)
      {
        ptr = static_cast<unsigned char*>(seed->Pointer) - *pIncs;
        if (*ptr == this->UnconnectedValue)
        { // add a new seed
          --(*pIndex);
          *ptr = this->ConnectedValue;
          this->AddSeedToEnd(this->NewSeed(newIndex, ptr));
          ++(*pIndex);
        }
      }
      ++pExtent;
      // check above pixel
      if (*pExtent > *pIndex)
      {
        ptr = static_cast<unsigned char*>(seed->Pointer) + *pIncs;
        if (*ptr == this->UnconnectedValue)
        { // add a new seed
          ++(*pIndex);
          *ptr = this->ConnectedValue;
          this->AddSeedToEnd(this->NewSeed(newIndex, ptr));
          --(*pIndex);
        }
      }
      ++pExtent;
      // move to next axis
      ++pIncs;
      ++pIndex;
    }

    // Delete seed
    delete seed;
  }
  svtkDebugMacro("Marked " << count << " pixels");
}

void svtkImageConnector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ConnectedValue: " << this->ConnectedValue << "\n";
  os << indent << "UnconnectedValue: " << this->UnconnectedValue << "\n";
}
