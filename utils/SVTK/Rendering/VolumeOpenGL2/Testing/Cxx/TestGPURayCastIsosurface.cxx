#include <svtkColorTransferFunction.h>
#include <svtkContourValues.h>
#include <svtkDataArray.h>
#include <svtkFloatingPointExceptions.h>
#include <svtkImageData.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOpenGLGPUVolumeRayCastMapper.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPointData.h>
#include <svtkRTAnalyticSource.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkVolume.h>
#include <svtkVolumeProperty.h>

//----------------------------------------------------------------------------
int TestGPURayCastIsosurface(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{

  svtkFloatingPointExceptions::Disable();
  svtkNew<svtkRTAnalyticSource> data;
  data->SetWholeExtent(-100, 100, -100, 100, -100, 100);
  data->Update();
  std::cout << "range: " << data->GetOutput()->GetPointData()->GetScalars()->GetRange()[0] << ", "
            << data->GetOutput()->GetPointData()->GetScalars()->GetRange()[1] << std::endl;
  ;
  svtkNew<svtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(data->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.5);
  mapper->SetBlendModeToIsoSurface();

  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(220.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(150.0, 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(190.0, 0.0, 1.0, 1.0);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(220.0, 1.0);
  scalarOpacity->AddPoint(150.0, 0.2);
  scalarOpacity->AddPoint(190.0, 0.6);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  svtkNew<svtkRenderer> renderer;
  renderer->AddVolume(volume);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkInteractorStyleTrackballCamera> style;

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->SetInteractorStyle(style);

  // Check that no errors is created when no contour values are set
  renderWindow->Render();
  volumeProperty->GetIsoSurfaceValues()->SetValue(0, 220.0);
  renderWindow->Render();
  volumeProperty->GetIsoSurfaceValues()->SetNumberOfContours(0);
  renderWindow->Render();

  // Now add some contour values to draw iso surfaces
  volumeProperty->GetIsoSurfaceValues()->SetValue(0, 220.0);
  volumeProperty->GetIsoSurfaceValues()->SetValue(1, 150.0);
  volumeProperty->GetIsoSurfaceValues()->SetValue(2, 190.0);

  renderWindow->Render();

  renderWindowInteractor->Start();

  return 0;
}
