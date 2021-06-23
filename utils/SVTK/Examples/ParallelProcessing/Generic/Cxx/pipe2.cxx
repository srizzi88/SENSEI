/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pipe2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkContourFilter.h"
#include "svtkImageData.h"
#include "svtkInputPort.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

#include "PipelineParallelism.h"

// Pipe 2 for PipelineParallelism.
// See PipelineParallelism.cxx for more information.
void pipe2(svtkMultiProcessController* svtkNotUsed(controller), void* svtkNotUsed(arg))
{
  // Input port
  svtkInputPort* ip = svtkInputPort::New();
  ip->SetRemoteProcessId(0);
  ip->SetTag(11);

  // Iso-surface
  svtkContourFilter* cf = svtkContourFilter::New();
  cf->SetInput(ip->GetImageDataOutput());
  cf->SetNumberOfContours(1);
  cf->SetValue(0, 220);

  // Rendering objects
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(cf->GetOutputPort());

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  svtkRenderer* ren = svtkRenderer::New();
  ren->AddActor(actor);

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren);

  // Normally, the Render() call on a RenderWindow() calls
  // update on it's actors two times. This is not appropriate
  // for this example because with each Update(), the data
  // changes. For this reason, we will use a separate PolyData
  // object and (shallow) copy the data to it.
  svtkPolyData* pd = svtkPolyData::New();
  mapper->SetInput(pd);

  // Prime the pipeline. Tell the producer to start computing.
  ip->Update();

  // Get the first data, adjust camera appropriately
  cf->GetOutput()->Update();
  pd->ShallowCopy(cf->GetOutput());
  ren->ResetCamera();
  // Display data
  renWin->Render();

  // Get more data. With every update the XFreq of the rtSource
  // is increased.
  for (int i = 0; i < 17; i++)
  {
    cf->GetOutput()->Update();
    pd->ShallowCopy(cf->GetOutput());
    // Display
    renWin->Render();
  }
  // Tell the producer that we are done.
  ip->GetController()->TriggerRMI(0, svtkMultiProcessController::BREAK_RMI_TAG);

  pd->Delete();
  ip->Delete();
  cf->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
}
