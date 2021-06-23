#include <svtkNamedColors.h>
#include <svtkOBJImporter.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTexture.h>

int TestImportOBJ(int argc, char* argv[])
{
  if (argc < 4)
  {
    std::cout << "Usage: " << argv[0] << " objfile mtlfile texturepath" << std::endl;
    return EXIT_FAILURE;
  }
  auto importer = svtkSmartPointer<svtkOBJImporter>::New();
  importer->SetFileName(argv[1]);
  importer->SetFileNameMTL(argv[2]);
  importer->SetTexturePath(argv[3]);

  auto colors = svtkSmartPointer<svtkNamedColors>::New();

  auto renderer = svtkSmartPointer<svtkRenderer>::New();
  auto renWin = svtkSmartPointer<svtkRenderWindow>::New();
  auto iren = svtkSmartPointer<svtkRenderWindowInteractor>::New();

  renderer->SetBackground2(colors->GetColor3d("Silver").GetData());
  renderer->SetBackground(colors->GetColor3d("Gold").GetData());
  renderer->GradientBackgroundOn();
  renWin->AddRenderer(renderer);
  renderer->UseHiddenLineRemovalOn();
  renWin->AddRenderer(renderer);
  renWin->SetSize(640, 480);

  iren->SetRenderWindow(renWin);
  importer->SetRenderWindow(renWin);
  importer->Update();

  auto actors = renderer->GetActors();
  actors->InitTraversal();
  std::cout << "There are " << actors->GetNumberOfItems() << " actors" << std::endl;

  for (svtkIdType a = 0; a < actors->GetNumberOfItems(); ++a)
  {
    std::cout << importer->GetOutputDescription(a) << std::endl;

    svtkActor* actor = actors->GetNextActor();

    // OBJImporter turns texture interpolation off
    if (actor->GetTexture())
    {
      std::cout << "Has texture\n";
      actor->GetTexture()->InterpolateOn();
    }

    svtkPolyData* pd = dynamic_cast<svtkPolyData*>(actor->GetMapper()->GetInput());

    svtkPolyDataMapper* mapper = dynamic_cast<svtkPolyDataMapper*>(actor->GetMapper());
    mapper->SetInputData(pd);
  }
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
