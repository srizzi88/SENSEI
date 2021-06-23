/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCleanPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCleanPolyData.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkCleanPolyData);

//---------------------------------------------------------------------------
// Specify a spatial locator for speeding the search process. By
// default an instance of svtkPointLocator is used.
svtkCxxSetObjectMacro(svtkCleanPolyData, Locator, svtkIncrementalPointLocator);

//---------------------------------------------------------------------------
// Construct object with initial Tolerance of 0.0
svtkCleanPolyData::svtkCleanPolyData()
{
  this->PointMerging = 1;
  this->ToleranceIsAbsolute = 0;
  this->Tolerance = 0.0;
  this->AbsoluteTolerance = 1.0;
  this->ConvertPolysToLines = 1;
  this->ConvertLinesToPoints = 1;
  this->ConvertStripsToPolys = 1;
  this->Locator = nullptr;
  this->PieceInvariant = 1;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

//--------------------------------------------------------------------------
svtkCleanPolyData::~svtkCleanPolyData()
{
  this->SetLocator(nullptr);
}

//--------------------------------------------------------------------------
void svtkCleanPolyData::OperateOnPoint(double in[3], double out[3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}

//--------------------------------------------------------------------------
void svtkCleanPolyData::OperateOnBounds(double in[6], double out[6])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
  out[3] = in[3];
  out[4] = in[4];
  out[5] = in[5];
}

//--------------------------------------------------------------------------
int svtkCleanPolyData::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->PieceInvariant)
  {
    // Although piece > 1 is handled by superclass, we should be thorough.
    if (outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) == 0)
    {
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    }
    else
    {
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 0);
    }
  }
  else
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }

  return 1;
}

//--------------------------------------------------------------------------
int svtkCleanPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* inPts = input->GetPoints();
  svtkIdType numPts = input->GetNumberOfPoints();

  svtkDebugMacro(<< "Beginning PolyData clean");
  if ((numPts < 1) || (inPts == nullptr))
  {
    svtkDebugMacro(<< "No data to Operate On!");
    return 1;
  }
  svtkIdType* updatedPts = new svtkIdType[input->GetMaxCellSize()];

  svtkIdType numNewPts;
  svtkIdType numUsedPts = 0;
  svtkPoints* newPts = inPts->NewInstance();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(numPts);

  // we'll be needing these
  svtkIdType inCellID, newId;
  svtkIdType i;
  svtkIdType ptId;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  double x[3];
  double newx[3];
  svtkIdType* pointMap = nullptr; // used if no merging

  svtkCellArray *inVerts = input->GetVerts(), *newVerts = nullptr;
  svtkCellArray *inLines = input->GetLines(), *newLines = nullptr;
  svtkCellArray *inPolys = input->GetPolys(), *newPolys = nullptr;
  svtkCellArray *inStrips = input->GetStrips(), *newStrips = nullptr;

  svtkPointData* inputPD = input->GetPointData();
  svtkCellData* inputCD = input->GetCellData();

  // We must be careful to 'operate' on the bounds of the locator so
  // that all inserted points lie inside it
  if (this->PointMerging)
  {
    this->CreateDefaultLocator(input);
    if (this->ToleranceIsAbsolute)
    {
      this->Locator->SetTolerance(this->AbsoluteTolerance);
    }
    else
    {
      this->Locator->SetTolerance(this->Tolerance * input->GetLength());
    }
    double originalbounds[6], mappedbounds[6];
    input->GetBounds(originalbounds);
    this->OperateOnBounds(originalbounds, mappedbounds);
    this->Locator->InitPointInsertion(newPts, mappedbounds);
  }
  else
  {
    pointMap = new svtkIdType[numPts];
    for (i = 0; i < numPts; ++i)
    {
      pointMap[i] = -1; // initialize unused
    }
  }

  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  if (!this->PointMerging)
  {
    outputPD->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
  }
  outputPD->CopyAllocate(inputPD);
  outputCD->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
  outputCD->CopyAllocate(inputCD);

  // Celldata needs to be copied correctly. If a poly is converted to
  // a line, or a line to a point, then using a CellCounter will not
  // do, as the cells should be ordered verts, lines, polys,
  // strips. We need to maintain separate cell data lists so we can
  // copy them all correctly. Tedious but easy to implement. We can
  // use outputCD for vertex cell data, then add the rest at the end.
  svtkCellData* outLineData = nullptr;
  svtkCellData* outPolyData = nullptr;
  svtkCellData* outStrpData = nullptr;
  svtkIdType vertIDcounter = 0, lineIDcounter = 0;
  svtkIdType polyIDcounter = 0, strpIDcounter = 0;

  // Begin to adjust topology.
  //
  // Vertices are renumbered and we remove duplicates
  inCellID = 0;
  if (!this->GetAbortExecute() && inVerts->GetNumberOfCells() > 0)
  {
    newVerts = svtkCellArray::New();
    newVerts->AllocateEstimate(inVerts->GetNumberOfCells(), 1);

    svtkDebugMacro(<< "Starting Verts " << inCellID);
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts, pts); inCellID++)
    {
      for (numNewPts = 0, i = 0; i < npts; ++i)
      {
        inPts->GetPoint(pts[i], x);
        this->OperateOnPoint(x, newx);
        if (!this->PointMerging)
        {
          if ((ptId = pointMap[pts[i]]) == -1)
          {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId, newx);
            outputPD->CopyData(inputPD, pts[i], ptId);
          }
        }
        else if (this->Locator->InsertUniquePoint(newx, ptId))
        {
          outputPD->CopyData(inputPD, pts[i], ptId);
        }
        updatedPts[numNewPts++] = ptId;
      } // for all points of vertex cell

      if (numNewPts > 0)
      {
        newId = newVerts->InsertNextCell(numNewPts, updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter != newId)
        {
          svtkErrorMacro(<< "Vertex ID fault in vertex test");
        }
        vertIDcounter++;
      }
    }
  }
  this->UpdateProgress(0.25);

  // lines reduced to one point are eliminated or made into verts
  if (!this->GetAbortExecute() && inLines->GetNumberOfCells() > 0)
  {
    newLines = svtkCellArray::New();
    newLines->AllocateEstimate(inLines->GetNumberOfCells(), 2);
    outLineData = svtkCellData::New();
    outLineData->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
    outLineData->CopyAllocate(inputCD);
    //
    svtkDebugMacro(<< "Starting Lines " << inCellID);
    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts); inCellID++)
    {
      for (numNewPts = 0, i = 0; i < npts; i++)
      {
        inPts->GetPoint(pts[i], x);
        this->OperateOnPoint(x, newx);
        if (!this->PointMerging)
        {
          if ((ptId = pointMap[pts[i]]) == -1)
          {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId, newx);
            outputPD->CopyData(inputPD, pts[i], ptId);
          }
        }
        else if (this->Locator->InsertUniquePoint(newx, ptId))
        {
          outputPD->CopyData(inputPD, pts[i], ptId);
        }
        if (i == 0 || ptId != updatedPts[numNewPts - 1])
        {
          updatedPts[numNewPts++] = ptId;
        }
      } // for all cell points

      if (numNewPts >= 2)
      {
        // Cell is a proper line or polyline, always add
        newId = newLines->InsertNextCell(numNewPts, updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter != newId)
        {
          svtkErrorMacro(<< "Line ID fault in line test");
        }
        lineIDcounter++;
      }
      else if (numNewPts == 1 && (npts == numNewPts || this->ConvertLinesToPoints))
      {
        // Cell was either a vertex to begin with and we didn't modify it or a degenerated line and
        // the user wanted it included as a vertex
        if (!newVerts)
        {
          newVerts = svtkCellArray::New();
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(numNewPts, updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter != newId)
        {
          svtkErrorMacro(<< "Vertex ID fault in line test");
        }
        vertIDcounter++;
      }
    }
    svtkDebugMacro(<< "Removed " << inLines->GetNumberOfCells() - newLines->GetNumberOfCells()
                  << " lines");
  }
  this->UpdateProgress(0.50);

  // polygons reduced to two points or less are either eliminated
  // or converted to lines or points if enabled
  if (!this->GetAbortExecute() && inPolys->GetNumberOfCells() > 0)
  {
    newPolys = svtkCellArray::New();
    newPolys->AllocateExact(inPolys->GetNumberOfCells(), inPolys->GetNumberOfConnectivityIds());
    outPolyData = svtkCellData::New();
    outPolyData->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
    outPolyData->CopyAllocate(inputCD);

    svtkDebugMacro(<< "Starting Polys " << inCellID);
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts); inCellID++)
    {
      for (numNewPts = 0, i = 0; i < npts; i++)
      {
        inPts->GetPoint(pts[i], x);
        this->OperateOnPoint(x, newx);
        if (!this->PointMerging)
        {
          if ((ptId = pointMap[pts[i]]) == -1)
          {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId, newx);
            outputPD->CopyData(inputPD, pts[i], ptId);
          }
        }
        else if (this->Locator->InsertUniquePoint(newx, ptId))
        {
          outputPD->CopyData(inputPD, pts[i], ptId);
        }
        if (i == 0 || ptId != updatedPts[numNewPts - 1])
        {
          updatedPts[numNewPts++] = ptId;
        }
      } // for points in cell
      if (numNewPts > 2 && updatedPts[0] == updatedPts[numNewPts - 1])
      {
        numNewPts--;
      }
      if (numNewPts > 2)
      {
        // Cell is a proper polygon, always add
        newId = newPolys->InsertNextCell(numNewPts, updatedPts);
        outPolyData->CopyData(inputCD, inCellID, newId);
        if (polyIDcounter != newId)
        {
          svtkErrorMacro(<< "Poly ID fault in poly test");
        }
        polyIDcounter++;
      }
      else if (numNewPts == 2 && (npts == numNewPts || this->ConvertPolysToLines))
      {
        // Cell was either a line to begin with and we didn't modify it or a degenerated poly and
        // the user wanted it included as a line
        if (!newLines)
        {
          newLines = svtkCellArray::New();
          newLines->AllocateEstimate(5, 2);
          outLineData = svtkCellData::New();
          outLineData->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
          outLineData->CopyAllocate(inputCD);
        }
        newId = newLines->InsertNextCell(numNewPts, updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter != newId)
        {
          svtkErrorMacro(<< "Line ID fault in poly test");
        }
        lineIDcounter++;
      }
      else if (numNewPts == 1 && (npts == numNewPts || this->ConvertLinesToPoints))
      {
        // Cell was either a vertex to begin with and we didn't modify it or a degenerated line and
        // the user wanted it included as a vertex
        if (!newVerts)
        {
          newVerts = svtkCellArray::New();
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(numNewPts, updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter != newId)
        {
          svtkErrorMacro(<< "Vertex ID fault in poly test");
        }
        vertIDcounter++;
      }
    }
    svtkDebugMacro(<< "Removed " << inPolys->GetNumberOfCells() - newPolys->GetNumberOfCells()
                  << " polys");
  }
  this->UpdateProgress(0.75);

  // triangle strips can reduced to polys/lines/points etc
  if (!this->GetAbortExecute() && inStrips->GetNumberOfCells() > 0)
  {
    newStrips = svtkCellArray::New();
    newStrips->AllocateExact(inStrips->GetNumberOfCells(), inStrips->GetNumberOfConnectivityIds());
    outStrpData = svtkCellData::New();
    outStrpData->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
    outStrpData->CopyAllocate(inputCD);

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts); inCellID++)
    {
      for (numNewPts = 0, i = 0; i < npts; i++)
      {
        inPts->GetPoint(pts[i], x);
        this->OperateOnPoint(x, newx);
        if (!this->PointMerging)
        {
          if ((ptId = pointMap[pts[i]]) == -1)
          {
            pointMap[pts[i]] = ptId = numUsedPts++;
            newPts->SetPoint(ptId, newx);
            outputPD->CopyData(inputPD, pts[i], ptId);
          }
        }
        else if (this->Locator->InsertUniquePoint(newx, ptId))
        {
          outputPD->CopyData(inputPD, pts[i], ptId);
        }
        if (i == 0 || ptId != updatedPts[numNewPts - 1])
        {
          updatedPts[numNewPts++] = ptId;
        }
      }
      if (numNewPts > 1 && updatedPts[0] == updatedPts[numNewPts - 1])
      {
        numNewPts--;
      }
      if (numNewPts > 3)
      {
        // Cell is a proper triangle strip, always add
        newId = newStrips->InsertNextCell(numNewPts, updatedPts);
        outStrpData->CopyData(inputCD, inCellID, newId);
        if (strpIDcounter != newId)
        {
          svtkErrorMacro(<< "Strip ID fault in strip test");
        }
        strpIDcounter++;
      }
      else if (numNewPts == 3 && (npts == numNewPts || this->ConvertStripsToPolys))
      {
        // Cell was either a triangle to begin with and we didn't modify it or a degenerated
        // triangle strip and the user wanted it included as a polygon
        if (!newPolys)
        {
          newPolys = svtkCellArray::New();
          newPolys->AllocateEstimate(5, 3);
          outPolyData = svtkCellData::New();
          outPolyData->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
          outPolyData->CopyAllocate(inputCD);
        }
        newId = newPolys->InsertNextCell(numNewPts, updatedPts);
        outPolyData->CopyData(inputCD, inCellID, newId);
        if (polyIDcounter != newId)
        {
          svtkErrorMacro(<< "Poly ID fault in strip test");
        }
        polyIDcounter++;
      }
      else if (numNewPts == 2 && (npts == numNewPts || this->ConvertPolysToLines))
      {
        // Cell was either a line to begin with and we didn't modify it or a degenerated triangle
        // strip and the user wanted it included as a line
        if (!newLines)
        {
          newLines = svtkCellArray::New();
          newLines->AllocateEstimate(5, 2);
          outLineData = svtkCellData::New();
          outLineData->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
          outLineData->CopyAllocate(inputCD);
        }
        newId = newLines->InsertNextCell(numNewPts, updatedPts);
        outLineData->CopyData(inputCD, inCellID, newId);
        if (lineIDcounter != newId)
        {
          svtkErrorMacro(<< "Line ID fault in strip test");
        }
        lineIDcounter++;
      }
      else if (numNewPts == 1 && (npts == numNewPts || this->ConvertLinesToPoints))
      {
        // Cell was either a vertex to begin with and we didn't modify it or a degenerated triangle
        // strip and the user wanted it included as a vertex
        if (!newVerts)
        {
          newVerts = svtkCellArray::New();
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(numNewPts, updatedPts);
        outputCD->CopyData(inputCD, inCellID, newId);
        if (vertIDcounter != newId)
        {
          svtkErrorMacro(<< "Vertex ID fault in strip test");
        }
        vertIDcounter++;
      }
    }
    svtkDebugMacro(<< "Removed " << inStrips->GetNumberOfCells() - newStrips->GetNumberOfCells()
                  << " strips");
  }

  svtkDebugMacro(<< "Removed " << numPts - newPts->GetNumberOfPoints() << " points");

  // Update ourselves and release memory
  //
  delete[] updatedPts;
  if (this->PointMerging)
  {
    this->Locator->Initialize(); // release memory.
  }
  else
  {
    newPts->SetNumberOfPoints(numUsedPts);
    delete[] pointMap;
  }

  // Now transfer all CellData from Lines/Polys/Strips into final
  // Cell data output
  svtkIdType combinedCellID = vertIDcounter;
  if (newLines)
  {
    for (i = 0; i < lineIDcounter; ++i, ++combinedCellID)
    {
      outputCD->CopyData(outLineData, i, combinedCellID);
    }
    outLineData->Delete();
  }
  if (newPolys)
  {
    for (i = 0; i < polyIDcounter; ++i, ++combinedCellID)
    {
      outputCD->CopyData(outPolyData, i, combinedCellID);
    }
    outPolyData->Delete();
  }
  if (newStrips)
  {
    for (i = 0; i < strpIDcounter; ++i, ++combinedCellID)
    {
      outputCD->CopyData(outStrpData, i, combinedCellID);
    }
    outStrpData->Delete();
  }

  output->SetPoints(newPts);
  newPts->Squeeze();
  newPts->Delete();
  if (newVerts)
  {
    newVerts->Squeeze();
    output->SetVerts(newVerts);
    newVerts->Delete();
  }
  if (newLines)
  {
    newLines->Squeeze();
    output->SetLines(newLines);
    newLines->Delete();
  }
  if (newPolys)
  {
    newPolys->Squeeze();
    output->SetPolys(newPolys);
    newPolys->Delete();
  }
  if (newStrips)
  {
    newStrips->Squeeze();
    output->SetStrips(newStrips);
    newStrips->Delete();
  }

  return 1;
}

//--------------------------------------------------------------------------
// Method manages creation of locators. It takes into account the potential
// change of tolerance (zero to non-zero).
void svtkCleanPolyData::CreateDefaultLocator(svtkPolyData* input)
{
  double tol;
  if (this->ToleranceIsAbsolute)
  {
    tol = this->AbsoluteTolerance;
  }
  else
  {
    if (input)
    {
      tol = this->Tolerance * input->GetLength();
    }
    else
    {
      tol = this->Tolerance;
    }
  }

  if (this->Locator == nullptr)
  {
    if (tol == 0.0)
    {
      this->Locator = svtkMergePoints::New();
      this->Locator->Register(this);
      this->Locator->Delete();
    }
    else
    {
      this->Locator = svtkPointLocator::New();
      this->Locator->Register(this);
      this->Locator->Delete();
    }
  }
  else
  {
    // check that the tolerance wasn't changed from zero to non-zero
    if ((tol > 0.0) && (this->GetLocator()->GetTolerance() == 0.0))
    {
      this->SetLocator(nullptr);
      this->Locator = svtkPointLocator::New();
      this->Locator->Register(this);
      this->Locator->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void svtkCleanPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Merging: " << (this->PointMerging ? "On\n" : "Off\n");
  os << indent << "ToleranceIsAbsolute: " << (this->ToleranceIsAbsolute ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << (this->Tolerance ? "On\n" : "Off\n");
  os << indent << "AbsoluteTolerance: " << (this->AbsoluteTolerance ? "On\n" : "Off\n");
  os << indent << "ConvertPolysToLines: " << (this->ConvertPolysToLines ? "On\n" : "Off\n");
  os << indent << "ConvertLinesToPoints: " << (this->ConvertLinesToPoints ? "On\n" : "Off\n");
  os << indent << "ConvertStripsToPolys: " << (this->ConvertStripsToPolys ? "On\n" : "Off\n");
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
  os << indent << "PieceInvariant: " << (this->PieceInvariant ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}

//--------------------------------------------------------------------------
svtkMTimeType svtkCleanPolyData::GetMTime()
{
  svtkMTimeType mTime = this->svtkObject::GetMTime();
  svtkMTimeType time;
  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}
