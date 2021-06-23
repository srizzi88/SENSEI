#include <svtkImageData.h>
#include <svtkInformation.h>
#include <svtkLogger.h>
#include <svtkNew.h>
#include <svtkOMETIFFReader.h>
#include <svtkPointData.h>
#include <svtkStreamingDemandDrivenPipeline.h>
#include <svtkVector.h>
#include <svtkVectorOperators.h>
#include <svtksys/RegularExpression.hxx>

#include <cstdlib>

int TestOMETIFFReader(int argc, char* argv[])
{
  std::string data;
  svtkVector3i size(0);
  svtkVector3d physicalSize(0.0);
  int sizeC = 0;
  int sizeT = 0;

  svtksys::RegularExpression regex("^([^x]+)x([^x]+)x([^x]+)$");
  for (int cc = 1; (cc + 1) < argc; cc++)
  {
    if (strcmp(argv[cc], "--data") == 0)
    {
      data = argv[++cc];
    }
    else if (strcmp(argv[cc], "--size") == 0)
    {
      if (regex.find(argv[++cc]))
      {
        size[0] = std::atoi(regex.match(1).c_str());
        size[1] = std::atoi(regex.match(2).c_str());
        size[2] = std::atoi(regex.match(3).c_str());
      }
    }
    else if (strcmp(argv[cc], "--physical-size") == 0)
    {
      if (regex.find(argv[++cc]))
      {
        physicalSize[0] = std::atof(regex.match(1).c_str());
        physicalSize[1] = std::atof(regex.match(2).c_str());
        physicalSize[2] = std::atof(regex.match(3).c_str());
      }
    }
    else if (strcmp(argv[cc], "--size_c") == 0)
    {
      sizeC = std::atoi(argv[++cc]);
    }
    else if (strcmp(argv[cc], "--size_t") == 0)
    {
      sizeT = std::atoi(argv[++cc]);
    }
  }

  svtkNew<svtkOMETIFFReader> reader;
  reader->SetFileName(data.c_str());
  reader->UpdateInformation();

  auto outInfo = reader->GetOutputInformation(0);
  if (sizeT >= 1 && outInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()) &&
    outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS()) == sizeT)
  {
    // verified timesteps.
  }
  else
  {
    svtkLogF(ERROR, "Failed to read timesteps; expected (%d), got (%d)", sizeT,
      outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS()));
  }

  reader->Update();
  auto img = reader->GetOutput();
  if (img->GetPointData()->GetNumberOfArrays() != sizeC)
  {
    svtkLogF(ERROR, "Failed to read channels; expected (%d), got (%d)", sizeC,
      img->GetPointData()->GetNumberOfArrays());
  }

  svtkVector3i dims;
  img->GetDimensions(dims.GetData());
  if (dims != size)
  {
    svtkLogF(ERROR, "Failed due to size mismatch; expected (%d, %d, %d), got (%d, %d, %d)", size[0],
      size[1], size[2], dims[0], dims[1], dims[2]);
  }

  svtkVector3d spacing;
  img->GetSpacing(spacing.GetData());
  if ((spacing - physicalSize).Norm() > 0.00001)
  {
    svtkLogF(ERROR, "Physical size / spacing mismatch; expected (%f, %f, %f), got (%f, %f, %f)",
      physicalSize[0], physicalSize[1], physicalSize[2], spacing[0], spacing[1], spacing[2]);
  }

  // now read in multiple pieces.
  for (int cc = 0; cc < 4; ++cc)
  {
    reader->Modified();
    reader->UpdatePiece(cc, 4, 0);
  }

  return EXIT_SUCCESS;
}
