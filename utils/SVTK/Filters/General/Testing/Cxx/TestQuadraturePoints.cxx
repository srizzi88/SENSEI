/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadraturePoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates the capabilities of svtkQuadraturePointInterpolator
// svtkQuadraturePointsGenerator and the class required to support their
// addition.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataObject.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDoubleArray.h"
#include "svtkExtractGeometry.h"
#include "svtkGlyph3D.h"
#include "svtkPNGWriter.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkQuadraturePointInterpolator.h"
#include "svtkQuadraturePointsGenerator.h"
#include "svtkQuadratureSchemeDictionaryGenerator.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTesting.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"
#include "svtkWarpVector.h"
#include "svtkWindowToImageFilter.h"
#include "svtkXMLUnstructuredGridReader.h"
#include "svtkXMLUnstructuredGridWriter.h"

#include "svtkSmartPointer.h"
#include <string>

// Generate a vector to warp by.
int GenerateWarpVector(svtkUnstructuredGrid* usg);
// Generate a scalar to threshold by.
int GenerateThresholdScalar(svtkUnstructuredGrid* usg);

int TestQuadraturePoints(int argc, char* argv[])
{
  svtkSmartPointer<svtkTesting> testHelper = svtkSmartPointer<svtkTesting>::New();
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();
  std::string tempDir = testHelper->GetTempDirectory();
  std::string inputFileName = dataRoot + "/Data/Quadratic/CylinderQuadratic.svtk";
  std::string tempFile = tempDir + "/tmp.vtu";
  std::string tempBaseline = tempDir + "/TestQuadraturePoints.png";

  // Raed, xml or legacy file.
  svtkUnstructuredGrid* input = nullptr;
  svtkSmartPointer<svtkXMLUnstructuredGridReader> xusgr =
    svtkSmartPointer<svtkXMLUnstructuredGridReader>::New();
  xusgr->SetFileName(inputFileName.c_str());

  svtkSmartPointer<svtkUnstructuredGridReader> lusgr =
    svtkSmartPointer<svtkUnstructuredGridReader>::New();
  lusgr->SetFileName(inputFileName.c_str());
  if (xusgr->CanReadFile(inputFileName.c_str()))
  {
    input = xusgr->GetOutput();
    xusgr->Update();
    lusgr = nullptr;
  }
  else if (lusgr->IsFileValid("unstructured_grid"))
  {
    lusgr->SetFileName(inputFileName.c_str());
    input = lusgr->GetOutput();
    lusgr->Update();
    xusgr = nullptr;
  }
  if (input == nullptr)
  {
    std::cerr << "Error: Could not read file " << inputFileName << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Add a couple arrays to be used in the demonstrations.
  int warpIdx = GenerateWarpVector(input);
  std::string warpName = input->GetPointData()->GetArray(warpIdx)->GetName();
  int threshIdx = GenerateThresholdScalar(input);
  std::string threshName = input->GetPointData()->GetArray(threshIdx)->GetName();

  // Add a quadrature scheme dictionary to the data set. This filter is
  // solely for our convenience. Typically we would expect that users
  // provide there own in XML format and use the readers or to generate
  // them on the fly.
  svtkSmartPointer<svtkQuadratureSchemeDictionaryGenerator> dictGen =
    svtkSmartPointer<svtkQuadratureSchemeDictionaryGenerator>::New();
  dictGen->SetInputData(input);

  // Interpolate fields to the quadrature points. This generates new field data
  // arrays, but not a set of points.
  svtkSmartPointer<svtkQuadraturePointInterpolator> fieldInterp =
    svtkSmartPointer<svtkQuadraturePointInterpolator>::New();
  fieldInterp->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  fieldInterp->SetInputConnection(dictGen->GetOutputPort());

  // Write the dataset as XML. This exercises the information writer.
  svtkSmartPointer<svtkXMLUnstructuredGridWriter> xusgw =
    svtkSmartPointer<svtkXMLUnstructuredGridWriter>::New();
  xusgw->SetFileName(tempFile.c_str());
  xusgw->SetInputConnection(fieldInterp->GetOutputPort());
  xusgw->Write();
  xusgw = nullptr;
  fieldInterp = nullptr;

  // Read the data back in form disk. This exercises the information reader.
  xusgr = nullptr;
  xusgr.TakeReference(svtkXMLUnstructuredGridReader::New());
  xusgr->SetFileName(tempFile.c_str());
  xusgr->Update();

  input = xusgr->GetOutput();
  input->Register(nullptr);
  input->GetPointData()->SetActiveVectors(warpName.c_str());
  input->GetPointData()->SetActiveScalars(threshName.c_str());

  xusgr = nullptr;

  // Demonstrate warp by vector.
  svtkSmartPointer<svtkWarpVector> warper = svtkSmartPointer<svtkWarpVector>::New();
  warper->SetInputData(input);
  warper->SetScaleFactor(0.02);
  input->Delete();

  // Demonstrate clip functionality.
  svtkSmartPointer<svtkPlane> plane = svtkSmartPointer<svtkPlane>::New();
  plane->SetOrigin(0.0, 0.0, 0.03);
  plane->SetNormal(0.0, 0.0, -1.0);
  svtkSmartPointer<svtkExtractGeometry> clip = svtkSmartPointer<svtkExtractGeometry>::New();
  clip->SetImplicitFunction(plane);
  clip->SetInputConnection(warper->GetOutputPort());

  // Demonstrate threshold functionality.
  svtkSmartPointer<svtkThreshold> thresholder = svtkSmartPointer<svtkThreshold>::New();
  thresholder->SetInputConnection(clip->GetOutputPort());
  thresholder->ThresholdBetween(0.0, 3.0);

  // Generate the quadrature point set using a specific array as point data.
  svtkSmartPointer<svtkQuadraturePointsGenerator> pointGen =
    svtkSmartPointer<svtkQuadraturePointsGenerator>::New();
  pointGen->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "QuadratureOffset");
  pointGen->SetInputConnection(thresholder->GetOutputPort());
  svtkPolyData* output = svtkPolyData::SafeDownCast(pointGen->GetOutput());
  pointGen->Update();
  const char* activeScalars = "pressure";
  output->GetPointData()->SetActiveScalars(activeScalars);

  // Glyph the point set.
  svtkSmartPointer<svtkSphereSource> ss = svtkSmartPointer<svtkSphereSource>::New();
  ss->SetRadius(0.0008);
  svtkSmartPointer<svtkGlyph3D> glyphs = svtkSmartPointer<svtkGlyph3D>::New();
  glyphs->SetInputConnection(pointGen->GetOutputPort());
  glyphs->SetSourceConnection(ss->GetOutputPort());
  glyphs->ScalingOff();
  glyphs->SetColorModeToColorByScalar();
  // Map the glyphs.
  svtkSmartPointer<svtkPolyDataMapper> pdmQPts = svtkSmartPointer<svtkPolyDataMapper>::New();
  pdmQPts->SetInputConnection(glyphs->GetOutputPort());
  pdmQPts->SetColorModeToMapScalars();
  pdmQPts->SetScalarModeToUsePointData();
  if (output->GetPointData()->GetArray(0) == nullptr)
  {
    svtkGenericWarningMacro(<< "no point data in output of svtkQuadraturePointsGenerator");
    return EXIT_FAILURE;
  }
  pdmQPts->SetScalarRange(output->GetPointData()->GetArray(activeScalars)->GetRange());
  svtkSmartPointer<svtkActor> outputActor = svtkSmartPointer<svtkActor>::New();
  outputActor->SetMapper(pdmQPts);

  // Extract the surface of the warped input, for reference.
  svtkSmartPointer<svtkDataSetSurfaceFilter> surface =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  surface->SetInputConnection(warper->GetOutputPort());
  // Map the warped surface.
  svtkSmartPointer<svtkPolyDataMapper> pdmWSurf = svtkSmartPointer<svtkPolyDataMapper>::New();
  pdmWSurf->SetInputConnection(surface->GetOutputPort());
  pdmWSurf->ScalarVisibilityOff();
  svtkSmartPointer<svtkActor> surfaceActor = svtkSmartPointer<svtkActor>::New();
  surfaceActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  surfaceActor->GetProperty()->SetRepresentationToSurface();
  surfaceActor->SetMapper(pdmWSurf);
  // Setup left render pane.
  svtkCamera* camera = nullptr;
  svtkSmartPointer<svtkRenderer> ren0 = svtkSmartPointer<svtkRenderer>::New();
  ren0->SetViewport(0.0, 0.0, 0.5, 1.0);
  ren0->AddActor(outputActor);
  ren0->SetBackground(0.328125, 0.347656, 0.425781);
  ren0->ResetCamera();
  camera = ren0->GetActiveCamera();
  camera->Elevation(95.0);
  camera->SetViewUp(0.0, 0.0, 1.0);
  camera->Azimuth(180.0);

  // Setup upper right pane.
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  ren1->SetViewport(0.5, 0.5, 1.0, 1.0);
  ren1->AddActor(outputActor);
  ren1->AddActor(surfaceActor);
  ren1->SetBackground(0.328125, 0.347656, 0.425781);
  ren1->ResetCamera();
  camera = ren1->GetActiveCamera();
  camera->Elevation(-85.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-5.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-10.0);
  camera->Azimuth(55.0);

  // Setup lower right pane.
  svtkSmartPointer<svtkRenderer> ren2 = svtkSmartPointer<svtkRenderer>::New();
  ren2->SetViewport(0.5, 0.0, 1.0, 0.5);
  ren2->AddActor(outputActor);
  ren2->SetBackground(0.328125, 0.347656, 0.425781);
  ren2->AddActor(surfaceActor);
  ren2->ResetCamera();

  // If interactive mode then we show wireframes for
  // reference.
  if (testHelper->IsInteractiveModeSpecified())
  {
    surfaceActor->GetProperty()->SetOpacity(1.0);
    surfaceActor->GetProperty()->SetRepresentationToWireframe();
  }
  // Render window
  svtkSmartPointer<svtkRenderWindow> renwin = svtkSmartPointer<svtkRenderWindow>::New();
  renwin->AddRenderer(ren0);
  renwin->AddRenderer(ren1);
  renwin->AddRenderer(ren2);
  renwin->SetSize(800, 600);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renwin);
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
int GenerateWarpVector(svtkUnstructuredGrid* usg)
{
  svtkDoubleArray* pts = svtkArrayDownCast<svtkDoubleArray>(usg->GetPoints()->GetData());

  svtkIdType nTups = usg->GetPointData()->GetArray(0)->GetNumberOfTuples();

  double ptsBounds[6];
  usg->GetPoints()->GetBounds(ptsBounds);
  double zmax = ptsBounds[5];
  double zmin = ptsBounds[4];
  double zmid = (zmax + zmin) / 4.0;

  svtkSmartPointer<svtkDoubleArray> da = svtkSmartPointer<svtkDoubleArray>::New();
  int idx = usg->GetPointData()->AddArray(da); // note: returns the index.
  da->SetName("warp");
  da->SetNumberOfComponents(3);
  da->SetNumberOfTuples(nTups);
  double* pda = da->GetPointer(0);
  double* ppts = pts->GetPointer(0);
  for (svtkIdType i = 0; i < nTups; ++i)
  {
    double zs = (ppts[2] - zmid) / (zmax - zmid); // move z to -1 to 1
    double fzs = zs * zs * zs;                    // z**3
    double r[2];                                  // radial vector
    r[0] = ppts[0];
    r[1] = ppts[1];
    double modR = sqrt(r[0] * r[0] + r[1] * r[1]);
    r[0] /= modR; // unit radial vector
    r[0] *= fzs;  // scale by z**3 in -1 to 1
    r[1] /= modR;
    r[1] *= fzs;
    pda[0] = r[0]; // copy into result
    pda[1] = r[1];
    pda[2] = 0.0;
    pda += 3; // next
    ppts += 3;
  }
  return idx;
}
//-----------------------------------------------------------------------------
int GenerateThresholdScalar(svtkUnstructuredGrid* usg)
{
  svtkDoubleArray* pts = svtkArrayDownCast<svtkDoubleArray>(usg->GetPoints()->GetData());

  svtkIdType nTups = usg->GetPointData()->GetArray(0)->GetNumberOfTuples();

  double ptsBounds[6];
  usg->GetPoints()->GetBounds(ptsBounds);
  double zmax = ptsBounds[5];
  double zmin = ptsBounds[4];
  double zmid = (zmax + zmin) / 4.0;

  svtkSmartPointer<svtkDoubleArray> da = svtkSmartPointer<svtkDoubleArray>::New();
  int idx = usg->GetPointData()->AddArray(da); // note: returns the index.
  da->SetName("threshold");
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(nTups);
  double* pda = da->GetPointer(0);
  double* ppts = pts->GetPointer(0);
  for (svtkIdType i = 0; i < nTups; ++i)
  {
    double zs = (ppts[2] - zmid) / (zmax - zmid); // move z to -1 to 1
    double fzs = zs * zs * zs;                    // z**3
    double r[2];                                  // radial vector
    r[0] = ppts[0];
    r[1] = ppts[1];
    double modR = sqrt(r[0] * r[0] + r[1] * r[1]);
    r[1] /= modR; // scale by z**3 in -1 to 1
    r[1] *= fzs;
    pda[0] = r[1]; // copy into result
    pda += 1;      // next
    ppts += 3;
  }
  return idx;
}
