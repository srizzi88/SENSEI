/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStaticCleanPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStaticCleanPolyData.h"

#include "svtkArrayDispatch.h"
#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSMPTools.h"
#include "svtkStaticPointLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm>

svtkStandardNewMacro(svtkStaticCleanPolyData);

namespace { //anonymous

//----------------------------------------------------------------------------
// Fast, threaded way to copy new points and attribute data to output.
template <typename InArrayT, typename OutArrayT>
struct CopyPointsAlgorithm
{
  svtkIdType *PtMap;
  InArrayT *InPts;
  OutArrayT *OutPts;
  ArrayList Arrays;

  CopyPointsAlgorithm(svtkIdType *ptMap,
                      InArrayT *inPts,
                      svtkPointData *inPD,
                      svtkIdType numNewPts,
                      OutArrayT *outPts,
                      svtkPointData *outPD)
    : PtMap(ptMap),
      InPts(inPts),
      OutPts(outPts)
  {
    this->Arrays.AddArrays(numNewPts, inPD, outPD);
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    using OutValueT = svtk::GetAPIType<OutArrayT>;

    const svtkIdType *ptMap = this->PtMap;

    const auto inPoints = svtk::DataArrayTupleRange<3>(this->InPts);
    auto outPoints = svtk::DataArrayTupleRange<3>(this->OutPts);

    for (; ptId < endPtId; ++ptId)
    {
      const svtkIdType outPtId = ptMap[ptId];
      if ( outPtId != -1 )
      {
        const auto inP = inPoints[ptId];
        auto outP = outPoints[outPtId];
        outP[0] = static_cast<OutValueT>(inP[0]);
        outP[1] = static_cast<OutValueT>(inP[1]);
        outP[2] = static_cast<OutValueT>(inP[2]);
        this->Arrays.Copy(ptId, outPtId);
      }
    }
  }
};

struct CopyPointsLauncher
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(InArrayT *inPts,
                  OutArrayT *outPts,
                  svtkIdType *ptMap,
                  svtkPointData *inPD,
                  svtkIdType numNewPts,
                  svtkPointData *outPD)
  {
    const svtkIdType numPts = inPts->GetNumberOfTuples();

    CopyPointsAlgorithm<InArrayT, OutArrayT> algo {
      ptMap, inPts, inPD, numNewPts, outPts, outPD};

    svtkSMPTools::For(0, numPts, algo);
  }
};

} // anonymous namespace

//---------------------------------------------------------------------------
// Construct object with initial Tolerance of 0.0
svtkStaticCleanPolyData::svtkStaticCleanPolyData()
{
  this->ToleranceIsAbsolute = 0;
  this->Tolerance = 0.0;
  this->AbsoluteTolerance = 1.0;
  this->ConvertPolysToLines = 1;
  this->ConvertLinesToPoints = 1;
  this->ConvertStripsToPolys = 1;
  this->Locator = svtkStaticPointLocator::New();
  this->PieceInvariant = 1;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

//--------------------------------------------------------------------------
svtkStaticCleanPolyData::~svtkStaticCleanPolyData()
{
  this->Locator->Delete();
  this->Locator = nullptr;
}

//--------------------------------------------------------------------------
int svtkStaticCleanPolyData::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
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
int svtkStaticCleanPolyData::RequestData(svtkInformation* svtkNotUsed(request),
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

  // we'll be needing these
  svtkIdType inCellID, newId;
  svtkIdType i;
  svtkIdType ptId;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;

  svtkCellArray *inVerts = input->GetVerts(), *newVerts = nullptr;
  svtkCellArray *inLines = input->GetLines(), *newLines = nullptr;
  svtkCellArray *inPolys = input->GetPolys(), *newPolys = nullptr;
  svtkCellArray *inStrips = input->GetStrips(), *newStrips = nullptr;

  svtkPointData* inPD = input->GetPointData();
  svtkCellData* inCD = input->GetCellData();

  // The merge map indicates which points are merged with what points
  svtkIdType* mergeMap = new svtkIdType[numPts];
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();
  double tol =
    (this->ToleranceIsAbsolute ? this->AbsoluteTolerance : this->Tolerance * input->GetLength());
  this->Locator->MergePoints(tol, mergeMap);

  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD = output->GetCellData();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  // Prefix sum: count the number of new points; allocate memory. Populate the
  // point map (old points to new).
  svtkIdType* pointMap = new svtkIdType[numPts];
  svtkIdType id, numNewPts = 0;
  // Count and map points to new points
  for (id = 0; id < numPts; ++id)
  {
    if (mergeMap[id] == id)
    {
      pointMap[id] = numNewPts++;
    }
  }
  // Now map old merged points to new points
  for (id = 0; id < numPts; ++id)
  {
    if (mergeMap[id] != id)
    {
      pointMap[id] = pointMap[mergeMap[id]];
    }
  }
  delete[] mergeMap;

  svtkPoints* newPts = inPts->NewInstance();
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
  newPts->SetNumberOfPoints(numNewPts);

  svtkDataArray *inArray = inPts->GetData();
  svtkDataArray *outArray = newPts->GetData();

  // Use a fast path for when both arrays are some mix of float/double:
  using FastValueTypes = svtkArrayDispatch::Reals;
  using Dispatcher = svtkArrayDispatch::Dispatch2ByValueType<FastValueTypes,
                                                            FastValueTypes>;

  CopyPointsLauncher launcher;
  if (!Dispatcher::Execute(inArray, outArray, launcher, pointMap, inPD,
                           numNewPts, outPD))
  { // Fallback to slow path for unusual types:
    launcher(inArray, outArray, pointMap, inPD, numNewPts, outPD);
  }

  // Finally, remap the topology to use new point ids. Celldata needs to be
  // copied correctly. If a poly is converted to a line, or a line to a
  // point, then using a CellCounter will not do, as the cells should be
  // ordered verts, lines, polys, strips. We need to maintain separate cell
  // data lists so we can copy them all correctly. Tedious but easy to
  // implement. We can use outCD for vertex cell data, then add the rest
  // at the end.
  svtkCellData* outLineData = nullptr;
  svtkCellData* outPolyData = nullptr;
  svtkCellData* outStrpData = nullptr;
  svtkIdType vertIDcounter = 0, lineIDcounter = 0;
  svtkIdType polyIDcounter = 0, strpIDcounter = 0;

  // Begin to adjust topology.
  //
  // Vertices are renumbered and we remove duplicates
  svtkIdType numCellPts;
  inCellID = 0;
  if (!this->GetAbortExecute() && inVerts->GetNumberOfCells() > 0)
  {
    newVerts = svtkCellArray::New();
    newVerts->AllocateEstimate(inVerts->GetNumberOfCells(), 1);

    svtkDebugMacro(<< "Starting Verts " << inCellID);
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts, pts); inCellID++)
    {
      for (numCellPts = 0, i = 0; i < npts; i++)
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      } // for all points of vertex cell

      if (numCellPts > 0)
      {
        newId = newVerts->InsertNextCell(numCellPts, updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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
    outLineData->CopyAllocate(inCD);
    //
    svtkDebugMacro(<< "Starting Lines " << inCellID);
    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts); inCellID++)
    {
      for (numCellPts = 0, i = 0; i < npts; i++)
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      } // for all cell points

      if ((numCellPts > 1) || !this->ConvertLinesToPoints)
      {
        newId = newLines->InsertNextCell(numCellPts, updatedPts);
        outLineData->CopyData(inCD, inCellID, newId);
        if (lineIDcounter != newId)
        {
          svtkErrorMacro(<< "Line ID fault in line test");
        }
        lineIDcounter++;
      }
      else if (numCellPts == 1)
      {
        if (!newVerts)
        {
          newVerts = svtkCellArray::New();
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(numCellPts, updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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
    newPolys->AllocateCopy(inPolys);
    outPolyData = svtkCellData::New();
    outPolyData->CopyAllocate(inCD);

    svtkDebugMacro(<< "Starting Polys " << inCellID);
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts); inCellID++)
    {
      for (numCellPts = 0, i = 0; i < npts; i++)
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      } // for points in cell

      if (numCellPts > 2 && updatedPts[0] == updatedPts[numCellPts - 1])
      {
        numCellPts--;
      }
      if ((numCellPts > 2) || !this->ConvertPolysToLines)
      {
        newId = newPolys->InsertNextCell(numCellPts, updatedPts);
        outPolyData->CopyData(inCD, inCellID, newId);
        if (polyIDcounter != newId)
        {
          svtkErrorMacro(<< "Poly ID fault in poly test");
        }
        polyIDcounter++;
      }
      else if ((numCellPts == 2) || !this->ConvertLinesToPoints)
      {
        if (!newLines)
        {
          newLines = svtkCellArray::New();
          newLines->AllocateEstimate(5, 2);
          outLineData = svtkCellData::New();
          outLineData->CopyAllocate(inCD);
        }
        newId = newLines->InsertNextCell(numCellPts, updatedPts);
        outLineData->CopyData(inCD, inCellID, newId);
        if (lineIDcounter != newId)
        {
          svtkErrorMacro(<< "Line ID fault in poly test");
        }
        lineIDcounter++;
      }
      else if (numCellPts == 1)
      {
        if (!newVerts)
        {
          newVerts = svtkCellArray::New();
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(numCellPts, updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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
    newStrips->AllocateCopy(inStrips);
    outStrpData = svtkCellData::New();
    outStrpData->CopyAllocate(inCD);

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts); inCellID++)
    {
      for (numCellPts = 0, i = 0; i < npts; i++)
      {
        ptId = pointMap[pts[i]];
        updatedPts[numCellPts++] = ptId;
      }
      if ((numCellPts > 3) || !this->ConvertStripsToPolys)
      {
        newId = newStrips->InsertNextCell(numCellPts, updatedPts);
        outStrpData->CopyData(inCD, inCellID, newId);
        if (strpIDcounter != newId)
        {
          svtkErrorMacro(<< "Strip ID fault in strip test");
        }
        strpIDcounter++;
      }
      else if ((numCellPts == 3) || !this->ConvertPolysToLines)
      {
        if (!newPolys)
        {
          newPolys = svtkCellArray::New();
          newPolys->AllocateEstimate(5, 3);
          outPolyData = svtkCellData::New();
          outPolyData->CopyAllocate(inCD);
        }
        newId = newPolys->InsertNextCell(numCellPts, updatedPts);
        outPolyData->CopyData(inCD, inCellID, newId);
        if (polyIDcounter != newId)
        {
          svtkErrorMacro(<< "Poly ID fault in strip test");
        }
        polyIDcounter++;
      }
      else if ((numCellPts == 2) || !this->ConvertLinesToPoints)
      {
        if (!newLines)
        {
          newLines = svtkCellArray::New();
          newLines->AllocateEstimate(5, 2);
          outLineData = svtkCellData::New();
          outLineData->CopyAllocate(inCD);
        }
        newId = newLines->InsertNextCell(numCellPts, updatedPts);
        outLineData->CopyData(inCD, inCellID, newId);
        if (lineIDcounter != newId)
        {
          svtkErrorMacro(<< "Line ID fault in strip test");
        }
        lineIDcounter++;
      }
      else if (numCellPts == 1)
      {
        if (!newVerts)
        {
          newVerts = svtkCellArray::New();
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(numCellPts, updatedPts);
        outCD->CopyData(inCD, inCellID, newId);
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

  svtkDebugMacro(<< "Removed " << numNewPts - newPts->GetNumberOfPoints() << " points");

  // Update ourselves and release memory
  //
  this->Locator->Initialize(); // release memory.
  delete[] updatedPts;
  delete[] pointMap;

  // Now transfer all CellData from Lines/Polys/Strips into final
  // Cell data output
  svtkIdType combinedCellID = vertIDcounter;
  if (newLines)
  {
    for (i = 0; i < lineIDcounter; ++i, ++combinedCellID)
    {
      outCD->CopyData(outLineData, i, combinedCellID);
    }
    outLineData->Delete();
  }
  if (newPolys)
  {
    for (i = 0; i < polyIDcounter; ++i, ++combinedCellID)
    {
      outCD->CopyData(outPolyData, i, combinedCellID);
    }
    outPolyData->Delete();
  }
  if (newStrips)
  {
    for (i = 0; i < strpIDcounter; ++i, ++combinedCellID)
    {
      outCD->CopyData(outStrpData, i, combinedCellID);
    }
    outStrpData->Delete();
  }

  output->SetPoints(newPts);
  newPts->Delete();
  if (newVerts)
  {
    output->SetVerts(newVerts);
    newVerts->Delete();
  }
  if (newLines)
  {
    output->SetLines(newLines);
    newLines->Delete();
  }
  if (newPolys)
  {
    output->SetPolys(newPolys);
    newPolys->Delete();
  }
  if (newStrips)
  {
    output->SetStrips(newStrips);
    newStrips->Delete();
  }

  return 1;
}

//--------------------------------------------------------------------------
svtkMTimeType svtkStaticCleanPolyData::GetMTime()
{
  svtkMTimeType mTime = this->svtkObject::GetMTime();
  svtkMTimeType time = this->Locator->GetMTime();
  return (time > mTime ? time : mTime);
}

//--------------------------------------------------------------------------
void svtkStaticCleanPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

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
