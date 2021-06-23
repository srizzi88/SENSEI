/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPStructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPStructuredDataWriter.h"
#include "svtkCommunicator.h"
#include "svtkDataSet.h"
#include "svtkErrorCode.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLStructuredDataWriter.h"

//----------------------------------------------------------------------------
svtkXMLPStructuredDataWriter::svtkXMLPStructuredDataWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPStructuredDataWriter::~svtkXMLPStructuredDataWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPStructuredDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkXMLPStructuredDataWriter::WriteInternal()
{
  int retVal = this->Superclass::WriteInternal();
  if (retVal == 0 || !this->GetContinuingExecution())
  {
    this->Extents.clear();
  }
  return retVal;
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredDataWriter::WritePrimaryElementAttributes(ostream& os, svtkIndent indent)
{
  int* wExt =
    this->GetInputInformation(0, 0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  this->WriteVectorAttribute("WholeExtent", 6, wExt);
  this->Superclass::WritePrimaryElementAttributes(os, indent);
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredDataWriter::WritePPieceAttributes(int index)
{
  if (this->Extents.find(index) != this->Extents.end())
  {
    this->WriteVectorAttribute("Extent", 6, &this->Extents[index][0]);
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }
  this->Superclass::WritePPieceAttributes(index);
}

//----------------------------------------------------------------------------
svtkXMLWriter* svtkXMLPStructuredDataWriter::CreatePieceWriter(int index)
{
  svtkXMLStructuredDataWriter* pWriter = this->CreateStructuredPieceWriter();
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);
  pWriter->SetGhostLevel(this->GhostLevel);

  return pWriter;
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredDataWriter::PrepareSummaryFile()
{
  this->Superclass::PrepareSummaryFile();

  // The code below gathers extens from all processes to write in the
  // meta-file. Note that the extent of each piece was already stored by
  // each writer in WritePiece(). This is gathering it all to root node.
  if (this->Controller)
  {
    // Even though the logic is pretty straightforward, we need to
    // do a fair amount of work to use GatherV. Each rank simply
    // serializes its extents to 7 int blocks - piece number and 6
    // extent values. Then we gather this all to root.
    int rank = this->Controller->GetLocalProcessId();
    int nRanks = this->Controller->GetNumberOfProcesses();

    int nPiecesTotal = 0;
    svtkIdType nPieces = static_cast<svtkIdType>(this->Extents.size());

    svtkIdType* offsets = nullptr;
    svtkIdType* nPiecesAll = nullptr;
    svtkIdType* recvLengths = nullptr;
    if (rank == 0)
    {
      nPiecesAll = new svtkIdType[nRanks];
      recvLengths = new svtkIdType[nRanks];
      offsets = new svtkIdType[nRanks];
    }
    this->Controller->Gather(&nPieces, nPiecesAll, 1, 0);
    if (rank == 0)
    {
      for (int i = 0; i < nRanks; i++)
      {
        offsets[i] = nPiecesTotal * 7;
        nPiecesTotal += nPiecesAll[i];
        recvLengths[i] = nPiecesAll[i] * 7;
      }
    }
    int* sendBuffer = nullptr;
    int sendSize = nPieces * 7;
    if (nPieces > 0)
    {
      sendBuffer = new int[sendSize];
      ExtentsType::iterator iter = this->Extents.begin();
      for (int count = 0; iter != this->Extents.end(); ++iter, ++count)
      {
        sendBuffer[count * 7] = iter->first;
        memcpy(&sendBuffer[count * 7 + 1], &iter->second[0], 6 * sizeof(int));
      }
    }
    int* recvBuffer = nullptr;
    if (rank == 0)
    {
      recvBuffer = new int[nPiecesTotal * 7];
    }
    this->Controller->GatherV(sendBuffer, recvBuffer, sendSize, recvLengths, offsets, 0);

    if (rank == 0)
    {
      // Add all received values to Extents.
      // These are later written in WritePPieceAttributes()
      for (int i = 1; i < nRanks; i++)
      {
        for (int j = 0; j < nPiecesAll[i]; j++)
        {
          int* buffer = recvBuffer + offsets[i] + j * 7;
          this->Extents[*buffer] = std::vector<int>(buffer + 1, buffer + 7);
        }
      }
    }

    delete[] nPiecesAll;
    delete[] recvBuffer;
    delete[] offsets;
    delete[] recvLengths;
    delete[] sendBuffer;
  }
}

//----------------------------------------------------------------------------
int svtkXMLPStructuredDataWriter::WritePiece(int index)
{
  int result = this->Superclass::WritePiece(index);
  if (result)
  {
    // Store the extent of this piece in Extents. This is later used
    // in WritePPieceAttributes to write the summary file.
    svtkDataSet* input = this->GetInputAsDataSet();
    int* ext = input->GetInformation()->Get(svtkDataObject::DATA_EXTENT());
    this->Extents[index] = std::vector<int>(ext, ext + 6);
  }
  return result;
}
