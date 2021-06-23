/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGeodesicPath.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"

//-----------------------------------------------------------------------------
svtkGeodesicPath::svtkGeodesicPath()
{
  this->SetNumberOfInputPorts(1);
}

//-----------------------------------------------------------------------------
svtkGeodesicPath::~svtkGeodesicPath() = default;

//-----------------------------------------------------------------------------
int svtkGeodesicPath::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void svtkGeodesicPath::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
