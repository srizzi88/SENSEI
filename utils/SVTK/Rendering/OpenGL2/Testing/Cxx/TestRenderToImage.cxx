#include <svtkActor.h>
#include <svtkImageActor.h>
#include <svtkImageData.h>
#include <svtkImageMapper3D.h>
#include <svtkNew.h>
#include <svtkOpenGLRenderWindow.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSphereSource.h>
#include <svtkTestUtilities.h>
#include <svtkUnsignedCharArray.h>

int TestRenderToImage(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(5.0);
  sphereSource->Update();

  // Visualize
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphereSource->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetMultiSamples(0);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindow->Render();

  // Render to the image
  svtkOpenGLRenderWindow* glRenderWindow = svtkOpenGLRenderWindow::SafeDownCast(renderWindow);

  glRenderWindow->SetShowWindow(false);
  glRenderWindow->SetUseOffScreenBuffers(true);
  renderWindow->Render();
  // Create an (empty) image at the window size
  const int* size = renderWindow->GetSize();
  svtkNew<svtkImageData> image;
  image->SetDimensions(size[0], size[1], 1);
  image->AllocateScalars(SVTK_UNSIGNED_CHAR, 3);
  renderWindow->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 0,
    svtkArrayDownCast<svtkUnsignedCharArray>(image->GetPointData()->GetScalars()));
  glRenderWindow->SetShowWindow(true);
  glRenderWindow->SetUseOffScreenBuffers(false);

  // Now add the actor
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderWindow->Render();

  glRenderWindow->SetShowWindow(false);
  glRenderWindow->SetUseOffScreenBuffers(true);
  renderWindow->Render();
  // Capture the framebuffer to the image, again
  renderWindow->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 0,
    svtkArrayDownCast<svtkUnsignedCharArray>(image->GetPointData()->GetScalars()));
  glRenderWindow->SetShowWindow(true);
  glRenderWindow->SetUseOffScreenBuffers(false);

  // Create a new image actor and remove the geometry one
  svtkNew<svtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(image);
  renderer->RemoveActor(actor);
  renderer->AddActor(imageActor);

  // Background color white to distinguish image boundary
  renderer->SetBackground(1, 1, 1);

  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();
  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
