
/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "svtkSmartPointer.h"
#include <svtkIconGlyphFilter.h>

#include <svtkAppendPolyData.h>
#include <svtkDoubleArray.h>
#include <svtkImageData.h>
#include <svtkPNGReader.h>
#include <svtkPointData.h>
#include <svtkPointSet.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper2D.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTexture.h>
#include <svtkTexturedActor2D.h>

#include <svtkTestUtilities.h>

int TestIconGlyphFilterGravity(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Tango/TangoIcons.png");

  int imageDims[3];

  svtkSmartPointer<svtkPNGReader> imageReader = svtkSmartPointer<svtkPNGReader>::New();

  imageReader->SetFileName(fname);
  delete[] fname;
  imageReader->Update();

  imageReader->GetOutput()->GetDimensions(imageDims);

  svtkSmartPointer<svtkPolyData> pointSet = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData = svtkSmartPointer<svtkDoubleArray>::New();
  pointData->SetNumberOfComponents(3);
  points->SetData(pointData);
  pointSet->SetPoints(points);

  svtkSmartPointer<svtkIntArray> iconIndex = svtkSmartPointer<svtkIntArray>::New();
  iconIndex->SetNumberOfComponents(1);

  pointSet->GetPointData()->SetScalars(iconIndex);

  for (double i = 1.0; i < 8; i++)
  {
    points->InsertNextPoint(i * 26.0, 26.0, 0.0);
  }

  for (int i = 0; i < points->GetNumberOfPoints(); i++)
  {
    iconIndex->InsertNextTuple1(i);
  }

  int size[] = { 24, 24 };

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter->SetInputData(pointSet);
  iconFilter->SetIconSize(size);
  iconFilter->SetUseIconSize(true);
  iconFilter->SetIconSheetSize(imageDims);
  iconFilter->SetGravityToBottomLeft();

  svtkSmartPointer<svtkPolyData> pointSet2 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points2 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData2 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData2->SetNumberOfComponents(3);
  points2->SetData(pointData2);
  pointSet2->SetPoints(points2);

  svtkSmartPointer<svtkIntArray> iconIndex2 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex2->SetNumberOfComponents(1);

  pointSet2->GetPointData()->SetScalars(iconIndex2);

  for (double i = 1.0; i < 8; i++)
  {
    points2->InsertNextPoint(i * 26.0, 52.0, 0.0);
  }

  for (int i = 0; i < points2->GetNumberOfPoints(); i++)
  {
    iconIndex2->InsertNextTuple1(i + 8);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter2 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter2->SetInputData(pointSet2);
  iconFilter2->SetIconSize(size);
  iconFilter2->SetUseIconSize(true);
  iconFilter2->SetIconSheetSize(imageDims);
  iconFilter2->SetGravityToBottomCenter();

  svtkSmartPointer<svtkPolyData> pointSet3 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points3 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData3 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData3->SetNumberOfComponents(3);
  points3->SetData(pointData3);
  pointSet3->SetPoints(points3);

  svtkSmartPointer<svtkIntArray> iconIndex3 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex3->SetNumberOfComponents(1);

  pointSet3->GetPointData()->SetScalars(iconIndex3);

  for (double i = 1.0; i < 8; i++)
  {
    points3->InsertNextPoint(i * 26.0, 78.0, 0.0);
  }

  for (int i = 0; i < points3->GetNumberOfPoints(); i++)
  {
    iconIndex3->InsertNextTuple1(i + 16);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter3 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter3->SetInputData(pointSet3);
  iconFilter3->SetIconSize(size);
  iconFilter3->SetUseIconSize(true);
  iconFilter3->SetIconSheetSize(imageDims);
  iconFilter3->SetGravityToBottomRight();

  svtkSmartPointer<svtkPolyData> pointSet4 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points4 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData4 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData4->SetNumberOfComponents(3);
  points4->SetData(pointData4);
  pointSet4->SetPoints(points4);

  svtkSmartPointer<svtkIntArray> iconIndex4 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex4->SetNumberOfComponents(1);

  pointSet4->GetPointData()->SetScalars(iconIndex4);

  for (double i = 1.0; i < 8; i++)
  {
    points4->InsertNextPoint(i * 26.0, 104.0, 0.0);
  }

  for (int i = 0; i < points4->GetNumberOfPoints(); i++)
  {
    iconIndex4->InsertNextTuple1(i + 24);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter4 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter4->SetInputData(pointSet4);
  iconFilter4->SetIconSize(size);
  iconFilter4->SetUseIconSize(true);
  iconFilter4->SetIconSheetSize(imageDims);
  iconFilter4->SetGravityToCenterLeft();

  svtkSmartPointer<svtkPolyData> pointSet5 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points5 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData5 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData5->SetNumberOfComponents(3);
  points5->SetData(pointData5);
  pointSet5->SetPoints(points5);

  svtkSmartPointer<svtkIntArray> iconIndex5 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex5->SetNumberOfComponents(1);

  pointSet5->GetPointData()->SetScalars(iconIndex5);

  for (double i = 1.0; i < 8; i++)
  {
    points5->InsertNextPoint(i * 26.0, 130.0, 0.0);
  }

  for (int i = 0; i < points5->GetNumberOfPoints(); i++)
  {
    iconIndex5->InsertNextTuple1(i + 32);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter5 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter5->SetInputData(pointSet5);
  iconFilter5->SetIconSize(size);
  iconFilter5->SetUseIconSize(true);
  iconFilter5->SetIconSheetSize(imageDims);
  iconFilter5->SetGravityToCenterCenter();

  svtkSmartPointer<svtkPolyData> pointSet6 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points6 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData6 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData6->SetNumberOfComponents(3);
  points6->SetData(pointData6);
  pointSet6->SetPoints(points6);

  svtkSmartPointer<svtkIntArray> iconIndex6 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex6->SetNumberOfComponents(1);

  pointSet6->GetPointData()->SetScalars(iconIndex6);

  for (double i = 1.0; i < 8; i++)
  {
    points6->InsertNextPoint(i * 26.0, 156.0, 0.0);
  }

  for (int i = 0; i < points6->GetNumberOfPoints(); i++)
  {
    iconIndex6->InsertNextTuple1(i + 40);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter6 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter6->SetInputData(pointSet6);
  iconFilter6->SetIconSize(size);
  iconFilter6->SetUseIconSize(true);
  iconFilter6->SetIconSheetSize(imageDims);
  iconFilter6->SetGravityToCenterRight();

  svtkSmartPointer<svtkPolyData> pointSet7 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points7 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData7 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData7->SetNumberOfComponents(3);
  points7->SetData(pointData7);
  pointSet7->SetPoints(points7);

  svtkSmartPointer<svtkIntArray> iconIndex7 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex7->SetNumberOfComponents(1);

  pointSet7->GetPointData()->SetScalars(iconIndex7);

  for (double i = 1.0; i < 8; i++)
  {
    points7->InsertNextPoint(i * 26.0, 182.0, 0.0);
  }

  for (int i = 0; i < points7->GetNumberOfPoints(); i++)
  {
    iconIndex7->InsertNextTuple1(i + 48);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter7 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter7->SetInputData(pointSet7);
  iconFilter7->SetIconSize(size);
  iconFilter7->SetUseIconSize(true);
  iconFilter7->SetIconSheetSize(imageDims);
  iconFilter7->SetGravityToTopLeft();

  svtkSmartPointer<svtkPolyData> pointSet8 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points8 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData8 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData8->SetNumberOfComponents(3);
  points8->SetData(pointData8);
  pointSet8->SetPoints(points8);

  svtkSmartPointer<svtkIntArray> iconIndex8 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex8->SetNumberOfComponents(1);

  pointSet8->GetPointData()->SetScalars(iconIndex8);

  for (double i = 1.0; i < 8; i++)
  {
    points8->InsertNextPoint(i * 26.0, 208.0, 0.0);
  }

  for (int i = 0; i < points8->GetNumberOfPoints(); i++)
  {
    iconIndex8->InsertNextTuple1(i + 56);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter8 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter8->SetInputData(pointSet8);
  iconFilter8->SetIconSize(size);
  iconFilter8->SetUseIconSize(true);
  iconFilter8->SetIconSheetSize(imageDims);
  iconFilter8->SetGravityToTopCenter();

  svtkSmartPointer<svtkPolyData> pointSet9 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points9 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkDoubleArray> pointData9 = svtkSmartPointer<svtkDoubleArray>::New();
  pointData9->SetNumberOfComponents(3);
  points9->SetData(pointData9);
  pointSet9->SetPoints(points9);

  svtkSmartPointer<svtkIntArray> iconIndex9 = svtkSmartPointer<svtkIntArray>::New();
  iconIndex9->SetNumberOfComponents(1);

  pointSet9->GetPointData()->SetScalars(iconIndex9);

  for (double i = 1.0; i < 8; i++)
  {
    points9->InsertNextPoint(i * 26.0, 234.0, 0.0);
  }

  for (int i = 0; i < points9->GetNumberOfPoints(); i++)
  {
    iconIndex9->InsertNextTuple1(i + 64);
  }

  svtkSmartPointer<svtkIconGlyphFilter> iconFilter9 = svtkSmartPointer<svtkIconGlyphFilter>::New();

  iconFilter9->SetInputData(pointSet9);
  iconFilter9->SetIconSize(size);
  iconFilter9->SetUseIconSize(true);
  iconFilter9->SetIconSheetSize(imageDims);
  iconFilter9->SetGravityToTopRight();

  svtkSmartPointer<svtkAppendPolyData> append = svtkSmartPointer<svtkAppendPolyData>::New();
  append->AddInputConnection(iconFilter->GetOutputPort());
  append->AddInputConnection(iconFilter2->GetOutputPort());
  append->AddInputConnection(iconFilter3->GetOutputPort());
  append->AddInputConnection(iconFilter4->GetOutputPort());
  append->AddInputConnection(iconFilter5->GetOutputPort());
  append->AddInputConnection(iconFilter6->GetOutputPort());
  append->AddInputConnection(iconFilter7->GetOutputPort());
  append->AddInputConnection(iconFilter8->GetOutputPort());
  append->AddInputConnection(iconFilter9->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper2D> mapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  mapper->SetInputConnection(append->GetOutputPort());

  svtkSmartPointer<svtkTexturedActor2D> iconActor = svtkSmartPointer<svtkTexturedActor2D>::New();
  iconActor->SetMapper(mapper);

  svtkSmartPointer<svtkTexture> texture = svtkSmartPointer<svtkTexture>::New();
  texture->SetInputConnection(imageReader->GetOutputPort());
  iconActor->SetTexture(texture);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetSize(208, 260);
  renWin->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renderer->AddActor(iconActor);
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
