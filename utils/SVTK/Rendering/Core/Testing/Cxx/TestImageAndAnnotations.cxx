#include <svtkCornerAnnotation.h>
#include <svtkImageData.h>
#include <svtkImageMapper.h>
#include <svtkInteractorStyleImage.h>
#include <svtkProperty2D.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTextProperty.h>

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

namespace
{

svtkSmartPointer<svtkImageData> CreateColorImage(unsigned int dim, bool transparent)
{
  svtkSmartPointer<svtkImageData> image = svtkSmartPointer<svtkImageData>::New();
  image->SetDimensions(dim, dim, 1);
  image->AllocateScalars(SVTK_UNSIGNED_CHAR, 4);

  for (unsigned int x = 0; x < dim; x++)
  {
    for (unsigned int y = 0; y < dim; y++)
    {
      unsigned char* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
      pixel[0] = 255;
      pixel[1] = 0;
      pixel[2] = 255;
      pixel[3] = transparent ? 127 : 255;
    }
  }

  return image;
}

svtkSmartPointer<svtkActor2D> CreateImageActor(int dim, int displayLocation, bool transparent)
{
  svtkSmartPointer<svtkImageData> colorImage = CreateColorImage(dim, transparent);

  svtkSmartPointer<svtkImageMapper> imageMapper = svtkSmartPointer<svtkImageMapper>::New();
  imageMapper->SetInputData(colorImage);
  imageMapper->SetColorWindow(255);
  imageMapper->SetColorLevel(127.5);

  svtkSmartPointer<svtkActor2D> imageActor = svtkSmartPointer<svtkActor2D>::New();
  imageActor->SetMapper(imageMapper);
  imageActor->SetPosition(dim, 0);
  imageActor->GetProperty()->SetDisplayLocation(displayLocation);

  return imageActor;
}

} // namespace

int TestImageAndAnnotations(int argc, char* argv[])
{
  // Setup renderer
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  // Setup render window
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);

  // Setup render window interactor
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkInteractorStyleImage> style = svtkSmartPointer<svtkInteractorStyleImage>::New();
  renderWindowInteractor->SetInteractorStyle(style);

  // Setup corner annotation
  svtkSmartPointer<svtkCornerAnnotation> cornerAnnotation =
    svtkSmartPointer<svtkCornerAnnotation>::New();
  cornerAnnotation->SetLinearFontScaleFactor(2);
  cornerAnnotation->SetNonlinearFontScaleFactor(1);
  cornerAnnotation->SetMaximumFontSize(20);
  cornerAnnotation->SetText(0, "background/opaque");      // lower left
  cornerAnnotation->SetText(1, "foreground/opaque");      // lower right
  cornerAnnotation->SetText(2, "background/transparent"); // upper left
  cornerAnnotation->SetText(3, "foreground/transparent"); // upper right
  cornerAnnotation->GetTextProperty()->SetColor(1, 1, 1);

  renderer->AddViewProp(cornerAnnotation);

  // Setup images
  const unsigned int Dim = 300;
  {
    // lower left: background/opaque
    bool transparent = false;
    svtkSmartPointer<svtkActor2D> imageActor =
      CreateImageActor(Dim, SVTK_BACKGROUND_LOCATION, transparent);
    imageActor->SetPosition(0, 0);
    renderer->AddActor(imageActor);
  }
  {
    // lower right: foreground/opaque
    bool transparent = false;
    svtkSmartPointer<svtkActor2D> imageActor =
      CreateImageActor(Dim, SVTK_FOREGROUND_LOCATION, transparent);
    imageActor->SetPosition(Dim, 0);
    renderer->AddActor(imageActor);
  }
  {
    // upper left: background/transparent
    bool transparent = true;
    svtkSmartPointer<svtkActor2D> imageActor =
      CreateImageActor(Dim, SVTK_BACKGROUND_LOCATION, transparent);
    imageActor->SetPosition(0, Dim);
    renderer->AddActor(imageActor);
  }
  {
    // upper right: foreground/transparent
    bool transparent = true;
    svtkSmartPointer<svtkActor2D> imageActor =
      CreateImageActor(Dim, SVTK_FOREGROUND_LOCATION, transparent);
    imageActor->SetPosition(Dim, Dim);
    renderer->AddActor(imageActor);
  }

  renderer->ResetCamera();

  // Render and start interaction
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->Initialize();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
