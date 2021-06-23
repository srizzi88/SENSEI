/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGPUInfoList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGPUInfoList.h"

#include "svtkGPUInfo.h"
#include "svtkGraphicsFactory.h"
#include <cassert>

#include "svtkGPUInfoListArray.h"
#include <vector>

// ----------------------------------------------------------------------------
svtkGPUInfoList* svtkGPUInfoList::New()
{
  svtkObject* ret = svtkGraphicsFactory::CreateInstance("svtkGPUInfoList");
  return static_cast<svtkGPUInfoList*>(ret);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if the operating system has been probed. Initial value is false.
bool svtkGPUInfoList::IsProbed()
{
  return this->Probed;
}

// ----------------------------------------------------------------------------
// Description:
// Return the number of GPUs.
// \pre probed: IsProbed()
int svtkGPUInfoList::GetNumberOfGPUs()
{
  if (!this->IsProbed())
  {
    svtkErrorMacro("You must first call the Probe method");
    return 0;
  }

  return static_cast<int>(this->Array->v.size());
}

// ----------------------------------------------------------------------------
// Description:
// Return information about GPU i.
// \pre probed: IsProbed()
// \pre valid_index: i>=0 && i<GetNumberOfGPUs()
// \post result_exists: result!=0
svtkGPUInfo* svtkGPUInfoList::GetGPUInfo(int i)
{
  assert("pre: probed" && this->IsProbed());
  assert("pre: valid_index" && i >= 0 && i < this->GetNumberOfGPUs());

  svtkGPUInfo* result = this->Array->v[static_cast<size_t>(i)];
  assert("post: result_exists" && result != nullptr);
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Default constructor. Set Probed to false. Set Array to nullptr.
svtkGPUInfoList::svtkGPUInfoList()
{
  this->Probed = false;
  this->Array = nullptr;
}

// ----------------------------------------------------------------------------
svtkGPUInfoList::~svtkGPUInfoList()
{
  if (this->Array != nullptr)
  {
    size_t c = this->Array->v.size();
    size_t i = 0;
    while (i < c)
    {
      this->Array->v[i]->Delete();
      ++i;
    }
    delete this->Array;
  }
}

// ----------------------------------------------------------------------------
void svtkGPUInfoList::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "IsProbed: " << this->Probed << endl;
  if (this->Probed)
  {
    int c = this->GetNumberOfGPUs();
    os << indent << "Number of GPUs: " << c << endl;
    int i = 0;
    while (i < c)
    {
      os << indent << " GPU " << i;
      this->GetGPUInfo(i)->PrintSelf(os, indent);
      ++i;
    }
  }
}
