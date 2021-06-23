/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPLagrangianParticleTracker.h"

#include "svtkAppendFilter.h"
#include "svtkBoundingBox.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkGenericCell.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLagrangianBasicIntegrationModel.h"
#include "svtkLagrangianParticle.h"
#include "svtkLagrangianThreadedData.h"
#include "svtkLongLongArray.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyLine.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

#include <vector>

#define LAGRANGIAN_PARTICLE_TAG 621
#define LAGRANGIAN_RANG_FLAG_TAG 622
#define LAGRANGIAN_ARRAY_TAG 623

namespace
{
enum CommunicationFlag
{
  SVTK_PLAGRANGIAN_WORKING_FLAG = 0,
  SVTK_PLAGRANGIAN_EMPTY_FLAG = 1,
  SVTK_PLAGRANGIAN_FINISHED_FLAG = 2
};
}

// Class used to serialize and stream a particle
class MessageStream
{
public:
  MessageStream(int BufferSize)
    : Size(BufferSize)
  {
    this->Data.resize(Size);
    this->Head = Data.data();
    this->count = 0;
  }

  ~MessageStream() {}

  int GetSize() { return this->Size; }

  template <class T>
  MessageStream& operator<<(T t)
  {
    size_t size = sizeof(T);
    char* value = reinterpret_cast<char*>(&t);
    for (size_t i = 0; i < size; i++)
    {
      *this->Head++ = *value++;
    }
    return *this;
  }

  template <class T>
  MessageStream& operator>>(T& t)
  {
    size_t size = sizeof(T);
    t = *reinterpret_cast<T*>(this->Head);
    this->Head += size;
    return *this;
  }

  char* GetRawData() { return this->Data.data(); }
  int GetLength() { return this->Head - this->Data.data(); }

  void Reset() { this->Head = this->Data.data(); }
  int count;

private:
  MessageStream(const MessageStream&) = delete;
  void operator=(const MessageStream&) = delete;

  std::vector<char> Data;
  char* Head;
  int Size;
};

// A singleton class used by each rank to stream particle with another
// It sends particle to all other ranks and can receive particle
// from any other rank.
class ParticleStreamManager
{
public:
  ParticleStreamManager(svtkMPIController* controller, svtkPointData* seedData,
    svtkLagrangianBasicIntegrationModel* model, const svtkBoundingBox* bounds)
  {
    // Initialize Members
    this->Controller = controller;
    this->SeedData = seedData;
    this->WeightsSize = model->GetWeightsSize();

    // Gather bounds and initialize requests
    std::vector<double> allBounds(6 * this->Controller->GetNumberOfProcesses(), 0);
    double nodeBounds[6];
    bounds->GetBounds(nodeBounds);
    this->Controller->AllGather(nodeBounds, &allBounds[0], 6);
    for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
    {
      svtkBoundingBox box;
      box.AddBounds(&allBounds[i * 6]);
      this->Boxes.push_back(box);
    }

    // Compute StreamSize for one particle
    // This is strongly linked to Send and Receive code
    this->StreamSize = sizeof(int) * 2 + sizeof(double) * 2 + 4 * sizeof(svtkIdType) + sizeof(int) +
      2 * sizeof(double) +
      3 * sizeof(double) *
        (model->GetNumberOfIndependentVariables() + model->GetNumberOfTrackedUserData());
    for (int i = 0; i < seedData->GetNumberOfArrays(); i++)
    {
      svtkDataArray* array = seedData->GetArray(i);
      this->StreamSize += array->GetNumberOfComponents() * sizeof(double);
    }

    // Initialize Streams
    this->ReceiveStream = new MessageStream(this->StreamSize);
  }

  ~ParticleStreamManager()
  {
    for (size_t i = 0; i < this->SendRequests.size(); i++)
    {
      this->SendRequests[i].first->Wait();
    }
    this->CleanSendRequests();

    // Delete  receive stream
    delete this->ReceiveStream;
  }

  // Method to send a particle to others ranks
  // if particle contained in bounds
  void SendParticle(svtkLagrangianParticle* particle)
  {
    // Serialize particle
    // This is strongly linked to Constructor and Receive code

    MessageStream* sendStream = new MessageStream(this->StreamSize);
    *sendStream << particle->GetSeedId();
    *sendStream << particle->GetId();
    *sendStream << particle->GetParentId();
    *sendStream << particle->GetNumberOfVariables();
    *sendStream << static_cast<int>(particle->GetTrackedUserData().size());
    *sendStream << particle->GetNumberOfSteps();
    *sendStream << particle->GetIntegrationTime();
    *sendStream << particle->GetPrevIntegrationTime();
    *sendStream << particle->GetUserFlag();
    *sendStream << particle->GetPInsertPreviousPosition();
    *sendStream << particle->GetPManualShift();

    double* prev = particle->GetPrevEquationVariables();
    double* curr = particle->GetEquationVariables();
    double* next = particle->GetNextEquationVariables();
    for (int i = 0; i < particle->GetNumberOfVariables(); i++)
    {
      *sendStream << prev[i];
      *sendStream << curr[i];
      *sendStream << next[i];
    }

    for (auto data : particle->GetPrevTrackedUserData())
    {
      *sendStream << data;
    }
    for (auto data : particle->GetTrackedUserData())
    {
      *sendStream << data;
    }
    for (auto data : particle->GetNextTrackedUserData())
    {
      *sendStream << data;
    }

    for (int i = 0; i < particle->GetSeedData()->GetNumberOfArrays(); i++)
    {
      svtkDataArray* array = particle->GetSeedData()->GetArray(i);
      double* tuple = array->GetTuple(particle->GetSeedArrayTupleIndex());
      for (int j = 0; j < array->GetNumberOfComponents(); j++)
      {
        *sendStream << tuple[j];
      }
    }

    // clean out old requests & sendStreams
    this->CleanSendRequests();

    // Send to other ranks
    for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
    {
      if (i == this->Controller->GetLocalProcessId())
      {
        continue;
      }
      if (particle->GetPManualShift() || this->Boxes[i].ContainsPoint(particle->GetPosition()))
      {
        ++sendStream->count; // increment counter on message
        this->SendRequests.push_back(std::make_pair(new svtkMPICommunicator::Request, sendStream));
        this->Controller->NoBlockSend(sendStream->GetRawData(), this->StreamSize, i,
          LAGRANGIAN_PARTICLE_TAG, *this->SendRequests.back().first);
      }
    }
  }

  // Method to receive and deserialize a particle from any other rank
  bool ReceiveParticleIfAny(svtkLagrangianParticle*& particle)
  {
    int probe, source;
    if (this->Controller->Iprobe(
          svtkMultiProcessController::ANY_SOURCE, LAGRANGIAN_PARTICLE_TAG, &probe, &source) &&
      probe)
    {
      this->ReceiveStream->Reset();
      this->Controller->Receive(this->ReceiveStream->GetRawData(), this->StreamSize,
        svtkMultiProcessController::ANY_SOURCE, LAGRANGIAN_PARTICLE_TAG);
      // Deserialize particle
      // This is strongly linked to Constructor and Send method
      int nVar, userFlag, nTrackedUserData;
      svtkIdType seedId, particleId, parentId, nSteps;
      double iTime, prevITime;
      bool pInsertPrevious, pManualShift;
      *this->ReceiveStream >> seedId;
      *this->ReceiveStream >> particleId;
      *this->ReceiveStream >> parentId;
      *this->ReceiveStream >> nVar;
      *this->ReceiveStream >> nTrackedUserData;
      *this->ReceiveStream >> nSteps;
      *this->ReceiveStream >> iTime;
      *this->ReceiveStream >> prevITime;
      *this->ReceiveStream >> userFlag;
      *this->ReceiveStream >> pInsertPrevious;
      *this->ReceiveStream >> pManualShift;

      // Create a particle with out of range seedData
      particle = svtkLagrangianParticle::NewInstance(nVar, seedId, particleId,
        this->SeedData->GetNumberOfTuples(), iTime, this->SeedData, this->WeightsSize,
        nTrackedUserData, nSteps, prevITime);
      particle->SetParentId(parentId);
      particle->SetUserFlag(userFlag);
      particle->SetPInsertPreviousPosition(pInsertPrevious);
      particle->SetPManualShift(pManualShift);
      double* prev = particle->GetPrevEquationVariables();
      double* curr = particle->GetEquationVariables();
      double* next = particle->GetNextEquationVariables();
      for (int i = 0; i < nVar; i++)
      {
        *this->ReceiveStream >> prev[i];
        *this->ReceiveStream >> curr[i];
        *this->ReceiveStream >> next[i];
      }

      std::vector<double>& prevTracked = particle->GetPrevTrackedUserData();
      for (auto& var : prevTracked)
      {
        *this->ReceiveStream >> var;
      }
      std::vector<double>& tracked = particle->GetTrackedUserData();
      for (auto& var : tracked)
      {
        *this->ReceiveStream >> var;
      }
      std::vector<double>& nextTracked = particle->GetNextTrackedUserData();
      for (auto& var : nextTracked)
      {
        *this->ReceiveStream >> var;
      }

      // Recover the correct seed data values and write them into the seedData
      // So particle seed data become correct
      for (int i = 0; i < this->SeedData->GetNumberOfArrays(); i++)
      {
        svtkDataArray* array = this->SeedData->GetArray(i);
        int numComponents = array->GetNumberOfComponents();
        std::vector<double> xi(numComponents);
        for (int j = 0; j < numComponents; j++)
        {
          *this->ReceiveStream >> xi[j];
        }
        array->InsertNextTuple(&xi[0]);
      }
      return true;
    }
    return false;
  }

  void CleanSendRequests()
  {
    auto it = SendRequests.begin();
    while (it != SendRequests.end())
    {
      if (it->first->Test())
      {
        delete it->first;    // delete Request
        --it->second->count; // decrement counter
        if (it->second->count == 0)
        {
          // delete the SendStream
          delete it->second;
        }
        it = SendRequests.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

private:
  svtkMPIController* Controller;
  int StreamSize;
  int WeightsSize;
  MessageStream* ReceiveStream;
  svtkPointData* SeedData;
  ParticleStreamManager(const ParticleStreamManager&) {}
  std::vector<svtkBoundingBox> Boxes;
  std::vector<std::pair<svtkMPICommunicator::Request*, MessageStream*> > SendRequests;
};

// Class used by the master rank to receive and send flag
// to other ranks
class MasterFlagManager
{
public:
  MasterFlagManager(svtkMPIController* controller)
  {
    this->Controller = controller;

    this->NRank = this->Controller->GetNumberOfProcesses() - 1;
    this->RankStates.resize(this->NRank);
    this->SentFlag = nullptr;
    this->SendRequests.resize(this->NRank);
    for (int i = 0; i < this->NRank; i++)
    {
      this->RankStates[i] = SVTK_PLAGRANGIAN_WORKING_FLAG;
      this->SendRequests[i] = nullptr;
    }
  }

  ~MasterFlagManager()
  {
    for (int i = 0; i < this->NRank; i++)
    {
      delete this->SendRequests[i];
    }
    delete this->SentFlag;
  }

  // Send a flag to all other ranks
  void SendFlag(int flag)
  {
    delete this->SentFlag;
    this->SentFlag = new int;
    *this->SentFlag = flag;
    for (int i = 0; i < this->NRank; i++)
    {
      if (this->SendRequests[i])
      {
        this->SendRequests[i]->Wait();
        delete this->SendRequests[i];
      }
      this->SendRequests[i] = new svtkMPICommunicator::Request;
      this->Controller->NoBlockSend(
        this->SentFlag, 1, i + 1, LAGRANGIAN_RANG_FLAG_TAG, *this->SendRequests[i]);
    }
  }

  // Receive flag from other ranks
  // This method should not be used directly
  int* UpdateAndGetFlags()
  {
    int probe, source;
    while (this->Controller->Iprobe(
             svtkMultiProcessController::ANY_SOURCE, LAGRANGIAN_RANG_FLAG_TAG, &probe, &source) &&
      probe)
    {
      this->Controller->Receive(&this->RankStates[source - 1], 1, source, LAGRANGIAN_RANG_FLAG_TAG);
    }
    return this->RankStates.data();
  }

  // Return true if all other ranks have the argument flag,
  // false otherwise
  bool LookForSameFlags(int flag)
  {
    this->UpdateAndGetFlags();
    for (int i = 0; i < this->NRank; i++)
    {
      if (this->RankStates[i] != flag)
      {
        return false;
      }
    }
    return true;
  }

  // Return true if any of the other rank have the argument flag
  // false otherwise
  bool LookForAnyFlag(int flag)
  {
    this->UpdateAndGetFlags();
    for (int i = 0; i < this->NRank; i++)
    {
      if (this->RankStates[i] == flag)
      {
        return true;
      }
    }
    return false;
  }

private:
  svtkMPIController* Controller;
  int NRank;
  int* SentFlag;
  std::vector<int> RankStates;
  std::vector<svtkMPICommunicator::Request*> SendRequests;
};

// Class used by non master ranks to communicate with master rank
class RankFlagManager
{
public:
  RankFlagManager(svtkMPIController* controller)
  {
    this->Controller = controller;

    // Initialize flags
    this->LastFlag = SVTK_PLAGRANGIAN_WORKING_FLAG;
    this->SentFlag = nullptr;
    this->SendRequest = nullptr;
  }

  ~RankFlagManager()
  {
    delete this->SendRequest;
    delete this->SentFlag;
  }

  // Send a flag to master
  void SendFlag(char flag)
  {
    delete this->SentFlag;
    this->SentFlag = new int;
    *this->SentFlag = flag;
    if (this->SendRequest)
    {
      this->SendRequest->Wait();
      delete this->SendRequest;
    }
    this->SendRequest = new svtkMPICommunicator::Request;
    this->Controller->NoBlockSend(
      this->SentFlag, 1, 0, LAGRANGIAN_RANG_FLAG_TAG, *this->SendRequest);
  }

  // Receive flag from master if any and return it
  int UpdateAndGetFlag()
  {
    int probe;
    while (this->Controller->Iprobe(0, LAGRANGIAN_RANG_FLAG_TAG, &probe, nullptr) && probe)
    {
      this->Controller->Receive(&this->LastFlag, 1, 0, LAGRANGIAN_RANG_FLAG_TAG);
    }
    return this->LastFlag;
  }

private:
  svtkMPIController* Controller;
  int* SentFlag;
  int LastFlag;
  svtkMPICommunicator::Request* SendRequest;
};

svtkStandardNewMacro(svtkPLagrangianParticleTracker);

//---------------------------------------------------------------------------
svtkPLagrangianParticleTracker::svtkPLagrangianParticleTracker()
  : Controller(svtkMPIController::SafeDownCast(svtkMultiProcessController::GetGlobalController()))
  , StreamManager(nullptr)
  , MFlagManager(nullptr)
  , RFlagManager(nullptr)
{
  // To get a correct progress update
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->IntegratedParticleCounterIncrement = this->Controller->GetNumberOfProcesses();
  }
}

//---------------------------------------------------------------------------
svtkPLagrangianParticleTracker::~svtkPLagrangianParticleTracker()
{
  delete RFlagManager;
  delete MFlagManager;
  delete StreamManager;
}

//---------------------------------------------------------------------------
int svtkPLagrangianParticleTracker::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  svtkInformation* info = inputVector[0]->GetInformationObject(0);
  if (info)
  {
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  if (sourceInfo)
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  svtkInformation* surfaceInfo = inputVector[2]->GetInformationObject(0);
  if (surfaceInfo)
  {
    surfaceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
    surfaceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
    surfaceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  return 1;
}

//---------------------------------------------------------------------------
void svtkPLagrangianParticleTracker::GenerateParticles(const svtkBoundingBox* bounds,
  svtkDataSet* seeds, svtkDataArray* initialVelocities, svtkDataArray* initialIntegrationTimes,
  svtkPointData* seedData, int nVar, std::queue<svtkLagrangianParticle*>& particles)
{
  // Generate particle
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->ParticleCounter = this->Controller->GetLocalProcessId();

    // delete potential remaining managers
    delete RFlagManager;
    delete MFlagManager;
    delete StreamManager;

    // Reduce SeedData Arrays
    int nArrays = seedData->GetNumberOfArrays();
    int actualNArrays;
    int rank = this->Controller->GetLocalProcessId();
    int dummyRank = -1;
    int fullArrayRank;

    // Recover maximum number of arrays
    this->Controller->AllReduce(&nArrays, &actualNArrays, 1, svtkCommunicator::MAX_OP);
    if (actualNArrays != nArrays)
    {
      // This rank does not have the maximum number of arrays
      if (nArrays != 0)
      {
        // this rank have an incorrect number of arrays, not supposed to happen
        svtkErrorMacro("Something went wrong with seed data arrays, discarding arrays");
        for (int i = nArrays - 1; i >= 0; i--)
        {
          seedData->RemoveArray(i);
        }
      }

      // Rank without any seeds, does not have access to the structure of
      // seeds pointData.
      // Recover this information from another rank.
      this->Controller->AllReduce(&dummyRank, &fullArrayRank, 1, svtkCommunicator::MAX_OP);
      int source, size;
      char type;
      int probe = false;
      while (!probe)
      {
        // Wait for the arrays metadata to be sent
        this->Controller->Iprobe(
          fullArrayRank, LAGRANGIAN_ARRAY_TAG, &probe, &source, &type, &size);
      }
      MessageStream stream(size);
      // Receive arrays metadata
      this->Controller->Receive(stream.GetRawData(), size, source, LAGRANGIAN_ARRAY_TAG);
      for (int i = 0; i < actualNArrays; i++)
      {
        // Create arrays according to metadata
        int dataType, nComponents, nameLen, compNameLen;
        stream >> dataType;
        svtkDataArray* array = svtkDataArray::CreateDataArray(dataType);
        stream >> nComponents;
        array->SetNumberOfComponents(nComponents);
        stream >> nameLen;
        std::vector<char> name(nameLen + 1, 0);
        for (int l = 0; l < nameLen; l++)
        {
          stream >> name[l];
        }
        array->SetName(&name[0]);
        for (int idComp = 0; idComp < nComponents; idComp++)
        {
          stream >> compNameLen;
          if (compNameLen > 0)
          {
            std::vector<char> compName(compNameLen + 1, 0);
            for (int compLength = 0; compLength < compNameLen; compLength++)
            {
              stream >> compName[compLength];
            }
            array->SetComponentName(idComp, &compName[0]);
          }
        }
        seedData->AddArray(array);
        array->Delete();
      }
    }
    else
    {
      // This rank contains the correct number of arrays
      this->Controller->AllReduce(&rank, &fullArrayRank, 1, svtkCommunicator::MAX_OP);

      // Select the highest rank containing arrays to be the one to be right about arrays metadata
      if (fullArrayRank == rank)
      {
        // Generate arrays metadata
        int streamSize = 0;
        streamSize += nArrays * 3 * sizeof(int);
        // nArrays * (Datatype + nComponents + strlen(name));
        for (int i = 0; i < nArrays; i++)
        {
          svtkDataArray* array = seedData->GetArray(i);
          const char* name = array->GetName();
          streamSize += static_cast<int>(strlen(name)); // name
          int nComp = array->GetNumberOfComponents();
          for (int idComp = 0; idComp < nComp; idComp++)
          {
            streamSize += sizeof(int);
            const char* compName = array->GetComponentName(idComp);
            if (compName)
            {
              streamSize += static_cast<int>(strlen(compName));
            }
          }
        }
        MessageStream stream(streamSize);
        for (int i = 0; i < nArrays; i++)
        {
          svtkDataArray* array = seedData->GetArray(i);
          stream << array->GetDataType();
          stream << array->GetNumberOfComponents();
          const char* name = array->GetName();
          int nameLen = static_cast<int>(strlen(name));
          stream << nameLen;
          for (int l = 0; l < nameLen; l++)
          {
            stream << name[l];
          }
          for (int idComp = 0; idComp < array->GetNumberOfComponents(); idComp++)
          {
            const char* compName = array->GetComponentName(idComp);
            int compNameLen = 0;
            if (compName)
            {
              compNameLen = static_cast<int>(strlen(compName));
              stream << compNameLen;
              for (int compLength = 0; compLength < compNameLen; compLength++)
              {
                stream << compName[compLength];
              }
            }
            else
            {
              stream << compNameLen;
            }
          }
        }

        // Send to arrays metadata to all other ranks
        for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
        {
          if (i == this->Controller->GetLocalProcessId())
          {
            continue;
          }
          this->Controller->Send(stream.GetRawData(), streamSize, i, LAGRANGIAN_ARRAY_TAG);
        }
      }
      else
      {
        // Other ranks containing correct number of arrays, check metadata is correct
        char type;
        int source, size;
        int probe = false;
        while (!probe)
        {
          // Wait for array metadata
          this->Controller->Iprobe(
            fullArrayRank, LAGRANGIAN_ARRAY_TAG, &probe, &source, &type, &size);
        }
        MessageStream stream(size);
        // Receive array metadata
        this->Controller->Receive(stream.GetRawData(), size, source, LAGRANGIAN_ARRAY_TAG);
        // Check data arrays
        for (int i = 0; i < nArrays; i++)
        {
          svtkDataArray* array = seedData->GetArray(i);
          int dataType, nComponents, nameLen, compNameLen;
          stream >> dataType;
          if (dataType != array->GetDataType())
          {
            svtkErrorMacro("Incoherent dataType between nodes, results may be invalid");
          }
          stream >> nComponents;
          if (nComponents != array->GetNumberOfComponents())
          {
            svtkErrorMacro("Incoherent number of components between nodes, "
                          "results may be invalid");
          }
          const char* localName = array->GetName();
          stream >> nameLen;
          std::vector<char> name(nameLen + 1, 0);
          for (int l = 0; l < nameLen; l++)
          {
            stream >> name[l];
          }
          if (strcmp(&name[0], localName) != 0)
          {
            svtkErrorMacro("Incoherent array names between nodes, "
                          "results may be invalid");
          }
          for (int idComp = 0; idComp < nComponents; idComp++)
          {
            stream >> compNameLen;
            const char* localCompName = array->GetComponentName(idComp);
            std::vector<char> compName(compNameLen + 1, 0);
            for (int compLength = 0; compLength < compNameLen; compLength++)
            {
              stream >> compName[compLength];
            }
            if (localCompName && strcmp(&compName[0], localCompName) != 0)
            {
              svtkErrorMacro("Incoherent array component names between nodes, "
                            "results may be invalid");
            }
          }
        }
      }
    }

    // Create managers
    this->StreamManager =
      new ParticleStreamManager(this->Controller, seedData, this->IntegrationModel, bounds);
    if (this->Controller->GetLocalProcessId() == 0)
    {
      this->MFlagManager = new MasterFlagManager(this->Controller);
    }
    else
    {
      this->RFlagManager = new RankFlagManager(this->Controller);
    }

    // Create and set a dummy particle so FindInLocators can use caching.
    std::unique_ptr<svtkLagrangianThreadedData> dummyData(new svtkLagrangianThreadedData);
    svtkLagrangianParticle dummyParticle(
      0, 0, 0, 0, 0, nullptr, this->IntegrationModel->GetWeightsSize(), 0);
    dummyParticle.SetThreadedData(dummyData.get());

    // Generate particle and distribute the ones not in domain to other nodes
    for (svtkIdType i = 0; i < seeds->GetNumberOfPoints(); i++)
    {
      double position[3];
      seeds->GetPoint(i, position);
      double initialIntegrationTime =
        initialIntegrationTimes ? initialIntegrationTimes->GetTuple1(i) : 0;
      svtkIdType particleId = this->GetNewParticleId();
      svtkLagrangianParticle* particle = new svtkLagrangianParticle(nVar, particleId, particleId, i,
        initialIntegrationTime, seedData, this->IntegrationModel->GetWeightsSize(),
        this->IntegrationModel->GetNumberOfTrackedUserData());
      memcpy(particle->GetPosition(), position, 3 * sizeof(double));
      initialVelocities->GetTuple(i, particle->GetVelocity());
      this->IntegrationModel->InitializeParticle(particle);
      if (this->IntegrationModel->FindInLocators(particle->GetPosition(), &dummyParticle))
      {
        particles.push(particle);
      }
      else
      {
        this->StreamManager->SendParticle(particle);
        delete particle;
      }
    }
    this->Controller->Barrier();
    this->ReceiveParticles(particles);
  }
  else
  {
    this->Superclass::GenerateParticles(
      bounds, seeds, initialVelocities, initialIntegrationTimes, seedData, nVar, particles);
  }
}

//---------------------------------------------------------------------------
void svtkPLagrangianParticleTracker::GetParticleFeed(
  std::queue<svtkLagrangianParticle*>& particleQueue)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    return;
  }

  // Receive particles first
  this->ReceiveParticles(particleQueue);

  // Particle queue is still empty
  if (particleQueue.empty())
  {
    if (this->Controller->GetLocalProcessId() == 0)
    {
      bool finished = false;
      do
      {
        // We are master, with no more particle, wait for all ranks to be empty
        if (this->MFlagManager->LookForSameFlags(SVTK_PLAGRANGIAN_EMPTY_FLAG))
        {
          // check for new particle
          this->ReceiveParticles(particleQueue);

          // Still empty
          if (particleQueue.empty())
          {
            // Everybody empty now, inform ranks
            this->MFlagManager->SendFlag(SVTK_PLAGRANGIAN_EMPTY_FLAG);
            finished = false;
            bool working = false;
            while (!finished && !working)
            {
              // Wait for rank to answer finished or working
              working = this->MFlagManager->LookForAnyFlag(SVTK_PLAGRANGIAN_WORKING_FLAG);
              finished = this->MFlagManager->LookForSameFlags(SVTK_PLAGRANGIAN_FINISHED_FLAG);
              if (working)
              {
                // A rank received a particle in the meantime and is working,
                // resume the wait
                this->MFlagManager->SendFlag(SVTK_PLAGRANGIAN_WORKING_FLAG);
              }
              if (finished)
              {
                // Nobody is working anymore, send finished flag and finish ourself
                this->MFlagManager->SendFlag(SVTK_PLAGRANGIAN_FINISHED_FLAG);
              }
            }
          }
        }
        // Receive Particles before looking at flags
        this->ReceiveParticles(particleQueue);
      } while (particleQueue.empty() && !finished);
    }
    else
    {
      // We are a rank with no more particle, send empty flag
      this->RFlagManager->SendFlag(SVTK_PLAGRANGIAN_EMPTY_FLAG);
      bool finished = false;
      do
      {
        // Wait for master inform us everybody is empty
        bool allEmpty = (this->RFlagManager->UpdateAndGetFlag() == SVTK_PLAGRANGIAN_EMPTY_FLAG);

        // Char for new particles
        this->ReceiveParticles(particleQueue);
        if (!particleQueue.empty())
        {
          // Received a particle, keep on working
          this->RFlagManager->SendFlag(SVTK_PLAGRANGIAN_WORKING_FLAG);
        }
        else if (allEmpty)
        {
          // Nobody has a particle anymore, send finished flag
          this->RFlagManager->SendFlag(SVTK_PLAGRANGIAN_FINISHED_FLAG);
          bool working = false;
          while (!finished && !working)
          {
            // Wait for master to send finished flag
            int flag = this->RFlagManager->UpdateAndGetFlag();
            if (flag == SVTK_PLAGRANGIAN_FINISHED_FLAG)
            {
              // we are finished now
              finished = true;
            }
            else if (flag == SVTK_PLAGRANGIAN_WORKING_FLAG)
            {
              // Another rank is working, resume the wait
              this->RFlagManager->SendFlag(SVTK_PLAGRANGIAN_EMPTY_FLAG);
              working = true;
            }
          }
        }
      } while (particleQueue.empty() && !finished);
    }
  }
}

//---------------------------------------------------------------------------
int svtkPLagrangianParticleTracker::Integrate(svtkInitialValueProblemSolver* integrator,
  svtkLagrangianParticle* particle, std::queue<svtkLagrangianParticle*>& particleQueue,
  svtkPolyData* particlePathsOutput, svtkPolyLine* particlePath, svtkDataObject* interactionOutput)
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    if (this->GenerateParticlePathsOutput && particle->GetPInsertPreviousPosition())
    {
      // This is a particle from another rank, store a duplicated previous point
      this->InsertPathOutputPoint(particle, particlePathsOutput, particlePath->GetPointIds(), true);
      particle->SetPInsertPreviousPosition(false);
    }
  }

  int ret = this->svtkLagrangianParticleTracker::Integrate(
    integrator, particle, particleQueue, particlePathsOutput, particlePath, interactionOutput);

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    if (particle->GetTermination() == svtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_DOMAIN)
    {
      if (!particle->GetPManualShift())
      {
        particle->SetPInsertPreviousPosition(true);
      }

      // Stream out of domain particles
      std::lock_guard<std::mutex> guard(this->StreamManagerMutex);
      this->StreamManager->SendParticle(particle);
    }
  }
  return ret;
}

//---------------------------------------------------------------------------
void svtkPLagrangianParticleTracker::ReceiveParticles(
  std::queue<svtkLagrangianParticle*>& particleQueue)
{
  svtkLagrangianParticle* receivedParticle;

  // Create and set a dummy particle so FindInLocators can use caching.
  std::unique_ptr<svtkLagrangianThreadedData> dummyData(new svtkLagrangianThreadedData);
  svtkLagrangianParticle dummyParticle(
    0, 0, 0, 0, 0, nullptr, this->IntegrationModel->GetWeightsSize(), 0);
  dummyParticle.SetThreadedData(dummyData.get());

  while (this->StreamManager->ReceiveParticleIfAny(receivedParticle))
  {
    // Check for manual shift
    if (receivedParticle->GetPManualShift())
    {
      this->IntegrationModel->ParallelManualShift(receivedParticle);
      receivedParticle->SetPManualShift(false);
    }
    // Receive all particles
    if (this->IntegrationModel->FindInLocators(receivedParticle->GetPosition(), &dummyParticle))
    {
      particleQueue.push(receivedParticle);
    }
    else
    {
      delete receivedParticle;
    }
  }
}

//---------------------------------------------------------------------------
bool svtkPLagrangianParticleTracker::FinalizeOutputs(
  svtkPolyData* particlePathsOutput, svtkDataObject* interactionOutput)
{
  if (this->GenerateParticlePathsOutput && this->Controller &&
    this->Controller->GetNumberOfProcesses() > 1)
  {
    // Construct array with all non outofdomains ids and terminations
    svtkNew<svtkLongLongArray> idTermination;
    svtkNew<svtkLongLongArray> allIdTermination;
    idTermination->Allocate(particlePathsOutput->GetNumberOfCells());
    idTermination->SetNumberOfComponents(2);
    svtkIntArray* terminations =
      svtkIntArray::SafeDownCast(particlePathsOutput->GetCellData()->GetArray("Termination"));
    svtkLongLongArray* ids =
      svtkLongLongArray::SafeDownCast(particlePathsOutput->GetCellData()->GetArray("Id"));
    for (int i = 0; i < particlePathsOutput->GetNumberOfCells(); i++)
    {
      if (terminations->GetValue(i) != svtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_DOMAIN)
      {
        idTermination->InsertNextTuple2(ids->GetValue(i), terminations->GetValue(i));
      }
    }
    idTermination->Squeeze();

    // AllGather it
    this->Controller->AllGatherV(idTermination, allIdTermination);

    // Modify current terminations
    for (svtkIdType i = 0; i < allIdTermination->GetNumberOfTuples(); i++)
    {
      svtkIdType id = allIdTermination->GetTuple2(i)[0];
      for (svtkIdType j = 0; j < particlePathsOutput->GetNumberOfCells(); j++)
      {
        if (ids->GetValue(j) == id)
        {
          terminations->SetTuple1(j, allIdTermination->GetTuple2(i)[1]);
        }
      }
    }
  }
  return this->Superclass::FinalizeOutputs(particlePathsOutput, interactionOutput);
}

//---------------------------------------------------------------------------
bool svtkPLagrangianParticleTracker::UpdateSurfaceCacheIfNeeded(svtkDataObject*& surfaces)
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    // Update local cache and reduce cache status
    int localCacheUpdated = this->Superclass::UpdateSurfaceCacheIfNeeded(surfaces);
    int maxLocalCacheUpdated;
    this->Controller->AllReduce(
      &localCacheUpdated, &maxLocalCacheUpdated, 1, svtkCommunicator::MAX_OP);

    if (!maxLocalCacheUpdated)
    {
      // Cache is still valid, use already reduced surface
      if (svtkDataSet::SafeDownCast(surfaces))
      {
        surfaces = this->TmpSurfaceInput;
      }
      else // if (svtkCompositeDataSet::SafeDownCast(surfaces))
      {
        surfaces = this->TmpSurfaceInputMB;
      }
      return false;
    }

    // Local cache has been updated, update temporary reduced surface
    // In Parallel, reduce surfaces on rank 0, which then broadcast them to all ranks.

    // Recover all surfaces on rank 0
    std::vector<svtkSmartPointer<svtkDataObject> > allSurfaces;
    this->Controller->Gather(surfaces, allSurfaces, 0);

    // Manager dataset case
    if (svtkDataSet::SafeDownCast(surfaces))
    {
      if (this->Controller->GetLocalProcessId() == 0)
      {
        // Rank 0 append all dataset together
        svtkNew<svtkAppendFilter> append;
        for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
        {
          svtkDataSet* ds = svtkDataSet::SafeDownCast(allSurfaces[i]);
          if (ds)
          {
            append->AddInputData(ds);
          }
        }
        append->Update();
        this->TmpSurfaceInput->ShallowCopy(append->GetOutput());
      }

      // Broadcast resulting UnstructuredGrid
      this->Controller->Broadcast(this->TmpSurfaceInput, 0);
      surfaces = this->TmpSurfaceInput;
    }

    // Composite case
    else if (svtkCompositeDataSet::SafeDownCast(surfaces))
    {
      if (this->Controller->GetLocalProcessId() == 0)
      {
        // Rank 0 reconstruct Composite tree
        svtkCompositeDataSet* mb = svtkCompositeDataSet::SafeDownCast(surfaces);
        this->TmpSurfaceInputMB->ShallowCopy(mb);
        svtkCompositeDataIterator* iter = mb->NewIterator();
        iter->SkipEmptyNodesOff();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          // Rank 0 append all dataset together
          svtkNew<svtkAppendFilter> append;
          for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
          {
            svtkCompositeDataSet* localMb = svtkCompositeDataSet::SafeDownCast(allSurfaces[i]);
            svtkDataSet* ds = svtkDataSet::SafeDownCast(localMb->GetDataSet(iter));
            if (ds)
            {
              append->AddInputData(ds);
            }
          }
          append->Update();
          this->TmpSurfaceInputMB->SetDataSet(iter, append->GetOutput());
        }
        iter->Delete();
      }
      // Broadcast resulting Composite
      this->Controller->Broadcast(this->TmpSurfaceInputMB, 0);
      surfaces = this->TmpSurfaceInputMB;
    }
    else
    {
      svtkErrorMacro("Unrecognized surface.");
    }
    return true;
  }
  else
  {
    return this->Superclass::UpdateSurfaceCacheIfNeeded(surfaces);
  }
}

//---------------------------------------------------------------------------
svtkIdType svtkPLagrangianParticleTracker::GetNewParticleId()
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    svtkIdType id = this->ParticleCounter;
    this->ParticleCounter += this->Controller->GetNumberOfProcesses();
    return id;
  }
  return this->Superclass::GetNewParticleId();
}

//---------------------------------------------------------------------------
void svtkPLagrangianParticleTracker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
