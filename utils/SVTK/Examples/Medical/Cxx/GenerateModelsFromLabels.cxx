//
// GenerateModelsFromLabels
//   Usage: GenerateModelsFromLabels InputVolume Startlabel Endlabel
//          where
//          InputVolume is a meta file containing a 3 volume of
//            discrete labels.
//          StartLabel is the first label to be processed
//          EndLabel is the last label to be processed
//          NOTE: There can be gaps in the labeling. If a label does
//          not exist in the volume, it will be skipped.
//
//
#include <svtkDiscreteMarchingCubes.h>
#include <svtkGeometryFilter.h>
#include <svtkImageAccumulate.h>
#include <svtkMaskFields.h>
#include <svtkMetaImageReader.h>
#include <svtkSmartPointer.h>
#include <svtkThreshold.h>
#include <svtkWindowedSincPolyDataFilter.h>
#include <svtkXMLPolyDataWriter.h>

#include <sstream>
#include <svtkImageData.h>
#include <svtkPointData.h>
#include <svtkUnstructuredGrid.h>

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    cout << "Usage: " << argv[0] << " InputVolume StartLabel EndLabel" << endl;
    return EXIT_FAILURE;
  }

  // Create all of the classes we will need
  svtkSmartPointer<svtkMetaImageReader> reader = svtkSmartPointer<svtkMetaImageReader>::New();
  svtkSmartPointer<svtkImageAccumulate> histogram = svtkSmartPointer<svtkImageAccumulate>::New();
  svtkSmartPointer<svtkDiscreteMarchingCubes> discreteCubes =
    svtkSmartPointer<svtkDiscreteMarchingCubes>::New();
  svtkSmartPointer<svtkWindowedSincPolyDataFilter> smoother =
    svtkSmartPointer<svtkWindowedSincPolyDataFilter>::New();
  svtkSmartPointer<svtkThreshold> selector = svtkSmartPointer<svtkThreshold>::New();
  svtkSmartPointer<svtkMaskFields> scalarsOff = svtkSmartPointer<svtkMaskFields>::New();
  svtkSmartPointer<svtkGeometryFilter> geometry = svtkSmartPointer<svtkGeometryFilter>::New();
  svtkSmartPointer<svtkXMLPolyDataWriter> writer = svtkSmartPointer<svtkXMLPolyDataWriter>::New();

  // Define all of the variables
  unsigned int startLabel = atoi(argv[2]);
  if (startLabel > SVTK_SHORT_MAX)
  {
    std::cout << "ERROR: startLabel is larger than " << SVTK_SHORT_MAX << std::endl;
    return EXIT_FAILURE;
  }
  unsigned int endLabel = atoi(argv[3]);
  if (endLabel > SVTK_SHORT_MAX)
  {
    std::cout << "ERROR: endLabel is larger than " << SVTK_SHORT_MAX << std::endl;
    return EXIT_FAILURE;
  }
  std::string filePrefix = "Label";
  unsigned int smoothingIterations = 15;
  double passBand = 0.001;
  double featureAngle = 120.0;

  // Generate models from labels
  // 1) Read the meta file
  // 2) Generate a histogram of the labels
  // 3) Generate models from the labeled volume
  // 4) Smooth the models
  // 5) Output each model into a separate file

  reader->SetFileName(argv[1]);

  histogram->SetInputConnection(reader->GetOutputPort());
  histogram->SetComponentExtent(0, endLabel, 0, 0, 0, 0);
  histogram->SetComponentOrigin(0, 0, 0);
  histogram->SetComponentSpacing(1, 1, 1);
  histogram->Update();

  discreteCubes->SetInputConnection(reader->GetOutputPort());
  discreteCubes->GenerateValues(endLabel - startLabel + 1, startLabel, endLabel);

  smoother->SetInputConnection(discreteCubes->GetOutputPort());
  smoother->SetNumberOfIterations(smoothingIterations);
  smoother->BoundarySmoothingOff();
  smoother->FeatureEdgeSmoothingOff();
  smoother->SetFeatureAngle(featureAngle);
  smoother->SetPassBand(passBand);
  smoother->NonManifoldSmoothingOn();
  smoother->NormalizeCoordinatesOn();
  smoother->Update();

  selector->SetInputConnection(smoother->GetOutputPort());
  selector->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);

  // Strip the scalars from the output
  scalarsOff->SetInputConnection(selector->GetOutputPort());
  scalarsOff->CopyAttributeOff(svtkMaskFields::POINT_DATA, svtkDataSetAttributes::SCALARS);
  scalarsOff->CopyAttributeOff(svtkMaskFields::CELL_DATA, svtkDataSetAttributes::SCALARS);

  geometry->SetInputConnection(scalarsOff->GetOutputPort());

  writer->SetInputConnection(geometry->GetOutputPort());

  for (unsigned int i = startLabel; i <= endLabel; i++)
  {
    // see if the label exists, if not skip it
    double frequency = histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(i);
    if (frequency == 0.0)
    {
      continue;
    }

    // select the cells for a given label
    selector->ThresholdBetween(i, i);

    // output the polydata
    std::stringstream ss;
    ss << filePrefix << i << ".vtp";
    cout << argv[0] << " writing " << ss.str() << endl;

    writer->SetFileName(ss.str().c_str());
    writer->Write();
  }
  return EXIT_SUCCESS;
}
