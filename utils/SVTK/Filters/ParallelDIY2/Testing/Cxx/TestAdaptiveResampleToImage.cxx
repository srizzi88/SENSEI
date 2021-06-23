#include "svtkAdaptiveResampleToImage.h"
#include "svtkBoundingBox.h"
#include "svtkClipDataSet.h"
#include "svtkDataSet.h"
#include "svtkLogger.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPartitionedDataSet.h"
#include "svtkRTAnalyticSource.h"

#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
#include "svtkMPIController.h"
#else
#include "svtkDummyController.h"
#endif

#include <numeric>
#include <vector>

namespace
{
bool ValidateDataset(svtkPartitionedDataSet* pds, svtkMultiProcessController* controller,
  int numLeaves, const svtkBoundingBox& gbox)
{
  numLeaves = svtkMath::NearestPowerOfTwo(numLeaves);

  const int numParts = static_cast<int>(pds->GetNumberOfPartitions());
  int allParts = numParts;
  controller->AllReduce(&numParts, &allParts, 1, svtkCommunicator::SUM_OP);

  if (allParts != numLeaves)
  {
    svtkLogF(ERROR, "Error: mismatched leaves. expected: %d, got %d", numLeaves, allParts);
    return false;
  }

  // validate all boxes same as the input dataset box.
  double bds[6];
  svtkMath::UninitializeBounds(bds);
  pds->GetBounds(bds);

  svtkBoundingBox bbox, allbbox;
  bbox.SetBounds(bds);
  controller->AllReduce(bbox, allbbox);

  if (allbbox != gbox)
  {
    svtkLogF(ERROR, "Error: mismatched bounds!");
    return false;
  }

  // validate no bounding boxes overlap.
  std::vector<int> parts(controller->GetNumberOfProcesses());
  parts[controller->GetLocalProcessId()] = numParts;

  controller->AllGather(&numParts, &parts[0], 1);

  std::vector<double> local_boxes(6 * numParts);
  for (int cc = 0; cc < numParts; ++cc)
  {
    svtkDataSet::SafeDownCast(pds->GetPartition(cc))->GetBounds(&local_boxes[6 * cc]);
  }
  std::vector<double> boxes(6 * std::accumulate(parts.begin(), parts.end(), 0));

  std::vector<svtkIdType> recvLengths(controller->GetNumberOfProcesses());
  std::vector<svtkIdType> offsets(controller->GetNumberOfProcesses());
  controller->AllGatherV(&local_boxes[0], &boxes[0], static_cast<svtkIdType>(local_boxes.size()),
    &recvLengths[0], &offsets[0]);
  if (controller->GetNumberOfProcesses() == 1)
  {
    boxes = local_boxes;
  }

  for (size_t i = 0; i < (boxes.size()) / 6; ++i)
  {
    const svtkBoundingBox boxA(&boxes[6 * i]);
    for (size_t j = i + 1; j < (boxes.size()) / 6; ++j)
    {
      const svtkBoundingBox boxB(&boxes[6 * j]);
      int overlap = 0;
      for (int dim = 0; dim < 3; ++dim)
      {
        if (boxB.GetMinPoint()[dim] > boxA.GetMinPoint()[dim] &&
          boxB.GetMinPoint()[dim] < boxA.GetMaxPoint()[dim])
        {
          overlap++;
        }
        else if (boxB.GetMaxPoint()[dim] > boxA.GetMinPoint()[dim] &&
          boxB.GetMaxPoint()[dim] < boxA.GetMaxPoint()[dim])
        {
          overlap++;
        }
      }
      if (overlap == 3)
      {
        svtkLogF(ERROR, "Error: boxes overlap!");
        abort();
      }
    }
  }

  return true;
}
}

int TestAdaptiveResampleToImage(int argc, char* argv[])
{
#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
  svtkMPIController* contr = svtkMPIController::New();
#else
  svtkDummyController* contr = svtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  svtkMultiProcessController::SetGlobalController(contr);

  int status = EXIT_SUCCESS;

  // Create Pipeline
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 63, 0, 63, 0, 63);
  wavelet->SetCenter(16, 16, 16);

  svtkNew<svtkClipDataSet> clip;
  clip->SetInputConnection(wavelet->GetOutputPort());
  clip->SetValue(157);

  svtkNew<svtkAdaptiveResampleToImage> resampler;
  resampler->SetNumberOfImages(4);
  resampler->SetInputConnection(clip->GetOutputPort());
  resampler->SetSamplingDimensions(8, 8, 8);
  resampler->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  double bds[6];
  svtkDataSet::SafeDownCast(clip->GetOutputDataObject(0))->GetBounds(bds);
  svtkBoundingBox bbox(bds), allbbox;
  contr->AllReduce(bbox, allbbox);

  if (!ValidateDataset(
        svtkPartitionedDataSet::SafeDownCast(resampler->GetOutputDataObject(0)), contr, 4, allbbox))
  {
    status = EXIT_FAILURE;
  }

  resampler->SetNumberOfImages(6);
  resampler->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);
  if (!ValidateDataset(
        svtkPartitionedDataSet::SafeDownCast(resampler->GetOutputDataObject(0)), contr, 6, allbbox))
  {
    status = EXIT_FAILURE;
  }

  resampler->SetNumberOfImages(3);
  resampler->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);
  if (!ValidateDataset(
        svtkPartitionedDataSet::SafeDownCast(resampler->GetOutputDataObject(0)), contr, 3, allbbox))
  {
    status = EXIT_FAILURE;
  }

  svtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return status;
}
