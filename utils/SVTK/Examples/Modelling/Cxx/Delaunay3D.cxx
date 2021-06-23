//
// Delaunay3D
// Usage: Delaunay3D InputFile(.vtp) OutputFile(.vtu)
//        where
//        InputFile is an XML PolyData file with extension .vtp
//        OutputFile is an XML Unstructured Grid file with extension .vtu
//
#include <svtkCleanPolyData.h>
#include <svtkDelaunay3D.h>
#include <svtkSmartPointer.h>
#include <svtkXMLDataSetWriter.h>
#include <svtkXMLPolyDataReader.h>

int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    cout << "Usage: " << argv[0] << " InputPolyDataFile OutputDataSetFile" << endl;
    return EXIT_FAILURE;
  }

  // Read the file
  svtkSmartPointer<svtkXMLPolyDataReader> reader = svtkSmartPointer<svtkXMLPolyDataReader>::New();
  reader->SetFileName(argv[1]);

  // Clean the polydata. This will remove duplicate points that may be
  // present in the input data.
  svtkSmartPointer<svtkCleanPolyData> cleaner = svtkSmartPointer<svtkCleanPolyData>::New();
  cleaner->SetInputConnection(reader->GetOutputPort());

  // Generate a tetrahedral mesh from the input points. By
  // default, the generated volume is the convex hull of the points.
  svtkSmartPointer<svtkDelaunay3D> delaunay3D = svtkSmartPointer<svtkDelaunay3D>::New();
  delaunay3D->SetInputConnection(cleaner->GetOutputPort());

  // Write the mesh as an unstructured grid
  svtkSmartPointer<svtkXMLDataSetWriter> writer = svtkSmartPointer<svtkXMLDataSetWriter>::New();
  writer->SetFileName(argv[2]);
  writer->SetInputConnection(delaunay3D->GetOutputPort());
  writer->Write();

  return EXIT_SUCCESS;
}
