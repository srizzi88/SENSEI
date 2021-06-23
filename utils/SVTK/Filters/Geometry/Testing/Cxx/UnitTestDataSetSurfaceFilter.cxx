/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestDataSetSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkDataSetSurfaceFilter.h"

#include "svtkAppendFilter.h"
#include "svtkPlaneSource.h"
#include "svtkRegularPolygonSource.h"
#include "svtkStripper.h"
#include "svtkTriangleFilter.h"

#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkUniformGrid.h"
#include "svtkUnstructuredGrid.h"

#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkPointData.h"
#include "svtkPointLocator.h"
#include "svtkPoints.h"

#include "svtkGenericCell.h"
#include "svtkHexahedron.h"
#include "svtkPolyData.h"
#include "svtkPolyLine.h"
#include "svtkQuadraticWedge.h"
#include "svtkTetra.h"

#include "svtkCommand.h"
#include "svtkTestErrorObserver.h"

#include <map>
#include <sstream>

static svtkSmartPointer<svtkDataSet> CreatePolyData(const int xres, const int yres);
static svtkSmartPointer<svtkDataSet> CreateTriangleStripData(const int xres, const int yres);
static svtkSmartPointer<svtkDataSet> CreateTetraData();
static svtkSmartPointer<svtkDataSet> CreatePolygonData(int sides = 6);
static svtkSmartPointer<svtkDataSet> CreateQuadraticWedgeData();
static svtkSmartPointer<svtkDataSet> CreateUniformGrid(unsigned int, unsigned int, unsigned int);
static svtkSmartPointer<svtkDataSet> CreateRectilinearGrid();
static svtkSmartPointer<svtkDataSet> CreateStructuredGrid(bool blank = false);
static svtkSmartPointer<svtkDataSet> CreateBadAttributes();
static svtkSmartPointer<svtkDataSet> CreateGenericCellData(int cellType);

namespace test
{
// What to expect for a cell
class CellDescription
{
public:
  CellDescription(int cellType, int numCells)
  {
    this->Type = cellType;
    this->Cells = numCells;
  }
  CellDescription()
    : Type(0)
    , Cells(0)
  {
  }
  int Type;
  int Cells;
};
}
int UnitTestDataSetSurfaceFilter(int, char*[])
{
  int status = EXIT_SUCCESS;
  {
    std::cout << "Testing empty print...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    std::ostringstream emptyPrint;
    filter->Print(emptyPrint);
    std::cout << "PASSED." << std::endl;
  }
  {
    std::map<std::string, test::CellDescription> typesToProcess;
    typesToProcess["Vertex"] = test::CellDescription(SVTK_VERTEX, 1);
    typesToProcess["Line"] = test::CellDescription(SVTK_LINE, 1);
    typesToProcess["Triangle"] = test::CellDescription(SVTK_TRIANGLE, 1);
    typesToProcess["Pixel"] = test::CellDescription(SVTK_PIXEL, 1);
    typesToProcess["Quad"] = test::CellDescription(SVTK_QUAD, 1);
    typesToProcess["Tetra"] = test::CellDescription(SVTK_TETRA, 4);
    typesToProcess["Voxel"] = test::CellDescription(SVTK_VOXEL, 6);
    typesToProcess["Hexahedron"] = test::CellDescription(SVTK_HEXAHEDRON, 6);
    typesToProcess["Wedge"] = test::CellDescription(SVTK_WEDGE, 5);
    typesToProcess["Pyramid"] = test::CellDescription(SVTK_PYRAMID, 5);
    typesToProcess["PentagonalPrism"] = test::CellDescription(SVTK_PENTAGONAL_PRISM, 7);
    typesToProcess["HexagonalPrism"] = test::CellDescription(SVTK_HEXAGONAL_PRISM, 8);
    typesToProcess["QuadraticEdge"] = test::CellDescription(SVTK_QUADRATIC_EDGE, 2);
    typesToProcess["QuadraticTriangle"] = test::CellDescription(SVTK_QUADRATIC_TRIANGLE, 1);
    typesToProcess["QuadraticQuad"] = test::CellDescription(SVTK_QUADRATIC_QUAD, 1);
    typesToProcess["QuadraticTetra"] = test::CellDescription(SVTK_QUADRATIC_TETRA, 16);
    typesToProcess["QuadraticHexahedron"] = test::CellDescription(SVTK_QUADRATIC_HEXAHEDRON, 36);
    typesToProcess["QuadraticWedge"] = test::CellDescription(SVTK_QUADRATIC_WEDGE, 26);
    typesToProcess["QuadraticPyramid"] = test::CellDescription(SVTK_QUADRATIC_PYRAMID, 22);
    typesToProcess["BiQuadraticQuad"] = test::CellDescription(SVTK_BIQUADRATIC_QUAD, 8);
    typesToProcess["TriQuadraticHexahedron"] =
      test::CellDescription(SVTK_TRIQUADRATIC_HEXAHEDRON, 768);
    typesToProcess["QuadraticLinearQuad"] = test::CellDescription(SVTK_QUADRATIC_LINEAR_QUAD, 4);
    typesToProcess["QuadraticLinearWedge"] = test::CellDescription(SVTK_QUADRATIC_LINEAR_WEDGE, 20);
    typesToProcess["BiQuadraticQuadraticWedge"] =
      test::CellDescription(SVTK_BIQUADRATIC_QUADRATIC_WEDGE, 32);

    std::map<std::string, test::CellDescription>::iterator it;
    for (it = typesToProcess.begin(); it != typesToProcess.end(); ++it)
    {
      std::cout << "Testing (" << it->first << ")...";
      svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
        svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
      filter->SetInputData(CreateGenericCellData(it->second.Type));
      filter->PassThroughCellIdsOn();
      filter->PassThroughPointIdsOn();
      if (it->first == "QuadraticTriangle" || it->first == "QuadraticQuad")
      {
        filter->SetNonlinearSubdivisionLevel(0);
      }
      if (it->first == "TriQuadraticHexahedron")
      {
        filter->SetNonlinearSubdivisionLevel(3);
      }
      filter->Update();
      int got = filter->GetOutput()->GetNumberOfCells();
      int expected = it->second.Cells;
      if (got != expected)
      {
        std::cout << " got " << got << " cells but expected " << expected;
        std::cout << " FAILED." << std::endl;
        status++;
      }
      else
      {
        std::cout << " # of cells: " << got;
        std::cout << " PASSED." << std::endl;
      }
      std::cout.flush();
    }
  }
  {
    std::cout << "Testing default settings (PolyData)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreatePolyData(10, 20));
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing (TriangleStrips)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateTriangleStripData(10, 20));
    filter->PassThroughCellIdsOff();
    filter->PassThroughPointIdsOff();
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing (PolyData Polygons)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreatePolygonData(9));
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing (UnstructuredGrid, QuadraticWedge, Tetra, PassThroughCellIds, "
                 "PassThroughPointIds)...";
    svtkSmartPointer<svtkAppendFilter> append = svtkSmartPointer<svtkAppendFilter>::New();
    append->AddInputData(CreateTetraData());
    append->AddInputData(CreateQuadraticWedgeData());

    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputConnection(append->GetOutputPort());
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout
      << "Testing (UniformGrid(5,10,1), UseStripsOn, PassThroughCellIds, PassThroughPointIds)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateUniformGrid(5, 10, 1));
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->UseStripsOn();
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::ostringstream fullPrint;
    filter->Print(fullPrint);

    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout
      << "Testing (UniformGrid(1,5,10), UseStripsOn, PassThroughCellIds, PassThroughPointIds)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateUniformGrid(1, 5, 10));
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->UseStripsOn();
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::ostringstream fullPrint;
    filter->Print(fullPrint);

    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout
      << "Testing (UniformGrid(5,1,10), UseStripsOn, PassThroughCellIds, PassThroughPointIds)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateUniformGrid(5, 1, 10));
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->UseStripsOn();
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::ostringstream fullPrint;
    filter->Print(fullPrint);

    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing (UniformGrid, UseStripsOff, PassThroughCellIds, PassThroughPointIds)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateUniformGrid(10, 5, 1));
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->UseStripsOff();
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing DataSetExecute...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();

    svtkSmartPointer<svtkDataSet> ugrid = CreateUniformGrid(10, 5, 1);

    svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
    filter->DataSetExecute(ugrid, polyData);

    int got = polyData->GetNumberOfCells();
    std::cout << " # of cells: " << got;

    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing UniformGridExecute all faces...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();

    svtkSmartPointer<svtkDataSet> ugrid = CreateUniformGrid(10, 5, 1);

    svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(ugrid);
    int* tmpext = grid->GetExtent();
    svtkIdType ext[6];
    ext[0] = tmpext[0];
    ext[1] = tmpext[1];
    ext[2] = tmpext[2];
    ext[3] = tmpext[3];
    ext[4] = tmpext[4];
    ext[5] = tmpext[5];
    bool faces[6] = { true, true, true, true, true, true };
    filter->UniformGridExecute(ugrid, polyData, ext, ext, faces);

    int got = polyData->GetNumberOfCells();
    std::cout << " # of cells: " << got;

    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing UniformGridExecute three faces...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();

    svtkSmartPointer<svtkDataSet> ugrid = CreateUniformGrid(10, 5, 2);

    svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(ugrid);
    int* tmpext = grid->GetExtent();
    svtkIdType ext[6];
    ext[0] = tmpext[0];
    ext[1] = tmpext[1];
    ext[2] = tmpext[2];
    ext[3] = tmpext[3];
    ext[4] = tmpext[4];
    ext[5] = tmpext[5];
    bool faces[6] = { true, false, true, false, true, false };
    filter->UniformGridExecute(ugrid, polyData, ext, ext, faces);

    int got = polyData->GetNumberOfCells();
    std::cout << " # of cells: " << got;

    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing (RectilinearGrid, PassThroughCellIds, PassThroughPointIds)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateRectilinearGrid());
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->Update();

    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << " # of cells: " << got;
    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing (StructuredGrid, PassThroughCellIds, PassThroughPointIds)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateStructuredGrid());
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->Update();
    svtkPolyData* output = filter->GetOutput();
    if (output->GetNumberOfCells() != 10)
    {
      std::cerr << "Incorrect number of cells generated by svtkDataSetSurfaceFilter!\n"
                << "Expected: 10, Found: " << output->GetNumberOfCells();
      return 1;
    }
    else if (output->GetNumberOfPoints() != 32)
    {
      std::cerr << "Incorrect number of points generated by svtkDataSetSurfaceFilter\n"
                << "Expected 32, Found : " << output->GetNumberOfPoints();
      return 1;
    }
    std::cout << " PASSED." << std::endl;
  }
  {
    std::cout << "Testing (StructuredGrid, Blanking, PassThroughCellIds, PassThroughPointIds)...";
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    svtkSmartPointer<svtkDataSet> input = CreateStructuredGrid(true);
    filter->SetInputData(input);
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->Update();
    svtkPolyData* output = filter->GetOutput();
    if (output->GetNumberOfCells() != 6)
    {
      std::cerr << "Incorrect number of cells generated by svtkDataSetSurfaceFilter!\n"
                << "Expected: 6, Found: " << output->GetNumberOfCells();
      return 1;
    }
    else if (output->GetNumberOfPoints() != 24)
    {
      std::cerr << "Incorrect number of points generated by svtkDataSetSurfaceFilter\n"
                << "Expected 24, Found : " << output->GetNumberOfPoints();
      return 1;
    }
    // verify that the blanked point is not present in output.
    double blankPt[3];
    input->GetPoint(6, blankPt);
    for (svtkIdType ptId = 0; ptId < output->GetNumberOfPoints(); ++ptId)
    {
      double x[3];
      output->GetPoint(ptId, x);
      if (svtkMath::Distance2BetweenPoints(blankPt, x) < 1.0e-5)
      {
        std::cerr << "Blanked point included in svtkDataSetSurfaceFilter output!\n"
                  << "ptId: " << ptId << '\n';
        return 1;
      }
    }
    std::cout << " PASSED." << std::endl;
  }
  // Error and warnings
  {
    std::cout << "Testing UniformGridExecute strips not supported error...";
    svtkSmartPointer<svtkTest::ErrorObserver> errorObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();
    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->UseStripsOn();
    filter->AddObserver(svtkCommand::ErrorEvent, errorObserver);
    svtkSmartPointer<svtkDataSet> ugrid = CreateUniformGrid(10, 5, 1);

    svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(ugrid);
    int* tmpext = grid->GetExtent();
    svtkIdType ext[6];
    ext[0] = tmpext[0];
    ext[1] = tmpext[1];
    ext[2] = tmpext[2];
    ext[3] = tmpext[3];
    ext[4] = tmpext[4];
    ext[5] = tmpext[5];
    bool faces[6] = { true, true, true, true, true, true };
    filter->UniformGridExecute(ugrid, polyData, ext, ext, faces);
    int status1 = errorObserver->CheckErrorMessage("Strips are not supported for uniform grid!");
    if (status1)
    {
      std::cout << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing cells == 0 ...";
    svtkSmartPointer<svtkTest::ErrorObserver> warningObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();

    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(svtkSmartPointer<svtkPolyData>::New());
    filter->AddObserver(svtkCommand::WarningEvent, warningObserver);
    filter->Update();

    if (warningObserver->GetError() || warningObserver->GetWarning())
    {
      std::cout << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing DataSetExecute cells == 0 ...";
    svtkSmartPointer<svtkTest::ErrorObserver> warningObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();

    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->AddObserver(svtkCommand::WarningEvent, warningObserver);

    svtkSmartPointer<svtkUnstructuredGrid> ugrid = svtkSmartPointer<svtkUnstructuredGrid>::New();

    svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
    filter->DataSetExecute(ugrid, polyData);
    if (warningObserver->GetError() || warningObserver->GetWarning())
    {
      std::cout << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing StructuredExecute invalid dataset error...";
    svtkSmartPointer<svtkTest::ErrorObserver> errorObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();

    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->AddObserver(svtkCommand::ErrorEvent, errorObserver);

    svtkSmartPointer<svtkUnstructuredGrid> ugrid = svtkSmartPointer<svtkUnstructuredGrid>::New();

    svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
    svtkIdType ext[6];
    ext[0] = 0;
    ext[1] = 1;
    ext[2] = 0;
    ext[3] = 1;
    ext[4] = 0;
    ext[5] = 1;

    filter->StructuredExecute(ugrid, polyData, ext, ext);

    int status1 = errorObserver->CheckErrorMessage("Invalid data set type: 4");
    if (status1)
    {
      std::cout << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing BadAttributes error...";
    svtkSmartPointer<svtkTest::ErrorObserver> errorObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();

    svtkSmartPointer<svtkDataSetSurfaceFilter> filter =
      svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
    filter->SetInputData(CreateBadAttributes());
    filter->PassThroughCellIdsOn();
    filter->PassThroughPointIdsOn();
    filter->GetInput()->AddObserver(svtkCommand::ErrorEvent, errorObserver);
    filter->Update();

    int status1 = errorObserver->CheckErrorMessage(
      "Point array PointDataTestArray with 1 components, only has 2 tuples but there are 3 points");
    if (status1)
    {
      std::cout << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  return status;
}

svtkSmartPointer<svtkDataSet> CreateTriangleStripData(const int xres, const int yres)
{
  svtkSmartPointer<svtkPlaneSource> plane = svtkSmartPointer<svtkPlaneSource>::New();
  plane->SetXResolution(xres);
  plane->SetYResolution(yres);
  plane->Update();

  svtkSmartPointer<svtkTriangleFilter> tris = svtkSmartPointer<svtkTriangleFilter>::New();
  tris->SetInputConnection(plane->GetOutputPort());
  svtkSmartPointer<svtkStripper> stripper = svtkSmartPointer<svtkStripper>::New();
  stripper->SetInputConnection(tris->GetOutputPort());
  stripper->Update();

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(stripper->GetOutput()->GetPoints());
  unstructuredGrid->SetCells(SVTK_TRIANGLE_STRIP, stripper->GetOutput()->GetStrips());
  return unstructuredGrid;
}

svtkSmartPointer<svtkDataSet> CreatePolyData(const int xres, const int yres)
{
  svtkSmartPointer<svtkPlaneSource> plane = svtkSmartPointer<svtkPlaneSource>::New();
  plane->SetXResolution(xres);
  plane->SetYResolution(yres);
  plane->Update();

  svtkSmartPointer<svtkTriangleFilter> tris = svtkSmartPointer<svtkTriangleFilter>::New();
  tris->SetInputConnection(plane->GetOutputPort());
  svtkSmartPointer<svtkStripper> stripper = svtkSmartPointer<svtkStripper>::New();
  stripper->SetInputConnection(tris->GetOutputPort());
  stripper->Update();

  svtkSmartPointer<svtkPolyData> pd = svtkSmartPointer<svtkPolyData>::New();
  pd = plane->GetOutput();
  return pd;
}

svtkSmartPointer<svtkDataSet> CreatePolygonData(int sides)
{
  svtkSmartPointer<svtkRegularPolygonSource> polygon =
    svtkSmartPointer<svtkRegularPolygonSource>::New();
  polygon->SetNumberOfSides(sides);
  polygon->Update();
  svtkSmartPointer<svtkIntArray> cellData = svtkSmartPointer<svtkIntArray>::New();
  cellData->SetNumberOfTuples(polygon->GetOutput()->GetNumberOfCells());
  cellData->SetName("CellDataTestArray");
  svtkIdType c = 0;
  for (svtkIdType i = 0; i < polygon->GetOutput()->GetNumberOfCells(); ++i)
  {
    cellData->SetTuple1(c++, i);
  }
  svtkSmartPointer<svtkIntArray> pointData = svtkSmartPointer<svtkIntArray>::New();
  pointData->SetNumberOfTuples(polygon->GetOutput()->GetNumberOfPoints());
  pointData->SetName("PointDataTestArray");
  c = 0;
  for (int i = 0; i < polygon->GetOutput()->GetNumberOfPoints(); ++i)
  {
    pointData->SetTuple1(c++, i);
  }

  svtkSmartPointer<svtkPolyData> pd = svtkSmartPointer<svtkPolyData>::New();
  pd = polygon->GetOutput();
  pd->GetPointData()->SetScalars(pointData);
  pd->GetCellData()->SetScalars(cellData);

  return pd;
}

svtkSmartPointer<svtkDataSet> CreateTetraData()
{
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(5, 5, 5);
  points->InsertNextPoint(6, 5, 5);
  points->InsertNextPoint(6, 6, 5);
  points->InsertNextPoint(5, 6, 6);

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(points);

  svtkSmartPointer<svtkTetra> tetra = svtkSmartPointer<svtkTetra>::New();
  tetra->GetPointIds()->SetId(0, 4);
  tetra->GetPointIds()->SetId(1, 5);
  tetra->GetPointIds()->SetId(2, 6);
  tetra->GetPointIds()->SetId(3, 7);

  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();
  cellArray->InsertNextCell(tetra);
  unstructuredGrid->SetCells(SVTK_TETRA, cellArray);

  svtkSmartPointer<svtkIntArray> pointData = svtkSmartPointer<svtkIntArray>::New();
  pointData->SetNumberOfTuples(unstructuredGrid->GetNumberOfPoints());
  pointData->SetName("PointDataTestArray");
  int c = 0;
  for (svtkIdType id = 0; id < tetra->GetNumberOfPoints(); ++id)
  {
    pointData->SetTuple1(c++, id);
  }
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  return unstructuredGrid;
}
svtkSmartPointer<svtkDataSet> CreateQuadraticWedgeData()
{
  svtkSmartPointer<svtkQuadraticWedge> aWedge = svtkSmartPointer<svtkQuadraticWedge>::New();
  double* pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(aWedge->GetPoints());

  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();
  cellArray->InsertNextCell(aWedge);
  unstructuredGrid->SetCells(SVTK_QUADRATIC_WEDGE, cellArray);
  return unstructuredGrid;
}
svtkSmartPointer<svtkDataSet> CreateUniformGrid(
  unsigned int dimx, unsigned int dimy, unsigned int dimz)
{
  svtkSmartPointer<svtkUniformGrid> image = svtkSmartPointer<svtkUniformGrid>::New();

  image->SetDimensions(dimx, dimy, dimz);

  image->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);

  for (unsigned int x = 0; x < dimx; x++)
  {
    for (unsigned int y = 0; y < dimy; y++)
    {
      for (unsigned int z = 0; z < dimz; z++)
      {
        unsigned char* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
        if (x < dimx / 2)
        {
          pixel[0] = 50;
        }
        else
        {
          pixel[0] = 150;
        }
      }
    }
  }
  return image;
}

svtkSmartPointer<svtkDataSet> CreateGenericCellData(int cellType)
{
  svtkSmartPointer<svtkGenericCell> aCell = svtkSmartPointer<svtkGenericCell>::New();
  aCell->SetCellType(cellType);
  if (aCell->RequiresInitialization())
  {
    aCell->Initialize();
  }

  int numPts = aCell->GetNumberOfPoints();
  double* pcoords = aCell->GetParametricCoords();
  for (int j = 0; j < numPts; ++j)
  {
    aCell->GetPointIds()->SetId(j, j);
    aCell->GetPoints()->SetPoint(j, pcoords + 3 * j);
  }

  svtkSmartPointer<svtkIntArray> pointData = svtkSmartPointer<svtkIntArray>::New();
  pointData->SetNumberOfTuples(numPts);
  pointData->SetName("PointDataTestArray");
  for (int j = 0; j < numPts; ++j)
  {
    pointData->SetTuple1(j, j);
  }

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(aCell->GetPoints());
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();
  cellArray->InsertNextCell(aCell);
  unstructuredGrid->SetCells(cellType, cellArray);
  return unstructuredGrid;
}

svtkSmartPointer<svtkDataSet> CreateRectilinearGrid()
{
  svtkSmartPointer<svtkRectilinearGrid> grid = svtkSmartPointer<svtkRectilinearGrid>::New();
  grid->SetDimensions(2, 3, 1);

  svtkSmartPointer<svtkDoubleArray> xArray = svtkSmartPointer<svtkDoubleArray>::New();
  xArray->InsertNextValue(0.0);
  xArray->InsertNextValue(2.0);

  svtkSmartPointer<svtkDoubleArray> yArray = svtkSmartPointer<svtkDoubleArray>::New();
  yArray->InsertNextValue(0.0);
  yArray->InsertNextValue(1.0);
  yArray->InsertNextValue(2.0);

  svtkSmartPointer<svtkDoubleArray> zArray = svtkSmartPointer<svtkDoubleArray>::New();
  zArray->InsertNextValue(0.0);

  grid->SetXCoordinates(xArray);
  grid->SetYCoordinates(yArray);
  grid->SetZCoordinates(zArray);

  return grid;
}

// Generate a 1x2x1 svtkStructuredGrid with 12 points.
svtkSmartPointer<svtkDataSet> CreateStructuredGrid(bool blank)
{
  svtkSmartPointer<svtkStructuredGrid> grid = svtkSmartPointer<svtkStructuredGrid>::New();

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  double x, y, z;

  x = 0.0;
  y = 0.0;
  z = 0.0;

  for (unsigned int k = 0; k < 2; k++)
  {
    z += 2.0;
    for (unsigned int j = 0; j < 3; j++)
    {
      y += 1.0;
      for (unsigned int i = 0; i < 2; i++)
      {
        x += .5;
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  // Specify the dimensions of the grid
  grid->SetDimensions(2, 3, 2);
  grid->SetPoints(points);

  // When blanking==true, the 6th point (0th cell) is blanked.
  if (blank)
  {
    grid->BlankPoint(points->GetNumberOfPoints() / 2);
  }
  return grid;
}

svtkSmartPointer<svtkDataSet> CreateBadAttributes()
{
  svtkSmartPointer<svtkPolyLine> aPolyLine = svtkSmartPointer<svtkPolyLine>::New();
  aPolyLine->GetPointIds()->SetNumberOfIds(3);
  aPolyLine->GetPointIds()->SetId(0, 0);
  aPolyLine->GetPointIds()->SetId(1, 1);
  aPolyLine->GetPointIds()->SetId(2, 2);

  aPolyLine->GetPoints()->SetNumberOfPoints(3);
  aPolyLine->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(1, 10.0, 30.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(2, 10.0, 30.0, 40.0);

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(aPolyLine->GetPoints());

  svtkSmartPointer<svtkIntArray> pointData = svtkSmartPointer<svtkIntArray>::New();
  pointData->SetNumberOfTuples(2);
  pointData->SetName("PointDataTestArray");
  for (int j = 0; j < 2; ++j)
  {
    pointData->SetTuple1(j, j);
  }

  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();
  cellArray->InsertNextCell(aPolyLine);
  unstructuredGrid->SetCells(SVTK_POLY_LINE, cellArray);
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  return unstructuredGrid;
}
