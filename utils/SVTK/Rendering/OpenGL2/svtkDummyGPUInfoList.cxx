/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDummyGPUInfoList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDummyGPUInfoList.h"

#include "svtkGPUInfoListArray.h"

#include "svtkObjectFactory.h"
#include <cassert>

svtkStandardNewMacro(svtkDummyGPUInfoList);

// ----------------------------------------------------------------------------
// Description:
// Build the list of svtkInfoGPU if not done yet.
// \post probed: IsProbed()
void svtkDummyGPUInfoList::Probe()
{
  if (!this->Probed)
  {
    this->Probed = true;
    this->Array = new svtkGPUInfoListArray;
    this->Array->v.resize(0); // no GPU.
  }
  assert("post: probed" && this->IsProbed());
}

// ----------------------------------------------------------------------------
svtkDummyGPUInfoList::svtkDummyGPUInfoList() = default;

// ----------------------------------------------------------------------------
svtkDummyGPUInfoList::~svtkDummyGPUInfoList() = default;

// ----------------------------------------------------------------------------
void svtkDummyGPUInfoList::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
