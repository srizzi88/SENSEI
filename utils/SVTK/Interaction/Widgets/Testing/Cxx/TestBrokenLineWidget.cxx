#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkBrokenLineWidget.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDataSetMapper.h"
#include "svtkExtractSelection.h"
#include "svtkInformation.h"
#include "svtkLinearSelector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProgrammableFilter.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"

#include "svtkTestUtilities.h"

#include <sstream>

// Callback for the broken line widget interaction
class svtkBLWCallback : public svtkCommand
{
public:
  static svtkBLWCallback* New() { return new svtkBLWCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    // Retrieve polydata line
    svtkBrokenLineWidget* line = reinterpret_cast<svtkBrokenLineWidget*>(caller);
    line->GetPolyData(Poly);

    // Update linear extractor with current points
    this->Selector->SetPoints(Poly->GetPoints());

    // Update selection from mesh
    this->Extractor->Update();
    svtkMultiBlockDataSet* outMB = svtkMultiBlockDataSet::SafeDownCast(this->Extractor->GetOutput());
    svtkUnstructuredGrid* selection = svtkUnstructuredGrid::SafeDownCast(outMB->GetBlock(0));
    this->Mapper->SetInputData(selection);

    // Update cardinality of selection
    std::ostringstream txt;
    txt << "Number of selected elements: " << (selection ? selection->GetNumberOfCells() : 0);
    this->Text->SetInput(txt.str().c_str());
  }
  svtkBLWCallback()
    : Poly(nullptr)
    , Selector(nullptr)
    , Extractor(nullptr)
    , Mapper(nullptr)
    , Text(nullptr)
  {
  }
  svtkPolyData* Poly;
  svtkLinearSelector* Selector;
  svtkExtractSelection* Extractor;
  svtkDataSetMapper* Mapper;
  svtkTextActor* Text;
};

int TestBrokenLineWidget(int argc, char* argv[])
{
  // Create render window and interactor
  svtkSmartPointer<svtkRenderWindow> win = svtkSmartPointer<svtkRenderWindow>::New();
  win->SetMultiSamples(0);
  win->SetSize(600, 300);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(win);
  iren->Initialize();

  // Create 2 viewports in window
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  ren1->SetBackground(.4, .4, .4);
  ren1->SetBackground2(.8, .8, .8);
  ren1->GradientBackgroundOn();
  ren1->SetViewport(0., 0., .5, 1.);
  win->AddRenderer(ren1);
  svtkSmartPointer<svtkRenderer> ren2 = svtkSmartPointer<svtkRenderer>::New();
  ren2->SetBackground(1., 1., 1.);
  ren2->SetViewport(.5, 0., 1., 1.);
  win->AddRenderer(ren2);

  // Create a good view angle
  svtkCamera* camera = ren1->GetActiveCamera();
  camera->SetFocalPoint(.12, 0., 0.);
  camera->SetPosition(.38, .3, .15);
  camera->SetViewUp(0., 0., 1.);
  ren2->SetActiveCamera(camera);

  // Read 3D unstructured input mesh
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AngularSector.svtk");
  svtkSmartPointer<svtkUnstructuredGridReader> reader =
    svtkSmartPointer<svtkUnstructuredGridReader>::New();
  reader->SetFileName(fileName);
  delete[] fileName;
  reader->Update();

  // Create mesh actor to be rendered in viewport 1
  svtkSmartPointer<svtkDataSetMapper> meshMapper = svtkSmartPointer<svtkDataSetMapper>::New();
  meshMapper->SetInputConnection(reader->GetOutputPort());
  svtkSmartPointer<svtkActor> meshActor = svtkSmartPointer<svtkActor>::New();
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->SetColor(.23, .37, .17);
  meshActor->GetProperty()->SetRepresentationToWireframe();
  ren1->AddActor(meshActor);

  // Create multi-block mesh for linear extractor
  reader->Update();
  svtkUnstructuredGrid* mesh = reader->GetOutput();
  svtkSmartPointer<svtkMultiBlockDataSet> meshMB = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  meshMB->SetNumberOfBlocks(1);
  meshMB->GetMetaData(static_cast<unsigned>(0))->Set(svtkCompositeDataSet::NAME(), "Mesh");
  meshMB->SetBlock(0, mesh);

  // Create broken line widget, attach it to input mesh
  svtkSmartPointer<svtkBrokenLineWidget> line = svtkSmartPointer<svtkBrokenLineWidget>::New();
  line->SetInteractor(iren);
  line->SetInputData(mesh);
  line->SetPriority(1.);
  line->KeyPressActivationOff();
  line->PlaceWidget();
  line->ProjectToPlaneOff();
  line->On();
  line->SetHandleSizeFactor(1.2);

  // Create list of points to define broken line
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(.23, .0, .0);
  points->InsertNextPoint(.0, .0, .0);
  points->InsertNextPoint(.23, .04, .04);
  line->InitializeHandles(points);

  // Extract polygonal line and then its points
  svtkSmartPointer<svtkPolyData> linePD = svtkSmartPointer<svtkPolyData>::New();
  line->GetPolyData(linePD);
  svtkSmartPointer<svtkPolyDataMapper> lineMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  lineMapper->SetInputData(linePD);
  svtkSmartPointer<svtkActor> lineActor = svtkSmartPointer<svtkActor>::New();
  lineActor->SetMapper(lineMapper);
  lineActor->GetProperty()->SetColor(1., 0., 0.);
  lineActor->GetProperty()->SetLineWidth(2.);
  ren2->AddActor(lineActor);

  // Create selection along broken line defined by list of points
  svtkSmartPointer<svtkLinearSelector> selector = svtkSmartPointer<svtkLinearSelector>::New();
  selector->SetInputData(meshMB);
  selector->SetPoints(points);
  selector->IncludeVerticesOff();
  selector->SetVertexEliminationTolerance(1.e-12);

  // Extract selection from mesh
  svtkSmartPointer<svtkExtractSelection> extractor = svtkSmartPointer<svtkExtractSelection>::New();
  extractor->SetInputData(0, meshMB);
  extractor->SetInputConnection(1, selector->GetOutputPort());
  extractor->Update();
  svtkMultiBlockDataSet* outMB = svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutput());
  svtkUnstructuredGrid* selection = svtkUnstructuredGrid::SafeDownCast(outMB->GetBlock(0));

  // Create selection actor
  svtkSmartPointer<svtkDataSetMapper> selMapper = svtkSmartPointer<svtkDataSetMapper>::New();
  selMapper->SetInputData(selection);
  svtkSmartPointer<svtkActor> selActor = svtkSmartPointer<svtkActor>::New();
  selActor->SetMapper(selMapper);
  selActor->GetProperty()->SetColor(0., 0., 0.);
  selActor->GetProperty()->SetRepresentationToWireframe();
  ren2->AddActor(selActor);

  // Annotate with number of elements
  svtkSmartPointer<svtkTextActor> txtActor = svtkSmartPointer<svtkTextActor>::New();
  std::ostringstream txt;
  txt << "Number of selected elements: " << (selection ? selection->GetNumberOfCells() : 0);
  txtActor->SetInput(txt.str().c_str());
  txtActor->SetTextScaleModeToViewport();
  txtActor->SetNonLinearFontScale(.2, 18);
  txtActor->GetTextProperty()->SetColor(0., 0., 1.);
  txtActor->GetTextProperty()->SetFontSize(18);
  ren2->AddActor(txtActor);

  // Invoke callback on polygonal line to interactively select elements
  svtkSmartPointer<svtkBLWCallback> cb = svtkSmartPointer<svtkBLWCallback>::New();
  cb->Poly = linePD;
  cb->Selector = selector;
  cb->Extractor = extractor;
  cb->Mapper = selMapper;
  cb->Text = txtActor;
  line->AddObserver(svtkCommand::InteractionEvent, cb);

  // Render and test
  win->Render();
  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
