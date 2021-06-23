/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridGhostCellsGenerator.h"

#include "svtkBitArray.h"
#include "svtkCellData.h"
#include "svtkCommunicator.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridNonOrientedCursor.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUniformHyperTreeGrid.h"
#include "svtkUnsignedCharArray.h"

#include <cmath>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkHyperTreeGridGhostCellsGenerator);

struct svtkHyperTreeGridGhostCellsGenerator::svtkInternals
{
  // Controller only has MPI processes which have cells
  svtkMultiProcessController* Controller;
};

namespace
{
struct SendBuffer
{
  SendBuffer()
    : count(0)
    , mask(0)
    , isParent(svtkBitArray::New())
  {
  }
  ~SendBuffer() { isParent->Delete(); }
  svtkIdType count;                // len buffer
  unsigned int mask;              // ghost mask
  std::vector<svtkIdType> indices; // indices for selected cells
  svtkBitArray* isParent;          // decomposition amr tree
};

struct RecvBuffer
{
  RecvBuffer()
    : count(0)
    , offset(0)
  {
  }
  svtkIdType count;  // len buffer
  svtkIdType offset; // offset in field vector
  std::vector<svtkIdType> indices;
};

static const int HTGGCG_SIZE_EXCHANGE_TAG = 5098;
static const int HTGGCG_DATA_EXCHANGE_TAG = 5099;
static const int HTGGCG_DATA2_EXCHANGE_TAG = 5100;
}

//-----------------------------------------------------------------------------
svtkHyperTreeGridGhostCellsGenerator::svtkHyperTreeGridGhostCellsGenerator()
{
  this->AppropriateOutput = true;
  this->Internals = new svtkInternals();
  this->Internals->Controller = svtkMultiProcessController::GetGlobalController();
}

//-----------------------------------------------------------------------------
svtkHyperTreeGridGhostCellsGenerator::~svtkHyperTreeGridGhostCellsGenerator()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridGhostCellsGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridGhostCellsGenerator::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkHyperTreeGridGhostCellsGenerator::ProcessTrees(
  svtkHyperTreeGrid* input, svtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  svtkHyperTreeGrid* output = svtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    svtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // We only need the structure of the input with no data in it
  output->Initialize();

  // local handle on the controller
  svtkMultiProcessController* controller = this->Internals->Controller;

  int processId = controller->GetLocalProcessId();

  int numberOfProcesses = controller->GetNumberOfProcesses();
  if (numberOfProcesses == 1)
  {
    output->DeepCopy(input);
    return 1;
  }
  else
  {
    output->CopyEmptyStructure(input);
  }

  // Link HyperTrees
  svtkHyperTreeGrid::svtkHyperTreeGridIterator inHTs;
  input->InitializeTreeIterator(inHTs);
  svtkIdType inTreeIndex;

  // To keep track of the number of nodes in the htg
  svtkIdType numberOfValues = 0;

  svtkNew<svtkHyperTreeGridNonOrientedCursor> outCursor, inCursor;

  svtkBitArray *outputMask = input->HasMask() ? svtkBitArray::New() : nullptr,
              *inputMask = input->HasMask() ? input->GetMask() : nullptr;

  // First, we copy the input htg into the output
  // We do it "by hand" to fill gaps if they exist
  while (inHTs.GetNextTree(inTreeIndex))
  {
    input->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    output->InitializeNonOrientedCursor(outCursor, inTreeIndex, true);
    outCursor->SetGlobalIndexStart(numberOfValues);
    this->CopyInputTreeToOutput(
      inCursor, outCursor, input->GetPointData(), output->GetPointData(), inputMask, outputMask);
    numberOfValues += outCursor->GetTree()->GetNumberOfVertices();
  }

  // Handling receive and send buffer.
  // The structure is as follows:
  // sendBuffer[id] or recvBuffer[id] == process id of neighbor with who to communicate buffer
  // sendBuffer[id][jd] or recvBuffer[id][jd] tells which tree index is being sent.
  typedef std::map<unsigned int, SendBuffer> SendTreeBufferMap;
  typedef std::map<unsigned int, SendTreeBufferMap> SendProcessBufferMap;
  typedef std::map<unsigned int, RecvBuffer> RecvTreeBufferMap;
  typedef std::map<unsigned int, RecvTreeBufferMap> RecvProcessBufferMap;

  SendProcessBufferMap sendBuffer;
  RecvProcessBufferMap recvBuffer;

  enum FlagType
  {
    NOT_TREATED,
    INITIALIZE_TREE,
    INITIALIZE_FIELD
  };
  std::unordered_map<unsigned, FlagType> flags;

  // Broadcast hyper tree locations to everyone
  unsigned cellDims[3];
  input->GetCellDims(cellDims);
  svtkIdType nbHTs = cellDims[0] * cellDims[1] * cellDims[2];

  std::vector<int> broadcastHyperTreesMapToProcesses(nbHTs + numberOfProcesses, -1),
    hyperTreesMapToProcesses(nbHTs + numberOfProcesses);
  input->InitializeTreeIterator(inHTs);
  while (inHTs.GetNextTree(inTreeIndex))
  {
    input->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    if (inCursor->HasTree())
    {
      broadcastHyperTreesMapToProcesses[inTreeIndex] = processId;
    }
  }
  broadcastHyperTreesMapToProcesses[nbHTs + processId] = input->HasMask();
  controller->AllReduce(broadcastHyperTreesMapToProcesses.data(), hyperTreesMapToProcesses.data(),
    nbHTs + numberOfProcesses, svtkCommunicator::MAX_OP);

  assert(input->GetDimension() > 1);

  // Determining who are my neighbors
  unsigned i, j, k = 0;
  input->InitializeTreeIterator(inHTs);
  switch (input->GetDimension())
  {
    case 2:
    {
      while (inHTs.GetNextTree(inTreeIndex))
      {
        input->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
        {
          for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
          {
            int neighbor = (i + ri) * cellDims[1] + j + rj;
            int id = hyperTreesMapToProcesses[neighbor];
            if (id >= 0 && id != processId)
            {
              // Construction a neighborhood mask to extract the interface in ExtractInterface later
              // on Same encoding as svtkHyperTreeGrid::GetChildMask
              sendBuffer[id][inTreeIndex].mask |= 1
                << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3));
              // Not receiving anything from this guy since we will send him stuff
              recvBuffer[id][neighbor].count = 0;
              // Process not treated yet, yielding the flag
              flags[id] = NOT_TREATED;
            }
          }
        }
      }
      break;
    }
    case 3:
    {
      while (inHTs.GetNextTree(inTreeIndex))
      {
        input->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rk = ((k > 0) ? -1 : 0); rk < (((k + 1) < cellDims[2]) ? 2 : 1); ++rk)
        {
          for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
          {
            for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
            {
              int neighbor = ((k + rk) * cellDims[1] + j + rj) * cellDims[0] + i + ri;
              int id = hyperTreesMapToProcesses[neighbor];
              if (id >= 0 && id != processId)
              {
                // Construction a neighborhood mask to extract the interface in ExtractInterface
                // later on Same encoding as svtkHyperTreeGrid::GetChildMask
                sendBuffer[id][inTreeIndex].mask |= 1
                  << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3 + (rk + 1) * 9));
                // Not receiving anything from this guy since we will send him stuff
                recvBuffer[id][neighbor].count = 0;
                // Process not treated yet, yielding the flag
                flags[id] = NOT_TREATED;
              }
            }
          }
        }
      }
      break;
    }
  }

  // Exchanging size with my neighbors
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = sendBuffer.find(id);
      if (sendIt != sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<svtkIdType> counts(sendTreeMap.size());
        int cpt = 0;
        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          svtkIdType treeId = sendTreeBufferPair.first;
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          input->InitializeNonOrientedCursor(inCursor, treeId);
          // Extracting the tree interface with its neighbors
          sendTreeBuffer.count = 0;
          svtkHyperTree* tree = inCursor->GetTree();
          if (tree)
          {
            // We store the isParent profile along the interface to know when to subdivide later
            // indices store the indices in the input of the nodes on the interface
            svtkIdType nbVertices = tree->GetNumberOfVertices();
            sendTreeBuffer.indices.resize(nbVertices);
            ExtractInterface(inCursor, sendTreeBuffer.isParent, sendTreeBuffer.indices, input,
              sendTreeBuffer.mask, sendTreeBuffer.count);
          }
          // Telling my neighbors how much data I will send later
          counts[cpt++] = sendTreeBuffer.count;
        }
        controller->Send(counts.data(), cpt, id, HTGGCG_SIZE_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving size info from my neighbors
      for (auto&& recvTreeMapPair : recvBuffer)
      {
        unsigned process = recvTreeMapPair.first;
        auto&& recvTreeMap = recvTreeMapPair.second;
        std::vector<svtkIdType> counts(recvTreeMap.size());
        controller->Receive(counts.data(), static_cast<svtkIdType>(recvTreeMap.size()), process,
          HTGGCG_SIZE_EXCHANGE_TAG);
        int cpt = 0;
        for (auto&& recvBufferPair : recvTreeMap)
        {
          recvBufferPair.second.count = counts[cpt++];
        }
      }
    }
  }

  // Synchronizing
  controller->Barrier();

  // Sending masks and parent state of each node
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = sendBuffer.find(id);
      if (sendIt != sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<unsigned char> buf;
        // Accumulated length
        svtkIdType len = 0;
        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          if (sendTreeBuffer.count)
          {
            // We send the bits packed in unsigned char
            svtkIdType dlen = sendTreeBuffer.count / sizeof(unsigned char) + 1;
            if (input->HasMask())
            {
              dlen *= 2;
            }
            buf.resize(len + dlen);
            memcpy(buf.data() + len, sendTreeBuffer.isParent->GetPointer(0),
              sendTreeBuffer.count / sizeof(unsigned char) + 1);
            if (input->HasMask())
            {
              unsigned char* mask = buf.data() + len + dlen / 2;
              for (svtkIdType ii = 0; ii < dlen / 2; ++ii)
              {
                mask[ii] = 0;
              }
              svtkBitArray* bmask = input->GetMask();
              // Filling the mask with bits at the appropriate location
              for (svtkIdType m = 0; m < sendTreeBuffer.count; ++m)
              {
                *mask |= static_cast<unsigned char>(bmask->GetValue(sendTreeBuffer.indices[m]))
                  << (sizeof(unsigned char) - 1 - (m % sizeof(unsigned char)));
                // Incrementing the pointer when unsigned char overflows
                mask += !((m + 1) % sizeof(unsigned char));
              }
            }
            len += dlen;
          }
        }
        this->Internals->Controller->Send(buf.data(), len, id, HTGGCG_DATA_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving masks
      for (auto&& recvTreeMapPair : recvBuffer)
      {
        unsigned process = recvTreeMapPair.first;
        auto&& recvTreeMap = recvTreeMapPair.second;

        // If we have not dealt with process yet,
        // we prepare for receiving with appropriate length
        if (flags[process] == NOT_TREATED)
        {
          svtkIdType len = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              // bit message is packed in unsigned char, getting the correct length of the message
              len += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              if (input->HasMask())
              {
                len += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              }
            }
          }
          std::vector<unsigned char> buf(len);

          this->Internals->Controller->Receive(buf.data(), len, process, HTGGCG_DATA_EXCHANGE_TAG);

          svtkIdType cpt = 0;
          // Distributing receive data among my trees, i.e. creating my ghost trees with this data
          // Remember: we only have the nodes / leaves at the inverface with our neighbor
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            svtkIdType treeId = recvTreeBufferPair.first;
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              output->InitializeNonOrientedCursor(outCursor, treeId, true);
              svtkNew<svtkBitArray> isParent;

              // Stealing ownership of buf in isParent to have svtkBitArray interface
              isParent->SetArray(buf.data() + cpt,
                input->HasMask() ? 2 * recvTreeBuffer.count : recvTreeBuffer.count, 1);

              recvTreeBuffer.offset = numberOfValues;
              recvTreeBuffer.indices.resize(recvTreeBuffer.count);

              outCursor->SetGlobalIndexStart(numberOfValues);

              if (!outputMask && hyperTreesMapToProcesses[nbHTs + process])
              {
                outputMask = svtkBitArray::New();
                outputMask->Resize(numberOfValues);
                for (svtkIdType ii = 0; ii < numberOfValues; ++ii)
                {
                  outputMask->SetValue(ii, 0);
                }
              }

              numberOfValues +=
                this->CreateGhostTree(outCursor, isParent, recvTreeBuffer.indices.data());

              // TODO Bug potentiel si localement on n'a pas de masque... mais le voisin si :D
              if (hyperTreesMapToProcesses[nbHTs + process])
              {
                svtkNew<svtkBitArray> mask;
                // Stealing ownership of buf for mask handling to have svtkBitArray interface

                mask->SetArray(buf.data() + cpt + recvTreeBuffer.count / sizeof(unsigned char) + 1,
                  recvTreeBuffer.count, 1);

                for (svtkIdType m = 0; m < recvTreeBuffer.count; ++m)
                {
                  outputMask->InsertValue(recvTreeBuffer.indices[m], mask->GetValue(m));
                }
                cpt += 2 * (recvTreeBuffer.count / sizeof(unsigned char) + 1);
              }
              else
              {
                if (outputMask)
                {
                  for (svtkIdType m = 0; m < recvTreeBuffer.count; ++m)
                  {
                    outputMask->InsertValue(recvTreeBuffer.indices[m], 0);
                  }
                }
                cpt += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              }
            }
          }
          flags[process] = INITIALIZE_TREE;
        }
      }
    }
  }

  // Synchronizing
  this->Internals->Controller->Barrier();

  // We now send the data store on each node
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = sendBuffer.find(id);
      if (sendIt != sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<double> buf;
        svtkIdType len = 0;
        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          if (sendTreeBuffer.count)
          {
            svtkPointData* pd = input->GetPointData();
            int nbArray = pd->GetNumberOfArrays();
            len += sendTreeBuffer.count * nbArray;
            buf.resize(len);
            double* arr = buf.data() + len - sendTreeBuffer.count * nbArray;
            for (int iArray = 0; iArray < nbArray; ++iArray)
            {
              svtkDataArray* inArray = pd->GetArray(iArray);
              for (svtkIdType m = 0; m < sendTreeBuffer.count; ++m)
              {
                arr[iArray * sendTreeBuffer.count + m] =
                  inArray->GetTuple1(sendTreeBuffer.indices[m]);
              }
            }
          }
        }
        this->Internals->Controller->Send(buf.data(), len, id, HTGGCG_DATA2_EXCHANGE_TAG);
      }
    }
    else
    {
      // We receive the data
      for (auto&& recvTreeMapPair : recvBuffer)
      {
        unsigned process = recvTreeMapPair.first;
        auto&& recvTreeMap = recvTreeMapPair.second;
        if (flags[process] == INITIALIZE_TREE)
        {
          unsigned long len = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            svtkPointData* pd = output->GetPointData();
            int nbArray = pd->GetNumberOfArrays();
            len += recvTreeBufferPair.second.count * nbArray;
          }
          std::vector<double> buf(len);

          this->Internals->Controller->Receive(buf.data(), len, process, HTGGCG_DATA2_EXCHANGE_TAG);
          svtkIdType cpt = 0;

          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            svtkPointData* pd = output->GetPointData();
            int nbArray = pd->GetNumberOfArrays();
            double* arr = buf.data() + cpt;

            for (int d = 0; d < nbArray; ++d)
            {
              svtkDataArray* outArray = pd->GetArray(d);
              for (svtkIdType m = 0; m < recvTreeBuffer.count; ++m)
              {
                outArray->InsertTuple1(
                  recvTreeBuffer.indices[m], arr[d * recvTreeBuffer.count + m]);
              }
            }
            cpt += recvTreeBuffer.count * nbArray;
          }
          flags[process] = INITIALIZE_FIELD;
        }
      }
    }
  }

  this->Internals->Controller->Barrier();
  {
    svtkNew<svtkUnsignedCharArray> scalars;
    scalars->SetNumberOfComponents(1);
    scalars->SetName(svtkDataSetAttributes::GhostArrayName());
    scalars->SetNumberOfTuples(numberOfValues);
    for (svtkIdType ii = 0; ii < input->GetNumberOfVertices(); ++ii)
    {
      scalars->InsertValue(ii, 0);
    }
    for (svtkIdType ii = input->GetNumberOfVertices(); ii < numberOfValues; ++ii)
    {
      scalars->InsertValue(ii, 1);
    }
    output->GetPointData()->AddArray(scalars);
    output->SetMask(outputMask);
  }

  this->UpdateProgress(1.);
  return 1;
}

//-----------------------------------------------------------------------------
void svtkHyperTreeGridGhostCellsGenerator::ExtractInterface(
  svtkHyperTreeGridNonOrientedCursor* inCursor, svtkBitArray* isParent,
  std::vector<svtkIdType>& indices, svtkHyperTreeGrid* grid, unsigned int mask, svtkIdType& pos)
{
  isParent->InsertTuple1(pos, !inCursor->IsLeaf());
  indices[pos] = inCursor->GetGlobalNodeIndex();
  ++pos;
  if (!inCursor->IsLeaf())
  {
    for (int ichild = 0; ichild < inCursor->GetNumberOfChildren(); ++ichild)
    {
      inCursor->ToChild(ichild);
      unsigned int newMask = mask & grid->GetChildMask(ichild);
      if (newMask)
      {
        this->ExtractInterface(inCursor, isParent, indices, grid, newMask, pos);
      }
      else
      {
        isParent->InsertTuple1(pos, 0);
        indices[pos] = inCursor->GetGlobalNodeIndex();
        ++pos;
      }
      inCursor->ToParent();
    }
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkHyperTreeGridGhostCellsGenerator::CreateGhostTree(
  svtkHyperTreeGridNonOrientedCursor* outCursor, svtkBitArray* isParent, svtkIdType* indices,
  svtkIdType&& pos)
{
  indices[pos] = outCursor->GetGlobalNodeIndex();
  if (isParent->GetValue(pos++))
  {
    outCursor->SubdivideLeaf();
    for (int ichild = 0; ichild < outCursor->GetNumberOfChildren(); ++ichild)
    {
      outCursor->ToChild(ichild);
      this->CreateGhostTree(outCursor, isParent, indices, std::forward<svtkIdType&&>(pos));
      outCursor->ToParent();
    }
  }
  return pos;
}

//-----------------------------------------------------------------------------
void svtkHyperTreeGridGhostCellsGenerator::CopyInputTreeToOutput(
  svtkHyperTreeGridNonOrientedCursor* inCursor, svtkHyperTreeGridNonOrientedCursor* outCursor,
  svtkPointData* inPointData, svtkPointData* outPointData, svtkBitArray* inMask, svtkBitArray* outMask)
{
  svtkIdType outIdx = outCursor->GetGlobalNodeIndex(), inIdx = inCursor->GetGlobalNodeIndex();
  outPointData->InsertTuple(outIdx, inIdx, inPointData);
  if (inMask)
  {
    outMask->InsertTuple1(outIdx, inMask->GetValue(inIdx));
  }
  if (!inCursor->IsLeaf())
  {
    outCursor->SubdivideLeaf();
    for (int ichild = 0; ichild < inCursor->GetNumberOfChildren(); ++ichild)
    {
      outCursor->ToChild(ichild);
      inCursor->ToChild(ichild);
      this->CopyInputTreeToOutput(inCursor, outCursor, inPointData, outPointData, inMask, outMask);
      outCursor->ToParent();
      inCursor->ToParent();
    }
  }
}
