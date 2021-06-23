#include <svtkAppendPolyData.h>
#include <svtkCleanPolyData.h>
#include <svtkClipPolyData.h>
#include <svtkContourFilter.h>
#include <svtkSmartPointer.h>
#include <svtkXMLPolyDataReader.h>

#include <svtkActor.h>
#include <svtkCellData.h>
#include <svtkFloatArray.h>
#include <svtkLookupTable.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkScalarsToColors.h>

#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include <vector>

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " InputPolyDataFile(.vtp) NumberOfContours" << std::endl;
    return EXIT_FAILURE;
  }

  // Read the file
  svtkSmartPointer<svtkXMLPolyDataReader> reader = svtkSmartPointer<svtkXMLPolyDataReader>::New();

  reader->SetFileName(argv[1]);
  reader->Update(); // Update so that we can get the scalar range

  double scalarRange[2];
  reader->GetOutput()->GetPointData()->GetScalars()->GetRange(scalarRange);

  svtkSmartPointer<svtkAppendPolyData> appendFilledContours =
    svtkSmartPointer<svtkAppendPolyData>::New();

  // Check for a reasonable number of contours to avoid excessive
  // computation. Here we arbitrarily pick an upper limit of 1000
  int numberOfContours = atoi(argv[2]);
  if (numberOfContours > 1000)
  {
    std::cout << "ERROR: the number of contours " << numberOfContours << " exceeds 1000"
              << std::endl;
    return EXIT_FAILURE;
  }
  if (numberOfContours <= 0)
  {
    std::cout << "ERROR: the number of contours " << numberOfContours << " is <= 0" << std::endl;
    return EXIT_FAILURE;
  }

  double delta = (scalarRange[1] - scalarRange[0]) / static_cast<double>(numberOfContours - 1);

  // Keep the clippers alive
  std::vector<svtkSmartPointer<svtkClipPolyData> > clippersLo;
  std::vector<svtkSmartPointer<svtkClipPolyData> > clippersHi;

  for (int i = 0; i < numberOfContours; i++)
  {
    double valueLo = scalarRange[0] + static_cast<double>(i) * delta;
    double valueHi = scalarRange[0] + static_cast<double>(i + 1) * delta;
    clippersLo.push_back(svtkSmartPointer<svtkClipPolyData>::New());
    clippersLo[i]->SetValue(valueLo);
    if (i == 0)
    {
      clippersLo[i]->SetInputConnection(reader->GetOutputPort());
    }
    else
    {
      clippersLo[i]->SetInputConnection(clippersHi[i - 1]->GetOutputPort(1));
    }
    clippersLo[i]->InsideOutOff();
    clippersLo[i]->Update();

    clippersHi.push_back(svtkSmartPointer<svtkClipPolyData>::New());
    clippersHi[i]->SetValue(valueHi);
    clippersHi[i]->SetInputConnection(clippersLo[i]->GetOutputPort());
    clippersHi[i]->GenerateClippedOutputOn();
    clippersHi[i]->InsideOutOn();
    clippersHi[i]->Update();
    if (clippersHi[i]->GetOutput()->GetNumberOfCells() == 0)
    {
      continue;
    }

    svtkSmartPointer<svtkFloatArray> cd = svtkSmartPointer<svtkFloatArray>::New();
    cd->SetNumberOfComponents(1);
    cd->SetNumberOfTuples(clippersHi[i]->GetOutput()->GetNumberOfCells());
    cd->FillComponent(0, valueLo);

    clippersHi[i]->GetOutput()->GetCellData()->SetScalars(cd);
    appendFilledContours->AddInputConnection(clippersHi[i]->GetOutputPort());
  }

  svtkSmartPointer<svtkCleanPolyData> filledContours = svtkSmartPointer<svtkCleanPolyData>::New();
  filledContours->SetInputConnection(appendFilledContours->GetOutputPort());

  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetNumberOfTableValues(numberOfContours + 1);
  lut->Build();
  svtkSmartPointer<svtkPolyDataMapper> contourMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  contourMapper->SetInputConnection(filledContours->GetOutputPort());
  contourMapper->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourMapper->SetScalarModeToUseCellData();
  contourMapper->SetLookupTable(lut);

  svtkSmartPointer<svtkActor> contourActor = svtkSmartPointer<svtkActor>::New();
  contourActor->SetMapper(contourMapper);
  contourActor->GetProperty()->SetInterpolationToFlat();

  svtkSmartPointer<svtkContourFilter> contours = svtkSmartPointer<svtkContourFilter>::New();
  contours->SetInputConnection(filledContours->GetOutputPort());
  contours->GenerateValues(numberOfContours, scalarRange[0], scalarRange[1]);

  svtkSmartPointer<svtkPolyDataMapper> contourLineMapperer =
    svtkSmartPointer<svtkPolyDataMapper>::New();
  contourLineMapperer->SetInputConnection(contours->GetOutputPort());
  contourLineMapperer->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourLineMapperer->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> contourLineActor = svtkSmartPointer<svtkActor>::New();
  contourLineActor->SetMapper(contourLineMapperer);
  contourLineActor->GetProperty()->SetLineWidth(2);

  // The usual renderer, render window and interactor
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  ren1->SetBackground(.1, .2, .3);
  renWin->AddRenderer(ren1);
  iren->SetRenderWindow(renWin);

  // Add the actors
  ren1->AddActor(contourActor);
  ren1->AddActor(contourLineActor);

  // Begin interaction
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
