/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParallelBFS.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtk_mpi.h>

#include "svtkEdgeListIterator.h"
#include "svtkGraphLayoutView.h"
#include "svtkInEdgeIterator.h"
#include "svtkInformation.h"
#include "svtkMPIController.h"
#include "svtkPBGLBreadthFirstSearch.h"
#include "svtkPBGLCollectGraph.h"
#include "svtkPBGLDistributedGraphHelper.h"
#include "svtkPBGLRandomGraphSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUndirectedGraph.h"
#include "svtkViewTheme.h"

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  svtkSmartPointer<svtkPBGLRandomGraphSource> source =
    svtkSmartPointer<svtkPBGLRandomGraphSource>::New();
  source->DirectedOff();
  source->SetNumberOfVertices(100000);
  source->SetNumberOfEdges(10000);
  source->StartWithTreeOn();
  svtkSmartPointer<svtkPBGLBreadthFirstSearch> bfs =
    svtkSmartPointer<svtkPBGLBreadthFirstSearch>::New();
  bfs->SetInputConnection(source->GetOutputPort());
  svtkSmartPointer<svtkPBGLCollectGraph> collect = svtkSmartPointer<svtkPBGLCollectGraph>::New();
  collect->SetInputConnection(bfs->GetOutputPort());

  // Setup pipeline request
  svtkSmartPointer<svtkMPIController> controller = svtkSmartPointer<svtkMPIController>::New();
  controller->Initialize(&argc, &argv, 1);
  int rank = controller->GetLocalProcessId();
  int procs = controller->GetNumberOfProcesses();
  collect->Update(rank, procs, 0);

  if (rank == 0)
  {
    svtkSmartPointer<svtkUndirectedGraph> g = svtkSmartPointer<svtkUndirectedGraph>::New();
    g->ShallowCopy(collect->GetOutput());
    svtkSmartPointer<svtkGraphLayoutView> view = svtkSmartPointer<svtkGraphLayoutView>::New();
    svtkSmartPointer<svtkViewTheme> theme;
    theme.TakeReference(svtkViewTheme::CreateMellowTheme());
    view->ApplyViewTheme(theme);
    view->SetRepresentationFromInput(g);
    view->SetVertexColorArrayName("BFS");
    view->ColorVerticesOn();
    svtkRenderWindow* win = view->GetRenderWindow();
    view->Update();
    view->GetRenderer()->ResetCamera();
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();
  }

  controller->Finalize();
  return 0;
}
