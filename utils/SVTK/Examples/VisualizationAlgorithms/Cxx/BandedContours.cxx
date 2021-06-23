#include <svtkBandedPolyDataContourFilter.h>
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

  svtkSmartPointer<svtkBandedPolyDataContourFilter> bandedContours =
    svtkSmartPointer<svtkBandedPolyDataContourFilter>::New();
  bandedContours->SetInputConnection(reader->GetOutputPort());
  bandedContours->SetScalarModeToValue();
  bandedContours->GenerateContourEdgesOn();
  bandedContours->GenerateValues(numberOfContours, scalarRange[0], scalarRange[1]);

  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetNumberOfTableValues(numberOfContours + 1);
  lut->Build();

  svtkSmartPointer<svtkPolyDataMapper> contourMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  contourMapper->SetInputConnection(bandedContours->GetOutputPort());
  contourMapper->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourMapper->SetScalarModeToUseCellData();
  contourMapper->SetLookupTable(lut);

  svtkSmartPointer<svtkActor> contourActor = svtkSmartPointer<svtkActor>::New();
  contourActor->SetMapper(contourMapper);
  contourActor->GetProperty()->SetInterpolationToFlat();

  svtkSmartPointer<svtkPolyDataMapper> contourLineMapper = svtkSmartPointer<svtkPolyDataMapper>::New();

  contourLineMapper->SetInputData(bandedContours->GetContourEdgesOutput());
  contourLineMapper->SetScalarRange(scalarRange[0], scalarRange[1]);
  contourLineMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> contourLineActor = svtkSmartPointer<svtkActor>::New();
  contourLineActor->SetMapper(contourLineMapper);
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
