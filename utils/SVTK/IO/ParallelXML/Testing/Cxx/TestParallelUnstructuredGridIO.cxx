#include "svtkNew.h"
#include <string>

#include "svtksys/FStream.hxx"
#include <svtkCell.h>
#include <svtkIdList.h>
#include <svtkMultiProcessController.h>
#include <svtkPoints.h>
#include <svtkTestUtilities.h>
#include <svtkUnstructuredGrid.h>
#include <svtkXMLPUnstructuredGridReader.h>
#include <svtkXMLPUnstructuredGridWriter.h>
#include <svtkXMLUnstructuredGridReader.h>

using namespace std;

bool CompareGrids(svtkUnstructuredGrid* s, svtkUnstructuredGrid* t)
{
  if (s->GetNumberOfCells() != t->GetNumberOfCells())
  {
    cerr << "The number of cells does not match: " << s->GetNumberOfCells()
         << " != " << t->GetNumberOfCells() << endl;
    return false;
  }
  for (svtkIdType i = 0; i < s->GetNumberOfCells(); ++i)
  {
    if (s->GetCellType(i) != t->GetCellType(i))
    {
      cerr << "The cell type does not match: " << s->GetCellType(i) << " != " << t->GetCellType(i)
           << endl;
      return false;
    }
    svtkNew<svtkIdList> sIds, tIds;
    if (s->GetCellType(i) == SVTK_POLYHEDRON)
    {
      s->GetFaceStream(i, sIds.GetPointer());
      t->GetFaceStream(i, tIds.GetPointer());
    }
    else
    {
      s->GetCellPoints(i, sIds.GetPointer());
      t->GetCellPoints(i, tIds.GetPointer());
    }
    if (sIds->GetNumberOfIds() != tIds->GetNumberOfIds())
    {
      cerr << "Cell type : " << s->GetCellType(i) << endl;
      cerr << "The number of ids does not match: " << sIds->GetNumberOfIds()
           << " != " << tIds->GetNumberOfIds() << endl;
      return false;
    }

    for (svtkIdType j = 0; j < sIds->GetNumberOfIds(); ++j)
    {
      svtkIdType sId = sIds->GetId(j);
      svtkIdType tId = tIds->GetId(j);

      if (sId != tId)
      {
        cerr << "Cell type : " << s->GetCellType(i) << endl;
        cerr << "The id at position " << j << " does not match: " << sId << " != " << tId << endl;
        return false;
      }
    }
  }

  return true;
}

int TestParallelUnstructuredGridIO(int argc, char* argv[])
{
  svtkNew<svtkPoints> points;

  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 0);

  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(1, 0, 1);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(0, 1, 1);

  points->InsertNextPoint(.5, .5, 2);
  points->InsertNextPoint(.5, .5, -1);

  svtkNew<svtkUnstructuredGrid> ug;
  ug->SetPoints(points);

  ug->Allocate(3); // allocate for 3 cells

  svtkNew<svtkIdList> ids;

  // add a hexahedron of the first 8 points (i.e. a cube)
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ug->InsertNextCell(SVTK_HEXAHEDRON, ids.GetPointer());
  ids->Reset();

  // add a polyhedron comprise of the top hexahedron face
  // and four triangles to the 9th point
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ids->InsertNextId(8);

  svtkNew<svtkIdList> faces;
  // top face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(7);

  // four triangle side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(6);
  faces->InsertNextId(7);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(7);
  faces->InsertNextId(4);
  faces->InsertNextId(8);

  // insert the polyhedron cell
  ug->InsertNextCell(
    SVTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  // put another pyramid on the bottom towards the 10th point
  faces->Reset();
  ids->Reset();

  // the list of points that the pyramid references
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(9);

  // bottom face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(3);

  // four side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(2);
  faces->InsertNextId(3);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(9);

  // insert the cell. We now have two pyramids with a cube in between
  ug->InsertNextCell(
    SVTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  svtkMultiProcessController* ctrl = svtkMultiProcessController::GetGlobalController();
  svtkNew<svtkXMLPUnstructuredGridWriter> w;
  w->SetController(ctrl);
  w->SetInputData(ug);
  w->SetUseSubdirectory(true);
  char* tempDir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  std::string dir(tempDir);
  std::string fn = dir + "/ug.pvtu";
  w->SetFileName(fn.c_str());
  w->SetDataModeToAscii();
  w->Update();
  delete[] tempDir;
  svtksys::ifstream f(fn.c_str());
  if (!f.good())
  {
    std::cerr << "File " << fn << " does not exist." << std::endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkXMLUnstructuredGridReader> r;
  std::string piece = dir + "/ug/ug_0.vtu";
  r->SetFileName(piece.c_str());
  r->Update();

  // first try reading the piece with a non-parallel reader
  svtkUnstructuredGrid* read = r->GetOutput();
  cout << "Comparing original with .vtu" << endl;
  if (!CompareGrids(ug.GetPointer(), read))
    return EXIT_FAILURE;

  // now read the .pvtu file with the paralle reader
  svtkNew<svtkXMLPUnstructuredGridReader> pr;
  pr->SetFileName(fn.c_str());
  // this will give a SIGSEGV on svtkXMLPUnstructuredGridReader::ReadPieceData()
  pr->Update();

  read = pr->GetOutput();
  cout << "Comparing original with .pvtu" << endl;
  if (!CompareGrids(ug.GetPointer(), read))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
