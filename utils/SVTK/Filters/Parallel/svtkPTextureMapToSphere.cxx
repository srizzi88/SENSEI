/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPTextureMapToSphere.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPTextureMapToSphere.h"

#include "svtkCommunicator.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkTextureMapToSphere.h"

svtkStandardNewMacro(svtkPTextureMapToSphere);

// Create object with Center (0,0,0) and the PreventSeam ivar is set to true. The
// sphere center is automatically computed.
svtkPTextureMapToSphere::svtkPTextureMapToSphere()
{
  this->Controller = svtkMultiProcessController::GetGlobalController();
}

void svtkPTextureMapToSphere::ComputeCenter(svtkDataSet* dataSet)
{
  if (this->AutomaticSphereGeneration && this->Controller->GetNumberOfProcesses() > 1)
  {
    svtkIdType numberOfPoints = dataSet->GetNumberOfPoints();
    double out[4] = { static_cast<double>(numberOfPoints), 0.0, 0.0, 0.0 }, x[3], in[4];

    for (svtkIdType id = 0; id < numberOfPoints; ++id)
    {
      dataSet->GetPoint(id, x);
      out[1] += x[0];
      out[2] += x[1];
      out[3] += x[2];
    }

    this->Controller->AllReduce(out, in, 4, svtkCommunicator::SUM_OP);

    if (!in[0])
    {
      svtkErrorMacro(<< "No points");
    }

    this->Center[0] = in[1] / in[0];
    this->Center[1] = in[2] / in[0];
    this->Center[2] = in[3] / in[0];
  }
  else
  {
    this->Superclass::ComputeCenter(dataSet);
  }
}

void svtkPTextureMapToSphere::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Controller)
  {
    os << indent << "Controller:\n";
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Controller: (none)" << endl;
  }
}
