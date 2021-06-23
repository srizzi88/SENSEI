/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGPUInfoListArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGPUInfoListArray
 * @brief   Internal class svtkGPUInfoList.
 *
 * svtkGPUInfoListArray is just a PIMPL mechanism for svtkGPUInfoList.
 */

#ifndef svtkGPUInfoListArray_h
#define svtkGPUInfoListArray_h

#include "svtkGPUInfo.h"
#include <vector> // STL Header

class svtkGPUInfoListArray
{
public:
  std::vector<svtkGPUInfo*> v;
};

#endif
// SVTK-HeaderTest-Exclude: svtkGPUInfoListArray.h
