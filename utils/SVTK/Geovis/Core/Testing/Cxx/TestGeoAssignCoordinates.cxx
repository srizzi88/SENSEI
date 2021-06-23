/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoAssignCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGeoAssignCoordinates.h"
#include "svtkGraphMapper.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestGeoAssignCoordinates(int argc, char* argv[])
{
  SVTK_CREATE(svtkMutableDirectedGraph, g);
  SVTK_CREATE(svtkDoubleArray, latitude);
  latitude->SetName("latitude");
  SVTK_CREATE(svtkDoubleArray, longitude);
  longitude->SetName("longitude");
  for (svtkIdType i = -90; i <= 90; i += 10)
  {
    for (svtkIdType j = -180; j < 180; j += 20)
    {
      g->AddVertex();
      latitude->InsertNextValue(i);
      longitude->InsertNextValue(j);
    }
  }
  g->GetVertexData()->AddArray(latitude);
  g->GetVertexData()->AddArray(longitude);

  SVTK_CREATE(svtkGeoAssignCoordinates, assign);
  assign->SetInputData(g);
  assign->SetLatitudeArrayName("latitude");
  assign->SetLongitudeArrayName("longitude");
  assign->SetGlobeRadius(1.0);
  assign->Update();

  SVTK_CREATE(svtkGraphMapper, mapper);
  mapper->SetInputConnection(assign->GetOutputPort());
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);
  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(actor);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  ren->ResetCamera();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
