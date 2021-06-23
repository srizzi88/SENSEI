/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPStreaklineFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPStreaklineFilter.h"
#include "svtkAppendPolyData.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSetGet.h"
#include "svtkSmartPointer.h"

#include <cassert>
#include <vector>

svtkStandardNewMacro(svtkPStreaklineFilter);

svtkPStreaklineFilter::svtkPStreaklineFilter()
{
  this->It.Initialize(this);
}

int svtkPStreaklineFilter::OutputParticles(svtkPolyData* particles)
{
  return this->It.OutputParticles(particles);
}

void svtkPStreaklineFilter::Finalize()
{
  int leader = 0;
  int tag = 129;

  if (this->Controller->GetLocalProcessId() == leader) // process 0 do the actual work
  {
    svtkNew<svtkAppendPolyData> append;
    int totalNumPts(0);
    for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
    {
      if (i != this->Controller->GetLocalProcessId())
      {
        svtkSmartPointer<svtkPolyData> output_i = svtkSmartPointer<svtkPolyData>::New();
        this->Controller->Receive(output_i, i, tag);
        append->AddInputData(output_i);
        totalNumPts += output_i->GetNumberOfPoints();
      }
      else
      {
        append->AddInputData(this->Output);
        totalNumPts += this->Output->GetNumberOfPoints();
      }
    }
    append->Update();
    svtkPolyData* appoutput = append->GetOutput();
    this->Output->Initialize();
    this->Output->ShallowCopy(appoutput);
    assert(this->Output->GetNumberOfPoints() == totalNumPts);
    this->It.Finalize();
  }
  else
  {
    // send everything to rank 0
    this->Controller->Send(this->Output, leader, tag);
    this->Output->Initialize();
  }

  return;
}

void svtkPStreaklineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
