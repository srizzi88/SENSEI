/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLStructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLStructuredDataWriter.h"

#include "svtkArrayIteratorIncludes.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataCompressor.h"
#include "svtkDataSet.h"
#include "svtkErrorCode.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude

//----------------------------------------------------------------------------
svtkXMLStructuredDataWriter::svtkXMLStructuredDataWriter()
{
  this->WritePiece = -1;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->WriteExtent[0] = 0;
  this->WriteExtent[1] = -1;
  this->WriteExtent[2] = 0;
  this->WriteExtent[3] = -1;
  this->WriteExtent[4] = 0;
  this->WriteExtent[5] = -1;

  this->CurrentPiece = 0;
  this->ProgressFractions = nullptr;
  this->FieldDataOM->Allocate(0);
  this->PointDataOM = new OffsetsManagerArray;
  this->CellDataOM = new OffsetsManagerArray;
}

//----------------------------------------------------------------------------
svtkXMLStructuredDataWriter::~svtkXMLStructuredDataWriter()
{
  delete[] this->ProgressFractions;
  delete this->PointDataOM;
  delete this->CellDataOM;
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WriteExtent: " << this->WriteExtent[0] << " " << this->WriteExtent[1] << "  "
     << this->WriteExtent[2] << " " << this->WriteExtent[3] << "  " << this->WriteExtent[4] << " "
     << this->WriteExtent[5] << "\n";
  os << indent << "NumberOfPieces" << this->NumberOfPieces << "\n";
  os << indent << "WritePiece: " << this->WritePiece << "\n";
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::SetInputUpdateExtent(int piece)
{
  svtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
  if ((this->WriteExtent[0] == 0) && (this->WriteExtent[1] == -1) && (this->WriteExtent[2] == 0) &&
    (this->WriteExtent[3] == -1) && (this->WriteExtent[4] == 0) && (this->WriteExtent[5] == -1))
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }
  else
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), this->WriteExtent, 6);
  }
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
}

svtkIdType svtkXMLStructuredDataWriter::GetNumberOfValues(svtkDataSet* input)
{
  svtkIdType dataSetValues = 0;
  svtkPointData* pointData = input->GetPointData();
  int pdArrays = pointData->GetNumberOfArrays();
  for (int i = 0; i < pdArrays; ++i)
  {
    svtkAbstractArray* array = pointData->GetAbstractArray(i);
    dataSetValues += array->GetNumberOfValues();
  }
  svtkCellData* cellData = input->GetCellData();
  int cdArrays = cellData->GetNumberOfArrays();
  for (int i = 0; i < cdArrays; ++i)
  {
    svtkAbstractArray* array = cellData->GetAbstractArray(i);
    dataSetValues += array->GetNumberOfValues();
  }
  return dataSetValues;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXMLStructuredDataWriter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    if (this->WritePiece >= 0)
    {
      this->CurrentPiece = this->WritePiece;
    }
    return 1;
  }

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    this->SetInputUpdateExtent(this->CurrentPiece);

    return 1;
  }

  // generate the data
  else if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->SetErrorCode(svtkErrorCode::NoError);

    if (!this->Stream && !this->FileName && !this->WriteToOutputString)
    {
      this->SetErrorCode(svtkErrorCode::NoFileNameError);
      svtkErrorMacro("The FileName or Stream must be set first or "
                    "the output must be written to a string.");
      return 0;
    }

    // We are just starting to write.  Do not call
    // UpdateProgressDiscrete because we want a 0 progress callback the
    // first time.
    this->UpdateProgress(0);
    this->SetProgressText("svtkXMLStructuredDataWriter");
    // Initialize progress range to entire 0..1 range.
    float wholeProgressRange[] = { 0.0f, 1.0f };
    svtkIdType fieldDataValues = 0;
    svtkFieldData* fieldData = this->GetInput()->GetFieldData();
    for (int i = 0; i < fieldData->GetNumberOfArrays(); ++i)
    {
      svtkAbstractArray* array = fieldData->GetAbstractArray(i);
      fieldDataValues += array->GetNumberOfValues();
    }
    svtkIdType dataSetValues = fieldDataValues + GetNumberOfValues(this->GetInputAsDataSet());
    if (dataSetValues == 0)
    {
      dataSetValues = 1;
    }
    float fraction[] = { 0.0f, static_cast<float>(fieldDataValues) / dataSetValues, 1.0f };
    this->SetProgressRange(wholeProgressRange, 0, fraction);
    int result = 1;
    if ((this->CurrentPiece == 0 || this->WritePiece >= 0) && this->CurrentTimeIndex == 0)
    {
      if (!this->OpenStream())
      {
        return 0;
      }
      if (this->GetInputAsDataSet() != nullptr &&
        (this->GetInputAsDataSet()->GetPointGhostArray() != nullptr ||
          this->GetInputAsDataSet()->GetCellGhostArray() != nullptr))
      {
        // use the current version for the file
        this->UsePreviousVersion = false;
      }
      // Write the file.
      if (!this->StartFile())
      {
        return 0;
      }

      if (!this->WriteHeader())
      {
        return 0;
      }

      this->CurrentTimeIndex = 0;
      if (this->DataMode == svtkXMLWriter::Appended && this->FieldDataOM->GetNumberOfElements())
      {
        svtkNew<svtkFieldData> fieldDataCopy;
        this->UpdateFieldData(fieldDataCopy);

        // Write the field data arrays.
        this->WriteFieldDataAppendedData(fieldDataCopy, this->CurrentTimeIndex, this->FieldDataOM);
        if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
      }
    }

    if (!(this->UserContinueExecuting == 0)) // if user ask to stop do not try to write a piece
    {
      this->SetProgressRange(wholeProgressRange, 1, fraction);
      result = this->WriteAPiece();
    }

    if (this->WritePiece < 0)
    {
      // Tell the pipeline to start looping.
      if (this->CurrentPiece == 0)
      {
        request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
      this->CurrentPiece++;
    }

    if (this->CurrentPiece == this->NumberOfPieces || this->WritePiece >= 0)
    {
      request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentPiece = 0;
      // We are done writing all the pieces, lets loop over time now:
      this->CurrentTimeIndex++;

      if (this->UserContinueExecuting != 1)
      {
        if (!this->WriteFooter())
        {
          return 0;
        }

        if (!this->EndFile())
        {
          return 0;
        }

        this->CloseStream();
        this->CurrentTimeIndex = 0; // Reset
      }
    }

    // We have finished writing.
    this->UpdateProgressDiscrete(1);
    return result;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::AllocatePositionArrays()
{
  this->ExtentPositions = new svtkTypeInt64[this->NumberOfPieces];

  // Prepare storage for the point and cell data array appended data
  // offsets for each piece.
  this->PointDataOM->Allocate(this->NumberOfPieces);
  this->CellDataOM->Allocate(this->NumberOfPieces);
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::DeletePositionArrays()
{
  delete[] this->ExtentPositions;
  this->ExtentPositions = nullptr;
}

//----------------------------------------------------------------------------
int svtkXMLStructuredDataWriter::WriteHeader()
{
  svtkIndent indent = svtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (!this->WritePrimaryElement(os, indent))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if (this->DataMode == svtkXMLWriter::Appended)
  {
    int begin = this->WritePiece;
    int end = this->WritePiece + 1;
    if (this->WritePiece < 0)
    {
      begin = 0;
      end = this->NumberOfPieces;
    }
    svtkIndent nextIndent = indent.GetNextIndent();

    this->AllocatePositionArrays();

    // Loop over each piece and write its structure.
    for (int i = begin; i < end; ++i)
    {
      // Update the piece's extent.

      os << nextIndent << "<Piece";
      // We allocate 66 characters because that is as big as 6 integers
      // with spaces can get.
      this->ExtentPositions[i] = this->ReserveAttributeSpace("Extent", 66);
      os << ">\n";

      if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }

      this->WriteAppendedPiece(i, nextIndent.GetNextIndent());

      if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }
      // Close the piece's element.
      os << nextIndent << "</Piece>\n";
    }

    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";

    os.flush();
    if (os.fail())
    {
      this->DeletePositionArrays();
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      return 0;
    }

    this->StartAppendedData();
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      this->DeletePositionArrays();
      return 0;
    }
  }

  // Split progress of the data write by the fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  this->ProgressFractions = new float[this->NumberOfPieces + 1];
  this->CalculatePieceFractions(this->ProgressFractions);

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLStructuredDataWriter::WriteAPiece()
{
  svtkIndent indent = svtkIndent().GetNextIndent();
  int result = 1;

  if (this->DataMode == svtkXMLWriter::Appended)
  {
    svtkDataSet* input = this->GetInputAsDataSet();

    // Make sure input is valid.
    if (input->CheckAttributes() == 0)
    {
      this->WriteAppendedPieceData(this->CurrentPiece);

      if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }
    }
    else
    {
      svtkErrorMacro("Input is invalid for piece " << this->CurrentPiece << ".  Aborting.");
      result = 0;
    }
  }
  else
  {
    this->WriteInlineMode(indent);
  }

  return result;
}

//----------------------------------------------------------------------------
int svtkXMLStructuredDataWriter::WriteFooter()
{
  svtkIndent indent = svtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (this->DataMode == svtkXMLWriter::Appended)
  {
    this->DeletePositionArrays();
    this->EndAppendedData();
  }
  else
  {
    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";

    os.flush();
    if (os.fail())
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    }
  }

  delete[] this->ProgressFractions;
  this->ProgressFractions = nullptr;

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLStructuredDataWriter::WriteInlineMode(svtkIndent indent)
{
  svtkDataSet* input = this->GetInputAsDataSet();
  ostream& os = *(this->Stream);

  int* extent = input->GetInformation()->Get(svtkDataObject::DATA_EXTENT());

  // Split progress of the data write by the fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Write each piece's XML and data.
  int result = 1;

  // Set the progress range for this piece.
  this->SetProgressRange(progressRange, this->CurrentPiece, this->ProgressFractions);

  // Make sure input is valid.
  if (input->CheckAttributes() == 0)
  {
    os << indent << "<Piece";
    this->WriteVectorAttribute("Extent", 6, extent);
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      return 0;
    }

    os << ">\n";

    this->WriteInlinePiece(indent.GetNextIndent());
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      return 0;
    }
    os << indent << "</Piece>\n";
  }
  else
  {
    svtkErrorMacro("Input is invalid for piece " << this->CurrentPiece << ".  Aborting.");
    result = 0;
  }

  return result;
}

//----------------------------------------------------------------------------
template <class iterT>
inline void svtkXMLStructuredDataWriterCopyTuples(
  iterT* destIter, svtkIdType destTuple, iterT* srcIter, svtkIdType sourceTuple, svtkIdType numTuples)
{
  // for all contiguous-fixed component size arrays (except Bit).
  int tupleSize = (srcIter->GetDataTypeSize() * srcIter->GetNumberOfComponents());

  memcpy(destIter->GetTuple(destTuple), srcIter->GetTuple(sourceTuple), numTuples * tupleSize);
}

//----------------------------------------------------------------------------
inline void svtkXMLStructuredDataWriterCopyTuples(svtkArrayIteratorTemplate<svtkStdString>* destIter,
  svtkIdType destTuple, svtkArrayIteratorTemplate<svtkStdString>* srcIter, svtkIdType sourceTuple,
  svtkIdType numTuples)
{
  svtkIdType numValues = numTuples * srcIter->GetNumberOfComponents();
  svtkIdType destIndex = destTuple * destIter->GetNumberOfComponents();
  svtkIdType srcIndex = sourceTuple * srcIter->GetNumberOfComponents();

  for (svtkIdType cc = 0; cc < numValues; cc++)
  {
    destIter->GetValue(destIndex++) = srcIter->GetValue(srcIndex++);
  }
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::WritePrimaryElementAttributes(ostream& os, svtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);

  int* ext = this->WriteExtent;
  if ((this->WriteExtent[0] == 0) && (this->WriteExtent[1] == -1) && (this->WriteExtent[2] == 0) &&
    (this->WriteExtent[3] == -1) && (this->WriteExtent[4] == 0) && (this->WriteExtent[5] == -1))
  {
    ext = this->GetInputInformation(0, 0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  }

  if (this->WritePiece >= 0)
  {
    svtkDataSet* input = this->GetInputAsDataSet();
    ext = input->GetInformation()->Get(svtkDataObject::DATA_EXTENT());
  }

  this->WriteVectorAttribute("WholeExtent", 6, ext);
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::WriteAppendedPiece(int index, svtkIndent indent)
{
  // Write the point data and cell data arrays.
  svtkDataSet* input = this->GetInputAsDataSet();
  this->WritePointDataAppended(input->GetPointData(), indent, &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WriteCellDataAppended(input->GetCellData(), indent, &this->CellDataOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::WriteAppendedPieceData(int index)
{
  // Write the point data and cell data arrays.
  svtkDataSet* input = this->GetInputAsDataSet();

  int* ext = input->GetInformation()->Get(svtkDataObject::DATA_EXTENT());

  ostream& os = *(this->Stream);

  std::streampos returnPosition = os.tellp();
  os.seekp(std::streampos(this->ExtentPositions[index]));
  this->WriteVectorAttribute("Extent", 6, ext);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  os.seekp(returnPosition);

  // Split progress between point data and cell data arrays.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  int total = (pdArrays + cdArrays) ? (pdArrays + cdArrays) : 1;
  float fractions[3] = { 0, static_cast<float>(pdArrays) / total, 1 };

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);
  this->WritePointDataAppendedData(
    input->GetPointData(), this->CurrentTimeIndex, &this->PointDataOM->GetPiece(index));
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  this->WriteCellDataAppendedData(
    input->GetCellData(), this->CurrentTimeIndex, &this->CellDataOM->GetPiece(index));
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::WriteInlinePiece(svtkIndent indent)
{
  // Write the point data and cell data arrays.
  svtkDataSet* input = this->GetInputAsDataSet();

  // Split progress between point data and cell data arrays.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  int total = (pdArrays + cdArrays) ? (pdArrays + cdArrays) : 1;
  float fractions[3] = { 0, float(pdArrays) / total, 1 };

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);
  this->WritePointDataInline(input->GetPointData(), indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  this->WriteCellDataInline(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLStructuredDataWriter::GetStartTuple(
  int* extent, svtkIdType* increments, int i, int j, int k)
{
  return (((i - extent[0]) * increments[0]) + ((j - extent[2]) * increments[1]) +
    ((k - extent[4]) * increments[2]));
}

//----------------------------------------------------------------------------
void svtkXMLStructuredDataWriter::CalculatePieceFractions(float* fractions)
{
  // Calculate the fraction of total data contributed by each piece.
  fractions[0] = 0;
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    int extent[6];
    this->GetInputExtent(extent);

    // Add this piece's size to the cumulative fractions array.
    fractions[i + 1] = fractions[i] +
      ((extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1) * (extent[5] - extent[4] + 1));
  }
  if (fractions[this->NumberOfPieces] == 0)
  {
    fractions[this->NumberOfPieces] = 1;
  }
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    fractions[i + 1] = fractions[i + 1] / fractions[this->NumberOfPieces];
  }
}
