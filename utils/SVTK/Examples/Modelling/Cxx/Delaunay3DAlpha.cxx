#include <svtkCleanPolyData.h>
#include <svtkDelaunay3D.h>
#include <svtkSmartPointer.h>
#include <svtkXMLDataSetWriter.h>
#include <svtkXMLPolyDataReader.h>

int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    cout << "Usage: " << argv[0] << " Alpha InputPolyDataFile OutputDataSetFile" << endl;
    return EXIT_FAILURE;
  }

  // Read a file
  svtkSmartPointer<svtkXMLPolyDataReader> reader = svtkSmartPointer<svtkXMLPolyDataReader>::New();
  reader->SetFileName(argv[2]);

  // Clean the polydata. This will remove duplicate points that may be
  // present in the input data.
  svtkSmartPointer<svtkCleanPolyData> cleaner = svtkSmartPointer<svtkCleanPolyData>::New();
  cleaner->SetInputConnection(reader->GetOutputPort());

  // Generate a mesh from the input points. If Alpha is non-zero, then
  // tetrahedra, triangles, edges and vertices that lie within the
  // alpha radius are output.
  svtkSmartPointer<svtkDelaunay3D> delaunay3D = svtkSmartPointer<svtkDelaunay3D>::New();
  delaunay3D->SetInputConnection(cleaner->GetOutputPort());
  delaunay3D->SetAlpha(atof(argv[1]));

  // Output the mesh
  svtkSmartPointer<svtkXMLDataSetWriter> writer = svtkSmartPointer<svtkXMLDataSetWriter>::New();
  writer->SetFileName(argv[3]);
  writer->SetInputConnection(delaunay3D->GetOutputPort());
  writer->Write();

  return EXIT_SUCCESS;
}
