/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLDataReader.h"

#include "svtkArrayIteratorIncludes.h"
#include "svtkCallbackCommand.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataArraySelection.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataParser.h"

#include <cassert>
#include <map> // needed for std::map

class svtkXMLDataReader::MapStringToInt : public std::map<std::string, int>
{
};
class svtkXMLDataReader::MapStringToInt64 : public std::map<std::string, svtkTypeInt64>
{
};

//----------------------------------------------------------------------------
svtkXMLDataReader::svtkXMLDataReader()
  : PointDataTimeStep(new svtkXMLDataReader::MapStringToInt())
  , PointDataOffset(new svtkXMLDataReader::MapStringToInt64())
  , CellDataTimeStep(new svtkXMLDataReader::MapStringToInt())
  , CellDataOffset(new svtkXMLDataReader::MapStringToInt64())
{
  this->NumberOfPieces = 0;
  this->PointDataElements = nullptr;
  this->CellDataElements = nullptr;
  this->Piece = 0;
  this->NumberOfPointArrays = 0;
  this->NumberOfCellArrays = 0;

  // Setup a callback for when the XMLParser's data reading routines
  // report progress.
  this->DataProgressObserver = svtkCallbackCommand::New();
  this->DataProgressObserver->SetCallback(&svtkXMLDataReader::DataProgressCallbackFunction);
  this->DataProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
svtkXMLDataReader::~svtkXMLDataReader()
{
  if (this->XMLParser)
  {
    this->DestroyXMLParser();
  }
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->DataProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::CreateXMLParser()
{
  this->Superclass::CreateXMLParser();
  this->XMLParser->AddObserver(svtkCommand::ProgressEvent, this->DataProgressObserver);
  if (this->GetParserErrorObserver())
  {
    this->XMLParser->AddObserver(svtkCommand::ErrorEvent, this->GetParserErrorObserver());
  }
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::DestroyXMLParser()
{
  if (this->XMLParser)
  {
    this->XMLParser->RemoveObserver(this->DataProgressObserver);
  }
  this->Superclass::DestroyXMLParser();
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void svtkXMLDataReader::SetupOutputInformation(svtkInformation* outInfo)
{
  if (this->InformationError)
  {
    svtkErrorMacro("Should not still be processing output information if have set InformationError");
    return;
  }

  // Initialize DataArraySelections to enable all that are present
  this->SetDataArraySelections(this->PointDataElements[0], this->PointDataArraySelection);
  this->SetDataArraySelections(this->CellDataElements[0], this->CellDataArraySelection);

  // Setup the Field Information for PointData.  We only need the
  // information from one piece because all pieces have the same set of arrays.
  svtkInformationVector* infoVector = nullptr;
  if (!this->SetFieldDataInfo(this->PointDataElements[0], svtkDataObject::FIELD_ASSOCIATION_POINTS,
        this->GetNumberOfPoints(), infoVector))
  {
    return;
  }
  if (infoVector)
  {
    outInfo->Set(svtkDataObject::POINT_DATA_VECTOR(), infoVector);
    infoVector->Delete();
  }

  // now the Cell data
  infoVector = nullptr;
  if (!this->SetFieldDataInfo(this->CellDataElements[0], svtkDataObject::FIELD_ASSOCIATION_CELLS,
        this->GetNumberOfCells(), infoVector))
  {
    return;
  }
  if (infoVector)
  {
    outInfo->Set(svtkDataObject::CELL_DATA_VECTOR(), infoVector);
    infoVector->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::CopyOutputInformation(svtkInformation* outInfo, int port)
{
  svtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);

  if (localInfo->Has(svtkDataObject::POINT_DATA_VECTOR()))
  {
    outInfo->CopyEntry(localInfo, svtkDataObject::POINT_DATA_VECTOR());
  }
  if (localInfo->Has(svtkDataObject::CELL_DATA_VECTOR()))
  {
    outInfo->CopyEntry(localInfo, svtkDataObject::CELL_DATA_VECTOR());
  }
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Count the number of pieces in the file.
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;

  for (int i = 0; i < numNested; ++i)
  {
    svtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      ++numPieces;
    }
  }

  // Now read each piece.  If no "Piece" elements were found, assume
  // the primary element itself is a single piece.
  if (numPieces)
  {
    this->SetupPieces(numPieces);
    int piece = 0;
    for (int i = 0; i < numNested; ++i)
    {
      svtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
      if (strcmp(eNested->GetName(), "Piece") == 0)
      {
        if (!this->ReadPiece(eNested, piece++))
        {
          return 0;
        }
      }
    }
  }
  else
  {
    this->SetupPieces(1);
    if (!this->ReadPiece(ePrimary, 0))
    {
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::SetupPieces(int numPieces)
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->NumberOfPieces = numPieces;
  if (numPieces > 0)
  {
    this->PointDataElements = new svtkXMLDataElement*[numPieces];
    this->CellDataElements = new svtkXMLDataElement*[numPieces];
  }
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    this->PointDataElements[i] = nullptr;
    this->CellDataElements[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::DestroyPieces()
{
  delete[] this->PointDataElements;
  delete[] this->CellDataElements;
  this->PointDataElements = nullptr;
  this->CellDataElements = nullptr;
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  svtkDataSet* output = svtkDataSet::SafeDownCast(this->GetCurrentOutput());
  svtkPointData* pointData = output->GetPointData();
  svtkCellData* cellData = output->GetCellData();

  // Get the size of the output arrays.
  svtkIdType pointTuples = this->GetNumberOfPoints();
  svtkIdType cellTuples = this->GetNumberOfCells();

  // Allocate the arrays in the output.  We only need the information
  // from one piece because all pieces have the same set of arrays.
  svtkXMLDataElement* ePointData = this->PointDataElements[0];
  svtkXMLDataElement* eCellData = this->CellDataElements[0];

  this->NumberOfPointArrays = 0;
  this->PointDataTimeStep->clear();
  this->PointDataOffset->clear();
  if (ePointData)
  {
    for (int i = 0; i < ePointData->GetNumberOfNestedElements(); i++)
    {
      svtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      const char* ename = eNested->GetAttribute("Name");
      if (this->PointDataArrayIsEnabled(eNested) && !pointData->HasArray(ename))
      {
        this->NumberOfPointArrays++;
        (*this->PointDataTimeStep)[ename] = -1;
        (*this->PointDataOffset)[ename] = -1;
        svtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
        {
          array->SetNumberOfTuples(pointTuples);
          pointData->AddArray(array);
          array->Delete();
        }
        else
        {
          this->DataError = 1;
        }
      }
    }
  }
  this->NumberOfCellArrays = 0;
  this->CellDataTimeStep->clear();
  this->CellDataOffset->clear();
  if (eCellData)
  {
    for (int i = 0; i < eCellData->GetNumberOfNestedElements(); i++)
    {
      svtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      const char* ename = eNested->GetAttribute("Name");
      if (this->CellDataArrayIsEnabled(eNested) && !cellData->HasArray(ename))
      {
        this->NumberOfCellArrays++;
        (*this->CellDataTimeStep)[ename] = -1;
        (*this->CellDataOffset)[ename] = -1;
        svtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
        {
          array->SetNumberOfTuples(cellTuples);
          cellData->AddArray(array);
          array->Delete();
        }
        else
        {
          this->DataError = 1;
        }
      }
    }
  }

  // Setup attribute indices for the point data and cell data.
  this->ReadAttributeIndices(ePointData, pointData);
  this->ReadAttributeIndices(eCellData, cellData);
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::ReadPiece(svtkXMLDataElement* ePiece, int piece)
{
  this->Piece = piece;
  return this->ReadPiece(ePiece);
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::ReadPiece(svtkXMLDataElement* ePiece)
{
  // Find the PointData and CellData in the piece.
  for (int i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    svtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "PointData") == 0)
    {
      this->PointDataElements[this->Piece] = eNested;
    }
    else if (strcmp(eNested->GetName(), "CellData") == 0)
    {
      this->CellDataElements[this->Piece] = eNested;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::ReadPieceData(int piece)
{
  this->Piece = piece;
  return this->ReadPieceData();
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::ReadPieceData()
{
  svtkDataSet* output = svtkDataSet::SafeDownCast(this->GetCurrentOutput());

  svtkPointData* pointData = output->GetPointData();
  svtkCellData* cellData = output->GetCellData();
  svtkXMLDataElement* ePointData = this->PointDataElements[this->Piece];
  svtkXMLDataElement* eCellData = this->CellDataElements[this->Piece];

  // Split current progress range over number of arrays.  This assumes
  // that each array contributes approximately the same amount of data
  // within this piece.
  float progressRange[2] = { 0.f, 0.f };
  int currentArray = 0;
  int numArrays = this->NumberOfPointArrays + this->NumberOfCellArrays;
  this->GetProgressRange(progressRange);

  // Read the data for this piece from each array.
  if (ePointData)
  {
    int a = 0;
    for (int i = 0; (i < ePointData->GetNumberOfNestedElements() && !this->AbortExecute); ++i)
    {
      svtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      if (this->PointDataArrayIsEnabled(eNested))
      {
        if (strcmp(eNested->GetName(), "DataArray") != 0 &&
          strcmp(eNested->GetName(), "Array") != 0)
        {
          svtkErrorMacro("Invalid Array.");
          this->DataError = 1;
          return 0;
        }
        int needToRead = this->PointDataNeedToReadTimeStep(eNested);
        if (needToRead)
        {
          // Set the range of progress for this array.
          this->SetProgressRange(progressRange, currentArray++, numArrays);

          // Read the array.
          svtkAbstractArray* array = pointData->GetAbstractArray(a++);
          if (array && !this->ReadArrayForPoints(eNested, array))
          {
            if (!this->AbortExecute)
            {
              svtkErrorMacro("Cannot read point data array \""
                << pointData->GetArray(a - 1)->GetName() << "\" from " << ePointData->GetName()
                << " in piece " << this->Piece
                << ".  The data array in the element may be too short.");
            }
            return 0;
          }
        }
      }
    }
  }
  if (eCellData)
  {
    int a = 0;
    for (int i = 0; (i < eCellData->GetNumberOfNestedElements() && !this->AbortExecute); ++i)
    {
      svtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      if (this->CellDataArrayIsEnabled(eNested))
      {
        if (strcmp(eNested->GetName(), "DataArray") != 0 &&
          strcmp(eNested->GetName(), "Array") != 0)
        {
          this->DataError = 1;
          svtkErrorMacro("Invalid Array");
          return 0;
        }
        int needToRead = this->CellDataNeedToReadTimeStep(eNested);
        if (needToRead)
        {
          // Set the range of progress for this array.
          this->SetProgressRange(progressRange, currentArray++, numArrays);

          // Read the array.
          if (!this->ReadArrayForCells(eNested, cellData->GetAbstractArray(a++)))
          {
            if (!this->AbortExecute)
            {
              svtkErrorMacro("Cannot read cell data array \""
                << cellData->GetAbstractArray(a - 1)->GetName() << "\" from "
                << ePointData->GetName() << " in piece " << this->Piece
                << ".  The data array in the element may be too short.");
            }
            return 0;
          }
        }
      }
    }
  }

  if (this->AbortExecute)
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::ReadXMLData()
{
  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();

  this->ReadFieldData();
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::ReadArrayForPoints(svtkXMLDataElement* da, svtkAbstractArray* outArray)
{
  svtkIdType components = outArray->GetNumberOfComponents();
  svtkIdType numberOfTuples = this->GetNumberOfPoints();
  return this->ReadArrayValues(da, 0, outArray, 0, numberOfTuples * components, POINT_DATA);
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::ReadArrayForCells(svtkXMLDataElement* da, svtkAbstractArray* outArray)
{
  svtkIdType components = outArray->GetNumberOfComponents();
  svtkIdType numberOfTuples = this->GetNumberOfCells();
  return this->ReadArrayValues(da, 0, outArray, 0, numberOfTuples * components, CELL_DATA);
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::ConvertGhostLevelsToGhostType(
  FieldType fieldType, svtkAbstractArray* data, svtkIdType startIndex, svtkIdType numValues)
{
  svtkUnsignedCharArray* ucData = svtkArrayDownCast<svtkUnsignedCharArray>(data);
  int numComp = data->GetNumberOfComponents();
  const char* name = data->GetName();
  if (this->GetFileMajorVersion() < 2 && ucData && numComp == 1 && name &&
    !strcmp(name, "svtkGhostLevels"))
  {
    // convert ghost levels to ghost type
    unsigned char* ghosts = ucData->GetPointer(0);
    // only CELL_DATA or POINT_DATA are possible at this point.
    unsigned char newValue = svtkDataSetAttributes::DUPLICATEPOINT;
    if (fieldType == CELL_DATA)
    {
      newValue = svtkDataSetAttributes::DUPLICATECELL;
    }
    for (int i = startIndex; i < numValues; ++i)
    {
      if (ghosts[i] > 0)
      {
        ghosts[i] = newValue;
      }
    }
    data->SetName(svtkDataSetAttributes::GhostArrayName());
  }
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::DataProgressCallbackFunction(
  svtkObject*, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<svtkXMLDataReader*>(clientdata)->DataProgressCallback();
}

//----------------------------------------------------------------------------
void svtkXMLDataReader::DataProgressCallback()
{
  if (this->InReadData)
  {
    float width = this->ProgressRange[1] - this->ProgressRange[0];
    float dataProgress = this->XMLParser->GetProgress();
    float progress = this->ProgressRange[0] + dataProgress * width;
    this->UpdateProgressDiscrete(progress);
    if (this->AbortExecute)
    {
      this->XMLParser->SetAbort(1);
    }
  }
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::PointDataNeedToReadTimeStep(svtkXMLDataElement* eNested)
{
  // First thing need to find the id of this dataarray from its name:
  const char* name = eNested->GetAttribute("Name");

  // Easy case no timestep:
  int numTimeSteps =
    eNested->GetVectorAttribute("TimeStep", this->NumberOfTimeSteps, this->TimeSteps);
  if (!(numTimeSteps <= this->NumberOfTimeSteps))
  {
    svtkErrorMacro("Invalid TimeStep specification");
    this->DataError = 1;
    return 0;
  }
  if (!numTimeSteps && !this->NumberOfTimeSteps)
  {
    assert(this->PointDataTimeStep->at(name) == -1); // No timestep in this file
    return 1;
  }
  // else TimeStep was specified but no TimeValues associated were found
  assert(this->NumberOfTimeSteps);

  // case numTimeSteps > 1
  int isCurrentTimeInArray =
    svtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
  if (numTimeSteps && !isCurrentTimeInArray)
  {
    return 0;
  }
  // we know that time steps are specified and that CurrentTimeStep is in the array
  // we need to figure out if we need to read the array or if it was forwarded
  // Need to check the current 'offset'
  svtkTypeInt64 offset;
  if (eNested->GetScalarAttribute("offset", offset))
  {
    if (this->PointDataOffset->at(name) != offset)
    {
      // save the pointsOffset
      assert(this->PointDataTimeStep->at(name) == -1); // cannot have mixture of binary and appended
      this->PointDataOffset->at(name) = offset;
      return 1;
    }
  }
  else
  {
    // No offset is specified this is a binary file
    // First thing to check if numTimeSteps == 0:
    if (!numTimeSteps && this->NumberOfTimeSteps && this->PointDataTimeStep->at(name) == -1)
    {
      // Update last PointsTimeStep read
      (*this->PointDataTimeStep)[name] = this->CurrentTimeStep;
      return 1;
    }
    int isLastTimeInArray = svtkXMLReader::IsTimeStepInArray(
      this->PointDataTimeStep->at(name), this->TimeSteps, numTimeSteps);
    // If no time is specified or if time is specified and match then read
    if (isCurrentTimeInArray && !isLastTimeInArray)
    {
      // CurrentTimeStep is in TimeSteps but Last is not := need to read
      // Update last PointsTimeStep read
      this->PointDataTimeStep->at(name) = this->CurrentTimeStep;
      return 1;
    }
  }
  // all other cases we don't need to read:
  return 0;
}

//----------------------------------------------------------------------------
int svtkXMLDataReader::CellDataNeedToReadTimeStep(svtkXMLDataElement* eNested)
{
  // First thing need to find the id of this dataarray from its name:
  const char* name = eNested->GetAttribute("Name");

  // Easy case no timestep:
  int numTimeSteps =
    eNested->GetVectorAttribute("TimeStep", this->NumberOfTimeSteps, this->TimeSteps);
  if (!(numTimeSteps <= this->NumberOfTimeSteps))
  {
    svtkErrorMacro("Invalid TimeSteps specification");
    this->DataError = 1;
    return 0;
  }
  if (!numTimeSteps && !this->NumberOfTimeSteps)
  {
    assert(this->CellDataTimeStep->at(name) == -1); // No timestep in this file
    return 1;
  }
  // else TimeStep was specified but no TimeValues associated were found
  assert(this->NumberOfTimeSteps);

  // case numTimeSteps > 1
  int isCurrentTimeInArray =
    svtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
  if (numTimeSteps && !isCurrentTimeInArray)
  {
    return 0;
  }
  // we know that time steps are specified and that CurrentTimeStep is in the array
  // we need to figure out if we need to read the array or if it was forwarded
  // Need to check the current 'offset'
  svtkTypeInt64 offset;
  if (eNested->GetScalarAttribute("offset", offset))
  {
    if (this->CellDataOffset->at(name) != offset)
    {
      // save the pointsOffset
      assert(this->CellDataTimeStep->at(name) == -1); // cannot have mixture of binary and appended
      this->CellDataOffset->at(name) = offset;
      return 1;
    }
  }
  else
  {
    // No offset is specified this is a binary file
    // First thing to check if numTimeSteps == 0:
    if (!numTimeSteps && this->NumberOfTimeSteps && this->CellDataTimeStep->at(name) == -1)
    {
      // Update last CellDataTimeStep read
      this->CellDataTimeStep->at(name) = this->CurrentTimeStep;
      return 1;
    }
    int isLastTimeInArray = svtkXMLReader::IsTimeStepInArray(
      this->CellDataTimeStep->at(name), this->TimeSteps, numTimeSteps);
    // If no time is specified or if time is specified and match then read
    if (isCurrentTimeInArray && !isLastTimeInArray)
    {
      // CurrentTimeStep is in TimeSteps but Last is not := need to read
      // Update last CellsTimeStep read
      this->CellDataTimeStep->at(name) = this->CurrentTimeStep;
      return 1;
    }
  }
  // all other cases we don't need to read:
  return 0;
}
