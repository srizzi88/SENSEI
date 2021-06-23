/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProbeFilter.h"

#include "svtkAbstractCellLocator.h"
#include "svtkBoundingBox.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellLocatorStrategy.h"
#include "svtkCharArray.h"
#include "svtkFindCellStrategy.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSMPThreadLocal.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <vector>

svtkStandardNewMacro(svtkProbeFilter);
svtkCxxSetObjectMacro(svtkProbeFilter, CellLocatorPrototype, svtkAbstractCellLocator);
svtkCxxSetObjectMacro(svtkProbeFilter, FindCellStrategy, svtkFindCellStrategy);

#define CELL_TOLERANCE_FACTOR_SQR 1e-6

class svtkProbeFilter::svtkVectorOfArrays : public std::vector<svtkDataArray*>
{
};

//----------------------------------------------------------------------------
svtkProbeFilter::svtkProbeFilter()
{
  this->CategoricalData = 0;
  this->SpatialMatch = 0;
  this->ValidPoints = svtkIdTypeArray::New();
  this->MaskPoints = nullptr;
  this->SetNumberOfInputPorts(2);
  this->ValidPointMaskArrayName = nullptr;
  this->SetValidPointMaskArrayName("svtkValidPointMask");
  this->CellArrays = new svtkVectorOfArrays();

  this->CellLocatorPrototype = nullptr;
  this->FindCellStrategy = nullptr;

  this->PointList = nullptr;
  this->CellList = nullptr;

  this->PassCellArrays = 0;
  this->PassPointArrays = 0;
  this->PassFieldArrays = 1;
  this->Tolerance = 1.0;
  this->ComputeTolerance = 1;
}

//----------------------------------------------------------------------------
svtkProbeFilter::~svtkProbeFilter()
{
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
  }
  this->ValidPoints->Delete();

  this->SetValidPointMaskArrayName(nullptr);
  this->SetCellLocatorPrototype(nullptr);
  this->SetFindCellStrategy(nullptr);

  delete this->CellArrays;
  delete this->PointList;
  delete this->CellList;
}

//----------------------------------------------------------------------------
void svtkProbeFilter::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkProbeFilter::SetSourceData(svtkDataObject* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
svtkIdTypeArray* svtkProbeFilter::GetValidPoints()
{
  if (this->MaskPoints && this->MaskPoints->GetMTime() > this->ValidPoints->GetMTime())
  {
    char* maskArray = this->MaskPoints->GetPointer(0);
    svtkIdType numPts = this->MaskPoints->GetNumberOfTuples();
    svtkIdType numValidPoints = std::count(maskArray, maskArray + numPts, static_cast<char>(1));
    this->ValidPoints->Allocate(numValidPoints);
    for (svtkIdType i = 0; i < numPts; ++i)
    {
      if (maskArray[i])
      {
        this->ValidPoints->InsertNextValue(i);
      }
    }
    this->ValidPoints->Modified();
  }

  return this->ValidPoints;
}

//----------------------------------------------------------------------------
int svtkProbeFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* source = svtkDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (this->CategoricalData == 1)
  {
    // If the categorical data flag is enabled, then a) there must be scalars
    // to treat as categorical data, and b) the scalars must have one component.
    if (!source->GetPointData()->GetScalars())
    {
      svtkErrorMacro(<< "No input scalars!");
      return 1;
    }
    if (source->GetPointData()->GetScalars()->GetNumberOfComponents() != 1)
    {
      svtkErrorMacro(<< "Source scalars have more than one component! Cannot categorize!");
      return 1;
    }

    // Set the scalar to interpolate via nearest neighbor. That way, we won't
    // get any false values (for example, a zone 4 cell appearing on the
    // boundary of zone 3 and zone 5).
    output->GetPointData()->SetCopyAttribute(
      svtkDataSetAttributes::SCALARS, 2, svtkDataSetAttributes::INTERPOLATE);
  }

  if (source)
  {
    this->Probe(input, source, output);
  }

  this->PassAttributeData(input, source, output);
  return 1;
}

//----------------------------------------------------------------------------
void svtkProbeFilter::PassAttributeData(
  svtkDataSet* input, svtkDataObject* svtkNotUsed(source), svtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
  {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i = 0; i < numPtArrays; ++i)
    {
      svtkDataArray* da = input->GetPointData()->GetArray(i);
      if (!output->GetPointData()->HasArray(da->GetName()))
      {
        output->GetPointData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      svtkAbstractArray* da = input->GetPointData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetPointData()->GetAttribute(i))
      {
        output->GetPointData()->SetAttribute(da, i);
      }
    }
  }

  // copy cell data arrays
  if (this->PassCellArrays)
  {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i = 0; i < numCellArrays; ++i)
    {
      svtkDataArray* da = input->GetCellData()->GetArray(i);
      if (!output->GetCellData()->HasArray(da->GetName()))
      {
        output->GetCellData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      svtkAbstractArray* da = input->GetCellData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetCellData()->GetAttribute(i))
      {
        output->GetCellData()->SetAttribute(da, i);
      }
    }
  }

  if (this->PassFieldArrays)
  {
    // nothing to do, svtkDemandDrivenPipeline takes care of that.
  }
  else
  {
    output->GetFieldData()->Initialize();
  }
}

//----------------------------------------------------------------------------
void svtkProbeFilter::BuildFieldList(svtkDataSet* source)
{
  delete this->PointList;
  delete this->CellList;

  this->PointList = new svtkDataSetAttributes::FieldList(1);
  this->PointList->InitializeFieldList(source->GetPointData());

  this->CellList = new svtkDataSetAttributes::FieldList(1);
  this->CellList->InitializeFieldList(source->GetCellData());
}

//----------------------------------------------------------------------------
// * input -- dataset probed with
// * source -- dataset probed into
// * output - output.
void svtkProbeFilter::InitializeForProbing(svtkDataSet* input, svtkDataSet* output)
{
  if (!this->PointList || !this->CellList)
  {
    svtkErrorMacro("BuildFieldList() must be called before calling this method.");
    return;
  }

  svtkIdType numPts = input->GetNumberOfPoints();

  // if this is repeatedly called by the pipeline for a composite mesh,
  // you need a new array for each block
  // (that is you need to reinitialize the object)
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
  }
  this->MaskPoints = svtkCharArray::New();
  this->MaskPoints->SetNumberOfComponents(1);
  this->MaskPoints->SetNumberOfTuples(numPts);
  this->MaskPoints->FillValue(0);
  this->MaskPoints->SetName(
    this->ValidPointMaskArrayName ? this->ValidPointMaskArrayName : "svtkValidPointMask");

  // Allocate storage for output PointData
  // All input PD is passed to output as PD. Those arrays in input CD that are
  // not present in output PD will be passed as output PD.
  svtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate((*this->PointList), numPts, numPts);

  svtkCellData* tempCellData = svtkCellData::New();
  // We're okay with copying global ids for cells. we just don't flag them as
  // such.
  tempCellData->CopyAllOn(svtkDataSetAttributes::COPYTUPLE);
  tempCellData->CopyAllocate((*this->CellList), numPts, numPts);

  this->CellArrays->clear();
  int numCellArrays = tempCellData->GetNumberOfArrays();
  for (int cc = 0; cc < numCellArrays; cc++)
  {
    svtkDataArray* inArray = tempCellData->GetArray(cc);
    if (inArray && inArray->GetName() && !outPD->GetArray(inArray->GetName()))
    {
      outPD->AddArray(inArray);
      this->CellArrays->push_back(inArray);
    }
  }
  tempCellData->Delete();

  this->InitializeOutputArrays(outPD, numPts);
  outPD->AddArray(this->MaskPoints);
}

//----------------------------------------------------------------------------
void svtkProbeFilter::InitializeOutputArrays(svtkPointData* outPD, svtkIdType numPts)
{
  for (int i = 0; i < outPD->GetNumberOfArrays(); ++i)
  {
    svtkDataArray* da = outPD->GetArray(i);
    if (da)
    {
      da->SetNumberOfTuples(numPts);
      da->Fill(0);
    }
  }
}

//----------------------------------------------------------------------------
void svtkProbeFilter::DoProbing(
  svtkDataSet* input, int srcIdx, svtkDataSet* source, svtkDataSet* output)
{
  svtkBoundingBox sbox(source->GetBounds());
  svtkBoundingBox ibox(input->GetBounds());
  if (!sbox.Intersects(ibox))
  {
    return;
  }

  if (svtkImageData::SafeDownCast(input))
  {
    svtkImageData* inImage = svtkImageData::SafeDownCast(input);
    svtkImageData* outImage = svtkImageData::SafeDownCast(output);
    this->ProbePointsImageData(inImage, srcIdx, source, outImage);
  }
  else
  {
    this->ProbeEmptyPoints(input, srcIdx, source, output);
  }
}

//----------------------------------------------------------------------------
void svtkProbeFilter::Probe(svtkDataSet* input, svtkDataSet* source, svtkDataSet* output)
{
  this->BuildFieldList(source);
  this->InitializeForProbing(input, output);
  this->DoProbing(input, 0, source, output);
}

//----------------------------------------------------------------------------
void svtkProbeFilter::ProbeEmptyPoints(
  svtkDataSet* input, int srcIdx, svtkDataSet* source, svtkDataSet* output)
{
  svtkIdType ptId, numPts;
  double x[3], tol2;
  svtkPointData *pd, *outPD;
  svtkCellData* cd;
  int subId;
  double pcoords[3], *weights;
  double fastweights[256];

  svtkDebugMacro(<< "Probing data");

  pd = source->GetPointData();
  cd = source->GetCellData();

  // lets use a stack allocated array if possible for performance reasons
  int mcs = source->GetMaxCellSize();
  if (mcs <= 256)
  {
    weights = fastweights;
  }
  else
  {
    weights = new double[mcs];
  }

  numPts = input->GetNumberOfPoints();
  outPD = output->GetPointData();

  char* maskArray = this->MaskPoints->GetPointer(0);

  if (this->ComputeTolerance)
  {
    // to compute a reasonable starting tolerance we use
    // a fraction of the largest cell length we come across
    // out of the first few cells. Tolerance is meant
    // to be an epsilon for cases such as probing 2D
    // cells where the XYZ may be a tad off the surface
    // but "close enough"
    double sLength2 = 0;
    for (svtkIdType i = 0; i < 20 && i < source->GetNumberOfCells(); i++)
    {
      double cLength2 = source->GetCell(i)->GetLength2();
      if (sLength2 < cLength2)
      {
        sLength2 = cLength2;
      }
    }
    // use 1% of the diagonal (1% has to be squared)
    tol2 = sLength2 * CELL_TOLERANCE_FACTOR_SQR;
  }
  else
  {
    tol2 = (this->Tolerance * this->Tolerance);
  }

  // svtkPointSet based datasets do not have an implicit structure to their
  // points. A locator is needed to accelerate the search for cells, i.e.,
  // perform the FindCell() operation. Because of backward legacy there are
  // multiple ways to do this. A svtkFindCellStrategy is preferred, but users
  // can also directly specify a cell locator (via the cell locator
  // prototype). If neither of these is specified, then
  // svtkDataSet::FindCell() is used to accelerate the search.
  svtkFindCellStrategy* strategy = nullptr;
  svtkNew<svtkCellLocatorStrategy> cellLocStrategy;
  svtkPointSet* ps;
  if ((ps = svtkPointSet::SafeDownCast(source)) != nullptr)
  {
    if (this->FindCellStrategy != nullptr)
    {
      this->FindCellStrategy->Initialize(ps);
      strategy = this->FindCellStrategy;
    }
    else if (this->CellLocatorPrototype != nullptr)
    {
      cellLocStrategy->SetCellLocator(this->CellLocatorPrototype->NewInstance());
      cellLocStrategy->GetCellLocator()->SetDataSet(source);
      cellLocStrategy->GetCellLocator()->Update();
      strategy = static_cast<svtkFindCellStrategy*>(cellLocStrategy.GetPointer());
      cellLocStrategy->GetCellLocator()->UnRegister(this); // strategy took ownership
    }
  }

  // Find the cell that contains xyz and get it
  if (strategy == nullptr)
  {
    svtkDebugMacro(<< "Using svtkDataSet::FindCell()");
  }
  else
  {
    svtkDebugMacro(<< "Using strategy: " << strategy->GetClassName());
  }

  // Loop over all input points, interpolating source data
  //
  svtkNew<svtkGenericCell> gcell;
  int abort = 0;
  svtkIdType progressInterval = numPts / 20 + 1;
  for (ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if (!(ptId % progressInterval))
    {
      this->UpdateProgress(static_cast<double>(ptId) / numPts);
      abort = GetAbortExecute();
    }

    if (maskArray[ptId] == static_cast<char>(1))
    {
      // skip points which have already been probed with success.
      // This is helpful for multiblock dataset probing.
      continue;
    }

    // Get the xyz coordinate of the point in the input dataset
    input->GetPoint(ptId, x);

    svtkIdType cellId = (strategy != nullptr)
      ? strategy->FindCell(x, nullptr, gcell.GetPointer(), -1, tol2, subId, pcoords, weights)
      : source->FindCell(x, nullptr, -1, tol2, subId, pcoords, weights);

    svtkCell* cell = nullptr;
    if (cellId >= 0)
    {
      cell = source->GetCell(cellId);
      if (this->ComputeTolerance)
      {
        // If ComputeTolerance is set, compute a tolerance proportional to the
        // cell length.
        double dist2;
        double closestPoint[3];
        cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
        if (dist2 > (cell->GetLength2() * CELL_TOLERANCE_FACTOR_SQR))
        {
          continue;
        }
      }
    }

    if (cell)
    {
      // Interpolate the point data
      outPD->InterpolatePoint((*this->PointList), pd, srcIdx, ptId, cell->PointIds, weights);
      svtkVectorOfArrays::iterator iter;
      for (iter = this->CellArrays->begin(); iter != this->CellArrays->end(); ++iter)
      {
        svtkDataArray* inArray = cd->GetArray((*iter)->GetName());
        if (inArray)
        {
          outPD->CopyTuple(inArray, *iter, cellId, ptId);
        }
      }
      maskArray[ptId] = static_cast<char>(1);
    }
  }

  this->MaskPoints->Modified();
  if (mcs > 256)
  {
    delete[] weights;
  }
}

//---------------------------------------------------------------------------
static void GetPointIdsInRange(double rangeMin, double rangeMax, double start, double stepsize,
  int numSteps, int& minid, int& maxid)
{
  if (stepsize == 0)
  {
    minid = maxid = 0;
    return;
  }

  minid = svtkMath::Ceil((rangeMin - start) / stepsize);
  if (minid < 0)
  {
    minid = 0;
  }

  maxid = svtkMath::Floor((rangeMax - start) / stepsize);
  if (maxid > numSteps - 1)
  {
    maxid = numSteps - 1;
  }
}

//---------------------------------------------------------------------------
void svtkProbeFilter::ProbeImagePointsInCell(svtkCell* cell, svtkIdType cellId, svtkDataSet* source,
  int srcBlockId, const double start[3], const double spacing[3], const int dim[3],
  svtkPointData* outPD, char* maskArray, double* wtsBuff)
{
  svtkPointData* pd = source->GetPointData();
  svtkCellData* cd = source->GetCellData();

  // get coordinates of sampling grids
  double cellBounds[6];
  cell->GetBounds(cellBounds);

  int idxBounds[6];
  GetPointIdsInRange(
    cellBounds[0], cellBounds[1], start[0], spacing[0], dim[0], idxBounds[0], idxBounds[1]);
  GetPointIdsInRange(
    cellBounds[2], cellBounds[3], start[1], spacing[1], dim[1], idxBounds[2], idxBounds[3]);
  GetPointIdsInRange(
    cellBounds[4], cellBounds[5], start[2], spacing[2], dim[2], idxBounds[4], idxBounds[5]);

  if ((idxBounds[1] - idxBounds[0]) < 0 || (idxBounds[3] - idxBounds[2]) < 0 ||
    (idxBounds[5] - idxBounds[4]) < 0)
  {
    return;
  }

  double cpbuf[3];
  double dist2 = 0;
  double* closestPoint = cpbuf;
  if (cell->IsA("svtkCell3D"))
  {
    // we only care about closest point and its distance for 2D cells
    closestPoint = nullptr;
  }

  double userTol2 = this->Tolerance * this->Tolerance;
  for (int iz = idxBounds[4]; iz <= idxBounds[5]; iz++)
  {
    double p[3];
    p[2] = start[2] + iz * spacing[2];
    for (int iy = idxBounds[2]; iy <= idxBounds[3]; iy++)
    {
      p[1] = start[1] + iy * spacing[1];
      for (int ix = idxBounds[0]; ix <= idxBounds[1]; ix++)
      {
        // For each grid point within the cell bound, interpolate values
        p[0] = start[0] + ix * spacing[0];

        double pcoords[3];
        int subId;
        int inside = cell->EvaluatePosition(p, closestPoint, subId, pcoords, dist2, wtsBuff);

        // If ComputeTolerance is set, compute a tolerance proportional to the
        // cell length. Otherwise, use the user specified absolute tolerance.
        double tol2 =
          this->ComputeTolerance ? (CELL_TOLERANCE_FACTOR_SQR * cell->GetLength2()) : userTol2;

        if ((inside == 1) && (dist2 <= tol2))
        {
          svtkIdType ptId = ix + dim[0] * (iy + dim[1] * iz);

          // Interpolate the point data
          outPD->InterpolatePoint(
            (*this->PointList), pd, srcBlockId, ptId, cell->PointIds, wtsBuff);

          // Assign cell data
          svtkVectorOfArrays::iterator iter;
          for (iter = this->CellArrays->begin(); iter != this->CellArrays->end(); ++iter)
          {
            svtkDataArray* inArray = cd->GetArray((*iter)->GetName());
            if (inArray)
            {
              outPD->CopyTuple(inArray, *iter, cellId, ptId);
            }
          }

          maskArray[ptId] = static_cast<char>(1);
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
namespace
{

class CellStorage
{
public:
  CellStorage() { this->Initialize(); }

  ~CellStorage() { this->Clear(); }

  // Copying does not make sense for this class but svtkSMPThreadLocal needs
  // these functions to compile. Just initialize the object.
  CellStorage(const CellStorage&) { this->Initialize(); }

  CellStorage& operator=(const CellStorage&)
  {
    this->Clear();
    this->Initialize();
    return *this;
  }

  svtkCell* GetCell(svtkDataSet* dataset, svtkIdType cellId)
  {
    int celltype = dataset->GetCellType(cellId);
    svtkGenericCell*& gc = this->Cells[celltype];
    if (!gc)
    {
      gc = svtkGenericCell::New();
    }
    dataset->GetCell(cellId, gc);
    return gc->GetRepresentativeCell();
  }

private:
  void Initialize()
  {
    svtkGenericCell* null = nullptr;
    std::fill(this->Cells, this->Cells + SVTK_NUMBER_OF_CELL_TYPES, null);
  }

  void Clear()
  {
    for (int i = 0; i < SVTK_NUMBER_OF_CELL_TYPES; ++i)
    {
      if (this->Cells[i])
      {
        this->Cells[i]->Delete();
      }
    }
  }

  svtkGenericCell* Cells[SVTK_NUMBER_OF_CELL_TYPES];
};

} // anonymous namespace

class svtkProbeFilter::ProbeImageDataWorklet
{
public:
  ProbeImageDataWorklet(svtkProbeFilter* probeFilter, svtkDataSet* source, int srcBlockId,
    const double start[3], const double spacing[3], const int dim[3], svtkPointData* outPD,
    char* maskArray, int maxCellSize)
    : ProbeFilter(probeFilter)
    , Source(source)
    , SrcBlockId(srcBlockId)
    , Start(start)
    , Spacing(spacing)
    , Dim(dim)
    , OutPointData(outPD)
    , MaskArray(maskArray)
    , MaxCellSize(maxCellSize)
  {
  }

  void operator()(svtkIdType cellBegin, svtkIdType cellEnd)
  {
    double fastweights[256];
    double* weights;
    if (this->MaxCellSize <= 256)
    {
      weights = fastweights;
    }
    else
    {
      std::vector<double>& dynamicweights = this->WeightsBuffer.Local();
      dynamicweights.resize(this->MaxCellSize);
      weights = &dynamicweights[0];
    }

    CellStorage& cs = this->Cells.Local();
    for (svtkIdType cellId = cellBegin; cellId < cellEnd; ++cellId)
    {
      svtkCell* cell = cs.GetCell(this->Source, cellId);
      this->ProbeFilter->ProbeImagePointsInCell(cell, cellId, this->Source, this->SrcBlockId,
        this->Start, this->Spacing, this->Dim, this->OutPointData, this->MaskArray, weights);
    }
  }

private:
  svtkProbeFilter* ProbeFilter;
  svtkDataSet* Source;
  int SrcBlockId;
  const double* Start;
  const double* Spacing;
  const int* Dim;
  svtkPointData* OutPointData;
  char* MaskArray;
  int MaxCellSize;

  svtkSMPThreadLocal<std::vector<double> > WeightsBuffer;
  svtkSMPThreadLocal<CellStorage> Cells;
};

//----------------------------------------------------------------------------
void svtkProbeFilter::ProbePointsImageData(
  svtkImageData* input, int srcIdx, svtkDataSet* source, svtkImageData* output)
{
  svtkPointData* outPD = output->GetPointData();
  char* maskArray = this->MaskPoints->GetPointer(0);

  //----------------------------------------
  double spacing[3];
  input->GetSpacing(spacing);
  int extent[6];
  input->GetExtent(extent);
  int dim[3];
  input->GetDimensions(dim);
  double start[3];
  input->GetOrigin(start);
  start[0] += static_cast<double>(extent[0]) * spacing[0];
  start[1] += static_cast<double>(extent[2]) * spacing[1];
  start[2] += static_cast<double>(extent[4]) * spacing[2];

  svtkIdType numSrcCells = source->GetNumberOfCells();

  // dummy call required before multithreaded calls
  static_cast<void>(source->GetCellType(0));
  ProbeImageDataWorklet worklet(
    this, source, srcIdx, start, spacing, dim, outPD, maskArray, source->GetMaxCellSize());
  svtkSMPTools::For(0, numSrcCells, worklet);

  this->MaskPoints->Modified();
}

//----------------------------------------------------------------------------
int svtkProbeFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // A variation of the bug fix from John Biddiscombe.
  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (svtkImageData::HasScalarType(sourceInfo))
  {
    svtkImageData::SetScalarType(svtkImageData::GetScalarType(sourceInfo), outInfo);
  }
  if (svtkImageData::HasNumberOfScalarComponents(sourceInfo))
  {
    svtkImageData::SetNumberOfScalarComponents(
      svtkImageData::GetNumberOfScalarComponents(sourceInfo), outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkProbeFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happened to CopyUpdateExtent in svtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (output &&
    (!strcmp(output->GetClassName(), "svtkUnstructuredGrid") ||
      !strcmp(output->GetClassName(), "svtkPolyData")))
  {
    usePiece = 1;
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  sourceInfo->Remove(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  if (!this->SpatialMatch)
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  else if (this->SpatialMatch == 1)
  {
    if (usePiece)
    {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()) + 1);
    }
    else
    {
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
    }
  }

  if (usePiece)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  else
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
  }

  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkProbeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkDataObject* source = this->GetSource();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "SpatialMatch: " << (this->SpatialMatch ? "On" : "Off") << "\n";
  os << indent << "ValidPointMaskArrayName: "
     << (this->ValidPointMaskArrayName ? this->ValidPointMaskArrayName : "svtkValidPointMask")
     << "\n";
  os << indent << "PassFieldArrays: " << (this->PassFieldArrays ? "On" : " Off") << "\n";

  os << indent << "FindCellStrategy: "
     << (this->FindCellStrategy ? this->FindCellStrategy->GetClassName() : "NULL") << "\n";
  os << indent << "CellLocatorPrototype: "
     << (this->CellLocatorPrototype ? this->CellLocatorPrototype->GetClassName() : "NULL") << "\n";
}
