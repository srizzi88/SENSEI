/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTessellatedBoxSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkTessellatedBoxSource.h"
#include "svtkXMLPolyDataWriter.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCellDataToPointData.h"
#include "svtkClipConvexPolyData.h"
#include "svtkColorTransferFunction.h"
#include "svtkContourFilter.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkLookupTable.h"
#include "svtkOutlineFilter.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPlane.h"
#include "svtkPlaneCollection.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStructuredPoints.h"
#include "svtkTestUtilities.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkUniformGrid.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLHierarchicalBoxDataReader.h"

int TestTessellatedBoxSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkTessellatedBoxSource* boxSource = svtkTessellatedBoxSource::New();
  boxSource->SetBounds(0, 1, 0, 1, 0, 1);
  boxSource->QuadsOn();
  boxSource->SetLevel(4);
  boxSource->Update();
  svtkXMLPolyDataWriter* writer = svtkXMLPolyDataWriter::New();
  writer->SetInputConnection(boxSource->GetOutputPort());
  boxSource->Delete();
  writer->SetFileName("box.vtp");
  writer->SetDataModeToAscii();
  writer->Update();

  svtkClipConvexPolyData* clip = svtkClipConvexPolyData::New();
  clip->SetInputConnection(boxSource->GetOutputPort());

  svtkPlaneCollection* planes = svtkPlaneCollection::New();
  clip->SetPlanes(planes);
  planes->Delete();

  svtkPlane* p = svtkPlane::New();
  planes->AddItem(p);
  p->Delete();

  double origin[3] = { 0.5, 0.5, 0.5 };
  double direction[3] = { 0, 0, 1 };

  p->SetOrigin(origin);
  p->SetNormal(direction);
  planes->AddItem(p);

  svtkXMLPolyDataWriter* writer2 = svtkXMLPolyDataWriter::New();
  writer2->SetInputConnection(clip->GetOutputPort());
  clip->Delete();
  writer2->SetFileName("clipbox.vtp");
  writer2->SetDataModeToAscii();
  writer2->Update();
  writer2->Delete();

  writer->Delete();

  return 0; // 0==success.
}
