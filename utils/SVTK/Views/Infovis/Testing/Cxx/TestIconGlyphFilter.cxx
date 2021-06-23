
/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include <svtkIconGlyphFilter.h>

#include <svtkApplyIcons.h>
#include <svtkDoubleArray.h>
#include <svtkFollower.h>
#include <svtkGraphLayoutView.h>
#include <svtkGraphMapper.h>
#include <svtkImageData.h>
#include <svtkMutableUndirectedGraph.h>
#include <svtkPNGReader.h>
#include <svtkPointData.h>
#include <svtkPointSet.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderedGraphRepresentation.h>
#include <svtkRenderer.h>
#include <svtkTexture.h>

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

int TestIconGlyphFilter(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Tango/TangoIcons.png");

  int imageDims[3];

  svtkPNGReader* imageReader = svtkPNGReader::New();

  imageReader->SetFileName(fname);
  imageReader->Update();

  delete[] fname;

  imageReader->GetOutput()->GetDimensions(imageDims);

  svtkMutableUndirectedGraph* graph = svtkMutableUndirectedGraph::New();
  svtkPoints* points = svtkPoints::New();
  svtkDoubleArray* pointData = svtkDoubleArray::New();
  pointData->SetNumberOfComponents(3);
  points->SetData(static_cast<svtkDataArray*>(pointData));
  graph->SetPoints(points);

  svtkIntArray* iconIndex = svtkIntArray::New();
  iconIndex->SetName("IconIndex");
  iconIndex->SetNumberOfComponents(1);

  graph->GetVertexData()->SetScalars(iconIndex);

  graph->AddVertex();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(3.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, 2.5, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(0.0, -2.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, -1.5, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(-1.0, 2.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(3.0, 0.0, 0.0);

  graph->AddEdge(0, 1);
  graph->AddEdge(1, 2);
  graph->AddEdge(2, 3);
  graph->AddEdge(3, 4);
  graph->AddEdge(4, 5);
  graph->AddEdge(5, 6);
  graph->AddEdge(6, 7);
  graph->AddEdge(7, 0);

  iconIndex->InsertNextTuple1(1);
  iconIndex->InsertNextTuple1(4);
  iconIndex->InsertNextTuple1(26);
  iconIndex->InsertNextTuple1(17);
  iconIndex->InsertNextTuple1(0);
  iconIndex->InsertNextTuple1(5);
  iconIndex->InsertNextTuple1(1);
  iconIndex->InsertNextTuple1(29);

  svtkGraphLayoutView* view = svtkGraphLayoutView::New();
  view->DisplayHoverTextOff();
  view->SetRepresentationFromInput(graph);
  view->SetLayoutStrategyToSimple2D();
  view->ResetCamera();

  svtkTexture* texture = svtkTexture::New();
  texture->SetInputConnection(imageReader->GetOutputPort());
  view->SetIconTexture(texture);
  int size[] = { 24, 24 };
  view->SetIconSize(size);
  svtkRenderedGraphRepresentation* rep =
    static_cast<svtkRenderedGraphRepresentation*>(view->GetRepresentation());
  rep->UseVertexIconTypeMapOff();
  rep->SetVertexSelectedIcon(12);
  rep->SetVertexIconSelectionModeToSelectedIcon();
  rep->VertexIconVisibilityOn();
  rep->SetVertexIconArrayName(iconIndex->GetName());
  rep->SetLayoutStrategyToPassThrough();

  view->GetRenderWindow()->SetSize(500, 500);

  view->GetInteractor()->Initialize();
  view->Render();
  int retVal = svtkRegressionTestImageThreshold(view->GetRenderWindow(), 18);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Start();
  }

  imageReader->Delete();
  iconIndex->Delete();
  graph->Delete();
  points->Delete();
  pointData->Delete();
  view->Delete();
  texture->Delete();

  return !retVal;
}
