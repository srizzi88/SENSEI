// SVTK: Spiral with svtkTubeFilter
// Varying tube radius and independent RGB colors with an unsignedCharArray
// Contributed by Marcus Thamson

#include <svtkCellArray.h>
#include <svtkDoubleArray.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>

#include <svtkCell.h>
#include <svtkCellData.h>
#include <svtkDataSet.h>
#include <svtkDataSetAttributes.h>
#include <svtkProperty.h>
#include <svtkSmartPointer.h>
#include <svtkTubeFilter.h>

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkDataSetMapper.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include <svtkMath.h>

int main(int, char*[])
{
  // Spiral tube
  double vX, vY, vZ;
  unsigned int nV = 256;       // No. of vertices
  unsigned int nCyc = 5;       // No. of spiral cycles
  double rT1 = 0.1, rT2 = 0.5; // Start/end tube radii
  double rS = 2;               // Spiral radius
  double h = 10;               // Height
  unsigned int nTv = 8;        // No. of surface elements for each tube vertex

  unsigned int i;

  // Create points and cells for the spiral
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  for (i = 0; i < nV; i++)
  {
    // Spiral coordinates
    vX = rS * cos(2 * svtkMath::Pi() * nCyc * i / (nV - 1));
    vY = rS * sin(2 * svtkMath::Pi() * nCyc * i / (nV - 1));
    vZ = h * i / nV;
    points->InsertPoint(i, vX, vY, vZ);
  }

  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  lines->InsertNextCell(nV);
  for (i = 0; i < nV; i++)
  {
    lines->InsertCellPoint(i);
  }

  svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetLines(lines);

  // Varying tube radius using sine-function
  svtkSmartPointer<svtkDoubleArray> tubeRadius = svtkSmartPointer<svtkDoubleArray>::New();
  tubeRadius->SetName("TubeRadius");
  tubeRadius->SetNumberOfTuples(nV);
  for (i = 0; i < nV; i++)
  {
    tubeRadius->SetTuple1(i, rT1 + (rT2 - rT1) * sin(svtkMath::Pi() * i / (nV - 1)));
  }
  polyData->GetPointData()->AddArray(tubeRadius);
  polyData->GetPointData()->SetActiveScalars("TubeRadius");

  // RBG array (could add Alpha channel too I guess...)
  // Varying from blue to red
  svtkSmartPointer<svtkUnsignedCharArray> colors = svtkSmartPointer<svtkUnsignedCharArray>::New();
  colors->SetName("Colors");
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(nV);
  for (i = 0; i < nV; i++)
  {
    colors->InsertTuple3(i, int(255 * i / (nV - 1)), 0, int(255 * (nV - 1 - i) / (nV - 1)));
  }
  polyData->GetPointData()->AddArray(colors);

  svtkSmartPointer<svtkTubeFilter> tube = svtkSmartPointer<svtkTubeFilter>::New();
  tube->SetInputData(polyData);
  tube->SetNumberOfSides(nTv);
  tube->SetVaryRadiusToVaryRadiusByAbsoluteScalar();

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(tube->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Colors");

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AddActor(actor);
  renderer->SetBackground(.2, .3, .4);

  // Make an oblique view
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->ResetCamera();

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);
  renWin->SetSize(500, 500);
  renWin->Render();

  svtkSmartPointer<svtkInteractorStyleTrackballCamera> style =
    svtkSmartPointer<svtkInteractorStyleTrackballCamera>::New();
  iren->SetInteractorStyle(style);

  iren->Start();

  return EXIT_SUCCESS;
}
