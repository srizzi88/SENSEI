#include <svtkColorTransferFunction.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOpenGLGPUVolumeRayCastMapper.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkRTAnalyticSource.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkVolume.h>
#include <svtkVolumeProperty.h>

//----------------------------------------------------------------------------
int TestGPURayCastSlicePlane(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkNew<svtkRTAnalyticSource> data;
  data->SetWholeExtent(-100, 100, -100, 100, -100, 100);
  data->Update();

  svtkNew<svtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(data->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.5);
  mapper->SetBlendModeToSlice();

  // we also test if slicing works with cropping
  mapper->SetCroppingRegionPlanes(0, 100, -100, 100, -100, 100);
  mapper->CroppingOn();

  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(220.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(150.0, 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(190.0, 0.0, 1.0, 1.0);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(220.0, 1.0);
  scalarOpacity->AddPoint(150.0, 0.2);
  scalarOpacity->AddPoint(190.0, 0.6);

  svtkNew<svtkPlane> slice;
  slice->SetOrigin(1.0, 0.0, 0.0);
  slice->SetNormal(0.707107, 0.0, 0.707107);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);
  volumeProperty->SetSliceFunction(slice);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  svtkNew<svtkRenderer> renderer;
  renderer->AddVolume(volume);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkInteractorStyleTrackballCamera> style;

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->SetInteractorStyle(style);

  renderWindow->Render();
  renderWindowInteractor->Start();

  return 0;
}
