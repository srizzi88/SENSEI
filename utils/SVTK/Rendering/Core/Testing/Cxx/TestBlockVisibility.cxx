#include "svtkActor.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkCubeSource.h"
#include "svtkDataArray.h"
#include "svtkLookupTable.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

#include <set>

static svtkSmartPointer<svtkMultiBlockDataSet> svtkCreateData()
{
  auto data = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  data->SetNumberOfBlocks(3 * 3 * 2);
  int blk = 0;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      svtkNew<svtkSphereSource> ssrc;
      ssrc->SetRadius(0.4);
      ssrc->SetCenter(i, j, 0.0);
      ssrc->Update();

      svtkNew<svtkCubeSource> csrc;
      csrc->SetBounds(i - 0.4, i + 0.4, j - 0.4, j + 0.4, 0.6, 1.4);
      csrc->Update();

      svtkNew<svtkPolyData> sphere;
      svtkNew<svtkPolyData> cube;

      sphere->DeepCopy(ssrc->GetOutputDataObject(0));
      cube->DeepCopy(csrc->GetOutputDataObject(0));
      data->SetBlock(blk++, sphere);
      data->SetBlock(blk++, cube);
    }
  }
  return data;
}

int TestBlockVisibility(int argc, char* argv[])
{
  // Standard rendering classes
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // We create a multiblock dataset with 18 blocks (spheres & cubes) and set the
  // block visibility to a pattern.

  auto mbds = svtkCreateData();

  svtkSmartPointer<svtkCompositePolyDataMapper2> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper2>::New();
  mapper->SetInputDataObject(mbds);
  // mapper->SetColorModeToMapScalars();
  // mapper->SetScalarModeToUsePointData();
  // mapper->ScalarVisibilityOn();
  mapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkCompositeDataDisplayAttributes> attrs =
    svtkSmartPointer<svtkCompositeDataDisplayAttributes>::New();
  mapper->SetCompositeDataDisplayAttributes(attrs);

  const int visblocks[] = { 0, 3, 4, 7, 8, 11, 13, 14, 17 };
  std::set<int> vis(visblocks, visblocks + sizeof(visblocks) / sizeof(visblocks[0]));
  for (int i = 0; i < static_cast<int>(mbds->GetNumberOfBlocks()); ++i)
  {
    svtkDataObject* blk = mbds->GetBlock(i);
    attrs->SetBlockVisibility(blk, vis.find(i) != vis.end() ? 1 : 0);
  }

  int numVisited = 0;
  int numVisible = 0;
  attrs->VisitVisibilities([&numVisited, &numVisible](svtkDataObject*, bool visible) {
    if (visible)
    {
      ++numVisible;
    }
    ++numVisited;
    return false; // do not terminate loop early.
  });

  if (numVisited != static_cast<int>(mbds->GetNumberOfBlocks()))
  {
    svtkGenericWarningMacro("ERROR: Visited " << numVisited << " blocks instead of expected "
                                             << mbds->GetNumberOfBlocks());
  }

  if (numVisible != static_cast<int>(vis.size()))
  {
    svtkGenericWarningMacro(
      "ERROR: " << numVisible << " visible blocks instead of expected " << vis.size());
  }

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
