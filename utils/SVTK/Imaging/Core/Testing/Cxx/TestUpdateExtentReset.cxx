/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageWeightedSum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkColorTransferFunction.h>
#include <svtkGlyph3D.h>
#include <svtkImageAppendComponents.h>
#include <svtkImageData.h>
#include <svtkImageMapToColors.h>
#include <svtkImageReslice.h>
#include <svtkInformation.h>
#include <svtkNew.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkStreamingDemandDrivenPipeline.h>

int TestUpdateExtentReset(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkImageData> img = svtkSmartPointer<svtkImageData>::New();
  img->SetDimensions(100, 100, 100);
  img->AllocateScalars(SVTK_FLOAT, 1);

  float* scalars = static_cast<float*>(img->GetScalarPointer());
  svtkIdType n = 100 * 100 * 100;
  for (svtkIdType i = 0; i < n; i++)
  {
    scalars[i] = 0.0;
  }

  svtkSmartPointer<svtkImageReslice> reslicer = svtkSmartPointer<svtkImageReslice>::New();
  reslicer->SetInputData(img);
  reslicer->SetOutputExtent(0, 100, 0, 100, 0, 0);

  svtkSmartPointer<svtkImageMapToColors> colors = svtkSmartPointer<svtkImageMapToColors>::New();
  colors->SetInputConnection(reslicer->GetOutputPort());

  svtkSmartPointer<svtkColorTransferFunction> ctf = svtkSmartPointer<svtkColorTransferFunction>::New();
  ctf->AddRGBPoint(0, 1., 0., 0.);
  colors->SetLookupTable(ctf);

  svtkSmartPointer<svtkImageAppendComponents> append =
    svtkSmartPointer<svtkImageAppendComponents>::New();
  append->SetInputConnection(0, colors->GetOutputPort());

  colors->Update();
  append->Update();
  colors->Update();
  // At this point, the COMBINED_UPDATE_EXTENT of the output of reslicer must be
  // reset to {0, -1, 0, -1, 0, -1}, otherwise, the following will fail because
  // when computing the output extent it will take into account the old (not reset)
  // COMBINED_UPDATE_EXTENT value.
  reslicer->SetOutputExtent(0, 100, 0, 80, 0, 0);
  colors->Update();

  svtkNew<svtkGlyph3D> polyDataFilter;
  polyDataFilter->SetInputConnection(0, colors->GetOutputPort());
  svtkNew<svtkSphereSource> sphere;
  polyDataFilter->SetSourceConnection(sphere->GetOutputPort());
  polyDataFilter->Update();
  // After Update(), the COMBINED_UPDATE_EXTENT of the output of reslicer must be
  // reset to {0, -1, 0, -1, 0, -1}, otherwise, the following will fail because
  // when computing the output extent it will take into account the old (not reset)
  // COMBINED_UPDATE_EXTENT value.
  int combinedExtent[6];
  reslicer->GetExecutive()->GetOutputInformation(0)->Get(
    svtkStreamingDemandDrivenPipeline::COMBINED_UPDATE_EXTENT(), combinedExtent);
  if (combinedExtent[0] <= combinedExtent[1] || combinedExtent[2] <= combinedExtent[3] ||
    combinedExtent[4] <= combinedExtent[5])
  {
    return EXIT_FAILURE;
  }
  reslicer->SetOutputExtent(0, 100, 0, 50, 0, 0);
  // For some reason there was no error reported when the combined extent was
  // was wrong, however you could check in svtkImageReslice::ThreadRequestData
  // and you'll see that the output extent is still {0, 100, 0, 80, 0, 0}
  append->Update();

  return EXIT_SUCCESS;
}
