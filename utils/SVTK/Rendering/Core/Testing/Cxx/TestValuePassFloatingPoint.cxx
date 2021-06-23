/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestValuePassFloatingPoint.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description:
// Tests svtkValuePass in FLOATING_POINT mode. The test generates a 3-component
// float array ("elevationVector") using the loaded polygonal data (points and cells).
// Polygons are rendered with the ValuePass to its internal floating point frame-buffer.
// The rendered float image is then queried from the svtkValuePass and used to
// generate a color image using svtkLookupTable, the color image is rendered with
// an image actor on-screen. This is repeated for each component.

#include "svtkActor.h"
#include "svtkArrayCalculator.h"
#include "svtkCamera.h"
#include "svtkCameraPass.h"
#include "svtkCellData.h"
#include "svtkElevationFilter.h"
#include "svtkFloatArray.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLookupTable.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPointData.h"
#include "svtkPointDataToCellData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSequencePass.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkValuePass.h"

void GenerateElevationArray(svtkSmartPointer<svtkPolyDataAlgorithm> source)
{
  svtkPolyData* data = source->GetOutput();
  const double* bounds = data->GetBounds();

  svtkSmartPointer<svtkElevationFilter> elevation = svtkSmartPointer<svtkElevationFilter>::New();
  elevation->SetInputConnection(source->GetOutputPort());

  // Use svtkElevation to generate an array per component. svtkElevation generates
  // a projected distance from each point in the dataset to the line, with respect to
  // the LowPoint ([0, 1] in this case. This is different from having the actual
  // coordinates of a given point.
  for (int c = 0; c < 3; c++)
  {
    std::string name;
    switch (c)
    {
      case 0:
        name = "delta_x";
        elevation->SetLowPoint(bounds[0], 0.0, 0.0);
        elevation->SetHighPoint(bounds[1], 0.0, 0.0);
        break;
      case 1:
        name = "delta_y";
        elevation->SetLowPoint(0.0, bounds[2], 0.0);
        elevation->SetHighPoint(0.0, bounds[3], 0.0);
        break;
      case 2:
        name = "delta_z";
        elevation->SetLowPoint(0.0, 0.0, bounds[4]);
        elevation->SetHighPoint(0.0, 0.0, bounds[5]);
        break;
    }
    elevation->Update();

    svtkPolyData* result = svtkPolyData::SafeDownCast(elevation->GetOutput());
    int outCellFlag;
    // Enums defined in svtkAbstractMapper
    svtkDataArray* elevArray =
      svtkAbstractMapper::GetScalars(result, SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA,
        SVTK_GET_ARRAY_BY_NAME /*acc mode*/, 0 /*arr id*/, "Elevation" /*arr name*/, outCellFlag);
    if (!elevArray)
    {
      std::cout << "->> Error: could not find array!" << std::endl;
      return;
    }

    elevArray->SetName(name.c_str());
    data->GetPointData()->AddArray(elevArray);
  }

  // Generate a 3-component vector array using the single components
  // form elevation

  // Point data
  svtkSmartPointer<svtkArrayCalculator> calc = svtkSmartPointer<svtkArrayCalculator>::New();
  calc->SetInputConnection(source->GetOutputPort());
  calc->SetAttributeTypeToPointData();
  calc->AddScalarArrayName("delta_x");
  calc->AddScalarArrayName("delta_y");
  calc->AddScalarArrayName("delta_z");
  calc->SetFunction("delta_x * iHat + delta_y * jHat + delta_z * kHat");
  calc->SetResultArrayName("elevationVector");
  calc->Update();

  // Cell data
  svtkSmartPointer<svtkPointDataToCellData> p2c = svtkSmartPointer<svtkPointDataToCellData>::New();
  p2c->SetInputConnection(calc->GetOutputPort());
  p2c->PassPointDataOn();
  p2c->Update();

  /// Include the elevation vector (point and cell data) in the original data
  svtkPolyData* outputP2c = svtkPolyData::SafeDownCast(p2c->GetOutput());
  data->GetPointData()->AddArray(
    svtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->GetArray("elevationVector"));
  data->GetCellData()->AddArray(outputP2c->GetCellData()->GetArray("elevationVector"));
};

//------------------------------------------------------------------------------
void RenderComponentImages(std::vector<svtkSmartPointer<svtkImageData> >& colorImOut,
  svtkRenderWindow* window, svtkRenderer* renderer, svtkValuePass* valuePass, int dataMode,
  char const* name)
{
  valuePass->SetInputArrayToProcess(dataMode, name);

  // Prepare a lut to map the floating point values
  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetAlpha(1.0);
  lut->Build();

  // Render each component in a separate image
  for (int c = 0; c < 3; c++)
  {
    valuePass->SetInputComponentToProcess(c);
    window->Render();

    /// Get the resulting values
    svtkFloatArray* result = valuePass->GetFloatImageDataArray(renderer);
    int* ext = valuePass->GetFloatImageExtents();

    // Map the resulting float image to a color table
    svtkUnsignedCharArray* colored =
      lut->MapScalars(result, SVTK_COLOR_MODE_DEFAULT, 0 /* single comp*/);

    // Create an image dataset to render in a quad.
    svtkSmartPointer<svtkImageData> colorIm = svtkSmartPointer<svtkImageData>::New();
    colorIm->SetExtent(ext);
    colorIm->GetPointData()->SetScalars(colored);
    colorImOut.push_back(colorIm);
    colored->Delete();
  }
};

///////////////////////////////////////////////////////////////////////////////
int TestValuePassFloatingPoint(int argc, char* argv[])
{
  // Load data
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetThetaResolution(8.0);
  sphere->SetPhiResolution(8.0);
  sphere->Update();

  // Prepare a 3-component array (data will be appended to reader's output)
  GenerateElevationArray(sphere);
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(sphere->GetOutput());
  mapper->ScalarVisibilityOn();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // Setup rendering and interaction
  svtkSmartPointer<svtkRenderWindowInteractor> interactor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  svtkSmartPointer<svtkInteractorStyleTrackballCamera> style =
    svtkSmartPointer<svtkInteractorStyleTrackballCamera>::New();
  interactor->SetInteractorStyle(style);

  svtkSmartPointer<svtkRenderWindow> window = svtkSmartPointer<svtkRenderWindow>::New();
  window->SetMultiSamples(0);
  window->SetSize(640, 640);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  window->AddRenderer(renderer);
  interactor->SetRenderWindow(window);

  renderer->AddActor(actor);
  renderer->SetBackground(0.2, 0.2, 0.5);

  // Setup the value pass
  int const comp = 0;

  svtkSmartPointer<svtkValuePass> valuePass = svtkSmartPointer<svtkValuePass>::New();
  valuePass->SetInputComponentToProcess(comp);
  // Initial data mode
  valuePass->SetInputArrayToProcess(SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA, "elevationVector");
  // valuePass->SetInputArrayToProcess(SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA,
  //  "elevationVector");

  // 3. Add it to a sequence of passes
  svtkSmartPointer<svtkRenderPassCollection> passes = svtkSmartPointer<svtkRenderPassCollection>::New();
  passes->AddItem(valuePass);

  svtkSmartPointer<svtkSequencePass> sequence = svtkSmartPointer<svtkSequencePass>::New();
  sequence->SetPasses(passes);

  svtkSmartPointer<svtkCameraPass> cameraPass = svtkSmartPointer<svtkCameraPass>::New();
  cameraPass->SetDelegatePass(sequence);

  svtkOpenGLRenderer* glRenderer = svtkOpenGLRenderer::SafeDownCast(renderer);

  // Render the value pass
  glRenderer->SetPass(cameraPass);
  window->Render();

  // Render point data images
  std::vector<svtkSmartPointer<svtkImageData> > colorImagesPoint;
  RenderComponentImages(colorImagesPoint, window, renderer, valuePass,
    SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA, "elevationVector");

  // Render cell data images
  std::vector<svtkSmartPointer<svtkImageData> > colorImagesCell;
  RenderComponentImages(colorImagesCell, window, renderer, valuePass,
    SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA, "elevationVector");

  ////// Render results on-screen
  renderer->RemoveActor(actor);

  // Add image actors to display the point dataArray's components
  svtkSmartPointer<svtkImageActor> ia_x = svtkSmartPointer<svtkImageActor>::New();
  ia_x->GetMapper()->SetInputData(colorImagesPoint.at(0));
  renderer->AddActor(ia_x);

  svtkSmartPointer<svtkImageActor> ia_y = svtkSmartPointer<svtkImageActor>::New();
  ia_y->RotateX(90);
  ia_y->GetMapper()->SetInputData(colorImagesPoint.at(1));
  renderer->AddActor(ia_y);

  svtkSmartPointer<svtkImageActor> ia_z = svtkSmartPointer<svtkImageActor>::New();
  ia_z->RotateY(-90);
  ia_z->GetMapper()->SetInputData(colorImagesPoint.at(2));
  renderer->AddActor(ia_z);

  // Add image actors to display cell dataArray's components
  svtkSmartPointer<svtkImageActor> iacell_x = svtkSmartPointer<svtkImageActor>::New();
  iacell_x->SetPosition(-500, 600, 600);
  iacell_x->GetMapper()->SetInputData(colorImagesCell.at(0));
  renderer->AddActor(iacell_x);

  svtkSmartPointer<svtkImageActor> iacell_y = svtkSmartPointer<svtkImageActor>::New();
  iacell_y->RotateX(90);
  iacell_y->SetPosition(-500, 600, 600);
  iacell_y->GetMapper()->SetInputData(colorImagesCell.at(1));
  renderer->AddActor(iacell_y);

  svtkSmartPointer<svtkImageActor> iacell_z = svtkSmartPointer<svtkImageActor>::New();
  iacell_z->RotateY(-90);
  iacell_z->SetPosition(-500, 600, 600);
  iacell_z->GetMapper()->SetInputData(colorImagesCell.at(2));
  renderer->AddActor(iacell_z);

  // Adjust viewpoint
  svtkCamera* cam = renderer->GetActiveCamera();
  cam->SetPosition(2, 2, 2);
  cam->SetFocalPoint(0, 0, 1);
  renderer->ResetCamera();

  // Use the default pass to render the colored image.
  glRenderer->SetPass(nullptr);
  window->Render();

  // initialize render loop
  int retVal = svtkRegressionTestImage(window);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  valuePass->ReleaseGraphicsResources(window);
  return !retVal;
}
