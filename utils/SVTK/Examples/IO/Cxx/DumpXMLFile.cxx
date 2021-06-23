//
// DumpXMLFile - report on the contents of an XML or legacy svtk file
//  Usage: DumpXMLFile XMLFile1 XMLFile2 ...
//         where
//         XMLFile is a svtk XML file of type .vtu, .vtp, .vts, .vtr,
//         .vti
//
#include <svtkCellData.h>
#include <svtkCellTypes.h>
#include <svtkDataSet.h>
#include <svtkDataSetReader.h>
#include <svtkFieldData.h>
#include <svtkImageData.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkRectilinearGrid.h>
#include <svtkSmartPointer.h>
#include <svtkStructuredGrid.h>
#include <svtkUnstructuredGrid.h>
#include <svtkXMLCompositeDataReader.h>
#include <svtkXMLImageDataReader.h>
#include <svtkXMLPolyDataReader.h>
#include <svtkXMLReader.h>
#include <svtkXMLRectilinearGridReader.h>
#include <svtkXMLStructuredGridReader.h>
#include <svtkXMLUnstructuredGridReader.h>
#include <svtksys/SystemTools.hxx>

#include <map>

template <class TReader>
svtkDataSet* ReadAnXMLFile(const char* fileName)
{
  svtkSmartPointer<TReader> reader = svtkSmartPointer<TReader>::New();
  reader->SetFileName(fileName);
  reader->Update();
  reader->GetOutput()->Register(reader);
  return svtkDataSet::SafeDownCast(reader->GetOutput());
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " XMLFile1 XMLFile2 ..." << std::endl;
  }

  // Process each file on the command line
  int f = 1;
  while (f < argc)
  {
    svtkDataSet* dataSet;
    std::string extension = svtksys::SystemTools::GetFilenameLastExtension(argv[f]);
    // Dispatch based on the file extension
    if (extension == ".vtu")
    {
      dataSet = ReadAnXMLFile<svtkXMLUnstructuredGridReader>(argv[f]);
    }
    else if (extension == ".vtp")
    {
      dataSet = ReadAnXMLFile<svtkXMLPolyDataReader>(argv[f]);
    }
    else if (extension == ".vts")
    {
      dataSet = ReadAnXMLFile<svtkXMLStructuredGridReader>(argv[f]);
    }
    else if (extension == ".vtr")
    {
      dataSet = ReadAnXMLFile<svtkXMLRectilinearGridReader>(argv[f]);
    }
    else if (extension == ".vti")
    {
      dataSet = ReadAnXMLFile<svtkXMLImageDataReader>(argv[f]);
    }
    else if (extension == ".svtk")
    {
      dataSet = ReadAnXMLFile<svtkDataSetReader>(argv[f]);
    }
    else
    {
      std::cerr << argv[0] << " Unknown extension: " << extension << std::endl;
      return EXIT_FAILURE;
    }

    int numberOfCells = dataSet->GetNumberOfCells();
    int numberOfPoints = dataSet->GetNumberOfPoints();

    // Generate a report
    std::cout << "------------------------" << std::endl;
    std::cout << argv[f] << std::endl
              << " contains a " << std::endl
              << dataSet->GetClassName() << " that has " << numberOfCells << " cells"
              << " and " << numberOfPoints << " points." << std::endl;
    typedef std::map<int, int> CellContainer;
    CellContainer cellMap;
    for (int i = 0; i < numberOfCells; i++)
    {
      cellMap[dataSet->GetCellType(i)]++;
    }

    CellContainer::const_iterator it = cellMap.begin();
    while (it != cellMap.end())
    {
      std::cout << "\tCell type " << svtkCellTypes::GetClassNameFromTypeId(it->first) << " occurs "
                << it->second << " times." << std::endl;
      ++it;
    }

    // Now check for point data
    svtkPointData* pd = dataSet->GetPointData();
    if (pd)
    {
      std::cout << " contains point data with " << pd->GetNumberOfArrays() << " arrays."
                << std::endl;
      for (int i = 0; i < pd->GetNumberOfArrays(); i++)
      {
        std::cout << "\tArray " << i << " is named "
                  << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL") << std::endl;
      }
    }
    // Now check for cell data
    svtkCellData* cd = dataSet->GetCellData();
    if (cd)
    {
      std::cout << " contains cell data with " << cd->GetNumberOfArrays() << " arrays."
                << std::endl;
      for (int i = 0; i < cd->GetNumberOfArrays(); i++)
      {
        std::cout << "\tArray " << i << " is named "
                  << (cd->GetArrayName(i) ? cd->GetArrayName(i) : "NULL") << std::endl;
      }
    }
    // Now check for field data
    if (dataSet->GetFieldData())
    {
      std::cout << " contains field data with " << dataSet->GetFieldData()->GetNumberOfArrays()
                << " arrays." << std::endl;
      for (int i = 0; i < dataSet->GetFieldData()->GetNumberOfArrays(); i++)
      {
        std::cout << "\tArray " << i << " is named "
                  << dataSet->GetFieldData()->GetArray(i)->GetName() << std::endl;
      }
    }
    dataSet->Delete();
    f++;
  }
  return EXIT_SUCCESS;
}
