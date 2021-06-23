/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: svtkDistributedPointCloudFilter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDistributedPointCloudFilter.h"

#include "svtkCharArray.h"
#include "svtkDataArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMPIController.h"
#include "svtkMPIUtilities.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOctreePointLocator.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"

#include <algorithm>

// Histogram precision to divide space in two
static const int HISTOGRAM_SIZE = 1024;

svtkStandardNewMacro(svtkDistributedPointCloudFilter);
svtkSetObjectImplementationMacro(
  svtkDistributedPointCloudFilter, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkDistributedPointCloudFilter::svtkDistributedPointCloudFilter()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkDistributedPointCloudFilter::~svtkDistributedPointCloudFilter()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkDistributedPointCloudFilter::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDistributedPointCloudFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkPointSet* input = svtkPointSet::GetData(inputVector[0]);
  if (!input)
  {
    svtkErrorMacro("No valid input!");
    return 0;
  }

  svtkPolyData* output = svtkPolyData::GetData(outputVector);
  if (!output)
  {
    svtkErrorMacro("No output object!");
    return 0;
  }

  svtkMPIController* controller = svtkMPIController::SafeDownCast(this->Controller);
  if (!controller)
  {
    // No MPI controller? Just pass points & point data
    output->SetPoints(input->GetPoints());
    output->GetPointData()->ShallowCopy(input->GetPointData());
    return 1;
  }

  std::vector<svtkMPIController*> subControllersTree;
  if (this->InitializeKdTree(subControllersTree))
  {
    double bounds[6];
    this->OptimizeBoundingBox(subControllersTree, input, bounds);
    this->GetPointsInsideBounds(controller, input, output, bounds);
  }
  else
  {
    svtkErrorMacro("Sub-communicators are not correctly initialized, no distribution performed");
    return 0;
  }
  // Destroy the kdtree controllers
  for (auto* c : subControllersTree)
  {
    c->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
bool svtkDistributedPointCloudFilter::InitializeKdTree(std::vector<svtkMPIController*>& kdTreeRounds)
{
  this->Controller->Register(this);

  kdTreeRounds.push_back(svtkMPIController::SafeDownCast(this->Controller));

  int index = 0;
  while (kdTreeRounds[index]->GetNumberOfProcesses() > 2)
  {
    svtkMPIController* ctrl = kdTreeRounds[index];
    int roundRank = ctrl->GetLocalProcessId();
    int split = ctrl->GetNumberOfProcesses() / 2;

    int color = (roundRank < split) ? 0 : 1;
    int key = roundRank - ((roundRank < split) ? 0 : split);

    svtkMPIController* subCtrl = ctrl->PartitionController(color, key);
    if (subCtrl == nullptr)
    {
      break;
    }
    kdTreeRounds.push_back(subCtrl);
    ++index;
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkDistributedPointCloudFilter::OptimizeBoundingBox(
  std::vector<svtkMPIController*>& kdTreeRounds, svtkPointSet* pointCloud, double regionBounds[6])
{
  if (kdTreeRounds.empty())
  {
    return false;
  }

  // ****************************************
  // Compute the domains bounds

  // Separate lower and upper because Allreduce should minimize lower and maximize upper
  double localLowerBound[3];
  double localUpperBound[3];
  double currentGroupLowerBound[3];
  double currentGroupUpperBound[3];

  svtkIdType numPts = pointCloud->GetNumberOfPoints();
  if (numPts > 0)
  {
    double* bounds = pointCloud->GetBounds();
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = bounds[2 * i + 0];
      localUpperBound[i] = bounds[2 * i + 1];
    }
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = +std::numeric_limits<double>::max();
      localUpperBound[i] = -std::numeric_limits<double>::max();
    }
  }

  // ****************************************
  // Main Loop: transfer points between process.
  // Point cloud is recursively splitted in two, among MPI groups.
  // Algorithm:
  // - 1. choose an axis (the longest)
  // - 2. build the local histogram of number of points along the given axis
  // - 3. compute the global histogram
  // - 4. process 0 only: find the median and the corresponding cut position and share it to others
  // - 5. split points in two groups : to keep on this node or to send
  // - 6. exchange points: each MPI process find a partner in the other MPI group.
  // - 7. update current MPI group bounds
  // Each round concerns a MPI subgroup of previous round group

  const int histSize = HISTOGRAM_SIZE;
  std::vector<int> histogram(histSize, 0);
  std::vector<int> histsum(histSize, 0);
  std::vector<svtkIdType> pointExchangeCount(kdTreeRounds[0]->GetNumberOfProcesses(), 0);
  std::vector<double> pts;

  if (numPts > 0)
  {
    pts.resize(3 * numPts);
    for (svtkIdType i = 0; i < numPts; i++)
    {
      pointCloud->GetPoint(i, &pts[i * 3]);
    }
  }

  for (size_t round = 0; round < kdTreeRounds.size(); round++)
  {
    svtkMPIController* kdTreeRound = kdTreeRounds[round];
    svtkMPICommunicator* roundComm =
      svtkMPICommunicator::SafeDownCast(kdTreeRound->GetCommunicator());
    int roundNumRanks = kdTreeRound->GetNumberOfProcesses();
    int roundRank = kdTreeRound->GetLocalProcessId();

    if (roundNumRanks == 1)
    {
      continue;
    }

    roundComm->AllReduceVoidArray(
      localLowerBound, currentGroupLowerBound, 3, SVTK_DOUBLE, svtkCommunicator::MIN_OP);
    roundComm->AllReduceVoidArray(
      localUpperBound, currentGroupUpperBound, 3, SVTK_DOUBLE, svtkCommunicator::MAX_OP);

    // ----------------------------------------
    // 1. choose a cut axis
    int cutaxis = 0;
    double length = currentGroupUpperBound[0] - currentGroupLowerBound[0];
    if (length < (currentGroupUpperBound[1] - currentGroupLowerBound[1]))
    {
      cutaxis = 1;
      length = currentGroupUpperBound[1] - currentGroupLowerBound[1];
    }
    if (length < (currentGroupUpperBound[2] - currentGroupLowerBound[2]))
    {
      cutaxis = 2;
      length = currentGroupUpperBound[2] - currentGroupLowerBound[2];
    }

    // ----------------------------------------
    // 2. build local histogram along chosen axis
    double offset = -currentGroupLowerBound[cutaxis];

    for (svtkIdType i = 0; i < numPts; i++)
    {
      int sampledPosition = static_cast<int>((pts[3 * i + cutaxis] + offset) / length * histSize);
      sampledPosition = std::max(std::min(sampledPosition, histSize - 1), 0);
      ++histogram[sampledPosition];
    }

    // ----------------------------------------
    // 3. reduction across round participants to get global histogram
    roundComm->ReduceVoidArray(
      histogram.data(), histsum.data(), histSize, SVTK_INT, svtkCommunicator::SUM_OP, 0);

    svtkIdType totalNumpts = numPts;
    roundComm->ReduceVoidArray(&numPts, &totalNumpts, 1, SVTK_ID_TYPE, svtkCommunicator::SUM_OP, 0);

    // ----------------------------------------
    // 4. process 0 of sub group computes cut position and broadcast it to others
    int cutpos = 0;
    if (roundRank == 0)
    {
      // ratio of left part over total. it won't be 2 for un-even number of participant processors
      double ratio = static_cast<double>(roundNumRanks) / static_cast<double>(roundNumRanks / 2);
      int i = 1;
      for (; i < histSize; i++)
      {
        histsum[i] += histsum[i - 1];
        if (histsum[i] >= (totalNumpts / ratio))
        {
          break;
        }
      }
      cutpos = i;
    }
    roundComm->BroadcastVoidArray(&cutpos, 1, SVTK_INT, 0);

    // ----------------------------------------
    // 5. classify points
    // which half of the group i belongs to. true = left, false = right
    bool side = roundRank < (roundNumRanks / 2);
    std::vector<double> partnerPts(3 * numPts);
    svtkIdType newNumPts = 0;
    svtkIdType partnerNumPts = 0;
    for (svtkIdType i = 0; i < numPts; i++)
    {
      int position = static_cast<int>((pts[3 * i + cutaxis] + offset) / length * histSize);
      bool where = position <= cutpos;
      if ((where && side) || (!where && !side))
      {
        pts[newNumPts * 3 + 0] = pts[i * 3 + 0];
        pts[newNumPts * 3 + 1] = pts[i * 3 + 1];
        pts[newNumPts * 3 + 2] = pts[i * 3 + 2];
        newNumPts++;
      }
      else
      {
        partnerPts[partnerNumPts * 3 + 0] = pts[i * 3 + 0];
        partnerPts[partnerNumPts * 3 + 1] = pts[i * 3 + 1];
        partnerPts[partnerNumPts * 3 + 2] = pts[i * 3 + 2];
        partnerNumPts++;
      }
    }

    roundComm->AllGatherVoidArray(&partnerNumPts, pointExchangeCount.data(), 1, SVTK_ID_TYPE);

    // ----------------------------------------
    // 6. exchange points
    // find a partner
    int partner;
    if (side)
    {
      partner = roundRank + (roundNumRanks / 2);
    }
    else
    {
      partner = roundRank - (roundNumRanks / 2);
    }

    svtkIdType toReceive = pointExchangeCount[partner];

    bool even = (roundNumRanks % 2) == 0;
    // If non even number of processes: last one send to 0 and receive nothing.
    if (!even && roundRank == roundNumRanks - 1)
    {
      partner = 0;
      toReceive = 0;
    }

    pts.resize(3 * (newNumPts + toReceive));

    svtkMPICommunicator::Request request;
    const int EXCHANGE_POINT_TAG = 524821;

    if (partnerNumPts > 0)
    {
      roundComm->NoBlockSend(partnerPts.data(), static_cast<int>(3 * partnerNumPts), partner,
        EXCHANGE_POINT_TAG, request);
    }
    if (toReceive > 0)
    {
      roundComm->ReceiveVoidArray(&pts[3 * newNumPts], static_cast<int>(3 * toReceive), SVTK_DOUBLE,
        partner, EXCHANGE_POINT_TAG);
    }

    // If non even number of processes: 0 receive from the last one.
    if (!even && roundRank == 0)
    {
      newNumPts += toReceive;
      partner = roundNumRanks - 1;
      toReceive = pointExchangeCount[partner];
      pts.resize(3 * (newNumPts + toReceive));
      if (toReceive > 0)
      {
        roundComm->ReceiveVoidArray(
          &pts[3 * newNumPts], 3 * toReceive, SVTK_DOUBLE, partner, EXCHANGE_POINT_TAG);
      }
    }

    numPts = newNumPts + toReceive;

    // ----------------------------------------
    // 7. update bounds
    for (int i = 0; i < 3; ++i)
    {
      localLowerBound[i] = std::numeric_limits<double>::max();
      localUpperBound[i] = -std::numeric_limits<double>::max();
    }

    for (svtkIdType i = 0; i < numPts; ++i)
    {
      for (int j = 0; j < 3; j++)
      {
        localLowerBound[j] = std::min(localLowerBound[j], pts[3 * i + j]);
        localUpperBound[j] = std::max(localUpperBound[j], pts[3 * i + j]);
      }
    }

    if (partnerNumPts > 0)
    {
      request.Wait();
    }

    histogram.assign(histSize, 0);
  }

  // update bounding box of the current processor assigned region
  regionBounds[0] = localLowerBound[0];
  regionBounds[2] = localLowerBound[1];
  regionBounds[4] = localLowerBound[2];
  regionBounds[1] = localUpperBound[0];
  regionBounds[3] = localUpperBound[1];
  regionBounds[5] = localUpperBound[2];

  return true;
}

//----------------------------------------------------------------------------
void svtkDistributedPointCloudFilter::GetPointsInsideBounds(svtkMPIController* controller,
  svtkPointSet* input, svtkPointSet* output, const double outterBounds[6])
{
  svtkMPICommunicator* com = svtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  int np = com ? com->GetNumberOfProcesses() : 1;
  int rank = com ? com->GetLocalProcessId() : 0;

  if (!com || np == 1)
  {
    output->ShallowCopy(input);
    return;
  }

  // round bounds to the nearest float value because locator use float internally.
  // Otherwise, points that are exactly on the bounds may be wrongly considered as outside
  // because of the cast.
  double localOutterBounds[6];
  for (int i = 0; i < 3; i++)
  {
    localOutterBounds[2 * i] = std::nextafter(
      static_cast<float>(outterBounds[2 * i]), static_cast<float>(outterBounds[2 * i]) - 1);
    localOutterBounds[2 * i + 1] = std::nextafter(
      static_cast<float>(outterBounds[2 * i + 1]), static_cast<float>(outterBounds[2 * i + 1]) + 1);
  }

  bool emptyData = input->GetNumberOfPoints() == 0;

  std::vector<double> allOutterBounds(np * 6);
  com->AllGather(localOutterBounds, allOutterBounds.data(), 6);

  // size in bytes of messages to be sent to other processes
  std::vector<svtkIdType> messagesSize(np);

  // number of points in messages to be sent to other processes
  std::vector<svtkIdType> messagePointCount(np);

  // array of point ids
  svtkNew<svtkIdTypeArray> idArray;
  std::vector<svtkSmartPointer<svtkCharArray> > dataToSend;
  dataToSend.resize(np);

  // we will need a locator to search points inside each processor assigned regions
  svtkNew<svtkOctreePointLocator> locator;

  if (!emptyData)
  {
    svtkNew<svtkPolyData> inputPolyData;
    inputPolyData->SetPoints(input->GetPoints());
    locator->SetDataSet(inputPolyData.Get());
    locator->BuildLocator();
  }

  // 1st step: define messages to send to each processor (including itself)
  // with polydata containing closest points to local data bounding box
  for (int partner = 0; partner < np; partner++)
  {
    idArray->SetNumberOfTuples(0);
    svtkIdType nPoints = 0;
    svtkIdType* ids = nullptr;
    if (!emptyData)
    {
      double* nbounds = &allOutterBounds[partner * 6];
      locator->FindPointsInArea(nbounds, idArray.Get());
      nPoints = idArray->GetNumberOfTuples();
      ids = idArray->GetPointer(0);
    }

    svtkNew<svtkPolyData> pointCloudToSend;
    svtkNew<svtkPoints> pointsToSend;
    pointsToSend->SetNumberOfPoints(nPoints);
    pointCloudToSend->SetPoints(pointsToSend.Get());

    svtkPointData* pointsToSendPointData = pointCloudToSend->GetPointData();
    pointsToSendPointData->CopyAllocate(input->GetPointData(), nPoints);

    for (svtkIdType i = 0; i < nPoints; i++)
    {
      pointsToSend->SetPoint(i, input->GetPoint(ids[i]));
      pointsToSendPointData->CopyData(input->GetPointData(), ids[i], i);
    }

    // flatten (marshal) point coordinates & data to a raw byte array
    messagePointCount[partner] = nPoints;
    dataToSend[partner] = svtkSmartPointer<svtkCharArray>::New();
    svtkCommunicator::MarshalDataObject(pointCloudToSend.Get(), dataToSend[partner]);
    messagesSize[partner] = dataToSend[partner]->GetNumberOfValues();
  }

  std::vector<svtkSmartPointer<svtkCharArray> > dataToReceive(np);
  std::vector<svtkMPICommunicator::Request> receiveRequests(np);

  // Calculate size of messages to receive
  std::vector<svtkIdType> receiveSize(np);
  std::vector<svtkIdType> receivePointCount(np);

  for (int i = 0; i < np; i++)
  {
    com->Gather(messagesSize.data() + i, receiveSize.data(), 1, i);
    com->Gather(messagePointCount.data() + i, receivePointCount.data(), 1, i);
  }

  // Starting asynchronous receptions
  int nReceive = 0;
  svtkIdType totalPointsToReceive = 0;
  for (int round = 0; round < np - 1; round++)
  {
    int partner = (rank + round + 1) % np;
    if (receiveSize[partner] > 0)
    {
      dataToReceive[partner] = svtkSmartPointer<svtkCharArray>::New();
      com->NoBlockReceive(dataToReceive[partner]->WritePointer(0, receiveSize[partner]),
        receiveSize[partner], partner, 0, receiveRequests[partner]);
      totalPointsToReceive += receivePointCount[partner];
      nReceive++;
    }
  }

  // local sending/receipt is just a pointer assignment
  dataToReceive[rank] = dataToSend[rank];
  dataToSend[rank] = nullptr;
  if (receiveSize[rank] > 0)
  {
    totalPointsToReceive += receivePointCount[rank];
    nReceive++;
  }

  // Starting asynchronous sends
  std::vector<svtkMPICommunicator::Request> sendRequests(np);
  for (int round = 0; round < np - 1; round++)
  {
    int partner = (rank + round + 1) % np;
    if (messagesSize[partner] > 0)
    {
      com->NoBlockSend(dataToSend[partner]->GetPointer(0), messagesSize[partner], partner, 0,
        sendRequests[partner]);
    }
  }

  // sum of received points from the different processors
  svtkIdType totalPoints = 0;
  svtkPointData* outputPointData = output->GetPointData();
  outputPointData->SetNumberOfTuples(totalPointsToReceive);

  while (nReceive > 0)
  {
    for (int round = 0; round < np; round++)
    {
      int partner = (rank + round) % np;
      if ((partner == rank || receiveRequests[partner].Test() == 1) && receiveSize[partner] > 0)
      {
        svtkNew<svtkPolyData> receivedPointCloud;
        svtkCommunicator::UnMarshalDataObject(dataToReceive[partner], receivedPointCloud.Get());

        dataToReceive[partner] = nullptr;
        svtkIdType nbReceivedPoints = receivedPointCloud->GetNumberOfPoints();
        svtkPointData* receivedPointData = receivedPointCloud->GetPointData();
        svtkPoints* receivedPoints = receivedPointCloud->GetPoints();
        svtkPoints* outputPoints = output->GetPoints();
        if (!outputPoints)
        {
          svtkNew<svtkPoints> points;
          outputPoints = points.Get();
          output->SetPoints(outputPoints);
        }
        svtkIdType outputNbPts = outputPoints->GetNumberOfPoints();
        outputPoints->Resize(outputNbPts + nbReceivedPoints);
        for (svtkIdType i = 0; i < nbReceivedPoints; i++)
        {
          outputPoints->InsertNextPoint(receivedPoints->GetPoint(i));
        }
        int nbArray = receivedPointData->GetNumberOfArrays();

        for (int a = 0; a < nbArray; a++)
        {
          svtkAbstractArray* fromArray = receivedPointData->GetAbstractArray(a);
          if (fromArray)
          {
            svtkAbstractArray* toArray = outputPointData->GetAbstractArray(fromArray->GetName());
            if (!toArray)
            {
              toArray = fromArray->NewInstance();
              toArray->SetName(fromArray->GetName());
              toArray->SetNumberOfComponents(fromArray->GetNumberOfComponents());
              toArray->SetNumberOfTuples(totalPointsToReceive);
              outputPointData->AddArray(toArray);
              toArray->Delete();
            }

            for (svtkIdType i = 0; i < nbReceivedPoints; i++)
            {
              toArray->SetTuple(totalPoints + i, i, fromArray);
            }
          }
        }
        totalPoints += nbReceivedPoints;
        nReceive--;
        receiveSize[partner] = 0;
      }
    }
  }

  // we wait for sent messages to be received before deleting them
  for (int round = 0; round < np - 1; round++)
  {
    int partner = (rank + round + 1) % np;
    if (messagesSize[partner] > 0)
    {
      sendRequests[partner].Wait();
    }
  }
}

//----------------------------------------------------------------------------
void svtkDistributedPointCloudFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
