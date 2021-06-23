/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFitToHeightMapFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFitToHeightMapFilter.h"

#include "svtkCellData.h"
#include "svtkGenericCell.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPixel.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTriangle.h"

svtkStandardNewMacro(svtkFitToHeightMapFilter);

// These are created to support a (float,double) fast path. They work in
// tandem with svtkTemplate2Macro found in svtkSetGet.h.
#define svtkTemplate2MacroFP(call)                                                                  \
  svtkTemplate2MacroCase1FP(SVTK_DOUBLE, double, call);                                              \
  svtkTemplate2MacroCase1FP(SVTK_FLOAT, float, call)
#define svtkTemplate2MacroCase1FP(type1N, type1, call)                                              \
  svtkTemplate2MacroCase2(type1N, type1, SVTK_DOUBLE, double, call);                                 \
  svtkTemplate2MacroCase2(type1N, type1, SVTK_FLOAT, float, call)

namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm for projecting points.
template <typename TPoints, typename TScalars>
struct FitPoints
{
  svtkIdType NPts;
  TPoints* InPoints;
  TPoints* OutPoints;
  TScalars* Scalars;
  double Dims[3];
  double Origin[3];
  double H[3];

  FitPoints(svtkIdType npts, TPoints* inPts, TPoints* outPts, TScalars* s, int dims[3], double o[3],
    double h[3])
    : NPts(npts)
    , InPoints(inPts)
    , OutPoints(outPts)
    , Scalars(s)
  {
    for (int i = 0; i < 3; ++i)
    {
      this->Dims[i] = static_cast<double>(dims[i]);
      this->Origin[i] = o[i];
      this->H[i] = h[i];
    }
  }

  void Initialize() {}

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const TPoints* xi = this->InPoints + 3 * ptId;
    TPoints* xo = this->OutPoints + 3 * ptId;
    const double* d = this->Dims;
    const double* o = this->Origin;
    const double* h = this->H;
    int ii, jj;
    double i, j, z, pc[2], ix, iy, w[4];
    TScalars const* scalars = this->Scalars;
    int s0, s1, s2, s3;

    for (; ptId < endPtId; ++ptId, xi += 3, xo += 3)
    {
      // Location in image
      i = (xi[0] - o[0]) / h[0];
      j = (xi[1] - o[1]) / h[1];

      // Clamp to image. i,j is integral index into image pixels. It has to be done carefully
      // to manage the parametric coordinates correctly.
      if (i < 0.0)
      {
        ix = 0.0;
        pc[0] = 0.0;
      }
      else if (i >= (d[0] - 1))
      {
        ix = d[0] - 2;
        pc[0] = 1.0;
      }
      else
      {
        pc[0] = modf(i, &ix);
      }

      if (j < 0.0)
      {
        iy = 0.0;
        pc[1] = 0.0;
      }
      else if (j >= (d[1] - 1))
      {
        iy = d[1] - 2;
        pc[1] = 1.0;
      }
      else
      {
        pc[1] = modf(j, &iy);
      }

      // Parametric coordinates for interpolation
      svtkPixel::InterpolationFunctions(pc, w);
      ii = static_cast<int>(ix);
      jj = static_cast<int>(iy);

      // Interpolate height from surrounding data values
      s0 = ii + jj * d[0];
      s1 = s0 + 1;
      s2 = s0 + d[0];
      s3 = s2 + 1;
      z = w[0] * scalars[s0] + w[1] * scalars[s1] + w[2] * scalars[s2] + w[3] * scalars[s3];

      // Set the output point coordinates with a new z-value.
      xo[0] = xi[0];
      xo[1] = xi[1];
      xo[2] = z;
    }
  }

  void Reduce() {}

  static void Execute(svtkIdType numPts, TPoints* inPts, TPoints* points, TScalars* s, int dims[3],
    double origin[3], double h[3])
  {
    FitPoints fit(numPts, inPts, points, s, dims, origin, h);
    svtkSMPTools::For(0, numPts, fit);
  }
}; // FitPoints

//----------------------------------------------------------------------------
// The threaded core of the algorithm when projecting cells.
template <typename TScalars>
struct FitCells
{
  int Strategy;
  svtkPolyData* Mesh;
  double* CellHeights;
  TScalars* Scalars;
  double Dims[3];
  double Origin[3];
  double H[3];

  // Avoid repeated allocation
  svtkSMPThreadLocalObject<svtkGenericCell> Cell;
  svtkSMPThreadLocalObject<svtkIdList> Prims;
  svtkSMPThreadLocalObject<svtkPoints> PrimPts;

  FitCells(int strat, svtkPolyData* mesh, double* cellHts, TScalars* s, int dims[3], double o[3],
    double h[3])
    : Strategy(strat)
    , Mesh(mesh)
    , CellHeights(cellHts)
    , Scalars(s)
  {
    for (int i = 0; i < 3; ++i)
    {
      this->Dims[i] = static_cast<double>(dims[i]);
      this->Origin[i] = o[i];
      this->H[i] = h[i];
    }
  }

  void Initialize()
  {
    svtkGenericCell*& cell = this->Cell.Local();
    cell->PointIds->Allocate(128);
    cell->Points->Allocate(128);

    svtkIdList*& prims = this->Prims.Local();
    prims->Allocate(128); // allocate some memory

    svtkPoints*& primPts = this->PrimPts.Local();
    primPts->Allocate(128); // allocate some memory
  }

  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    double* cellHts = this->CellHeights + cellId;
    TScalars const* scalars = this->Scalars;
    svtkGenericCell*& cell = this->Cell.Local();
    svtkIdList*& prims = this->Prims.Local();
    svtkPoints*& primPts = this->PrimPts.Local();
    const double* d = this->Dims;
    const double* o = this->Origin;
    const double* h = this->H;
    svtkIdType p, pi, numPrims;
    int cellDim, ii, jj;
    double i, j, z, pc[2], ix, iy, w[4];
    double x0[3], center[2];
    int s0, s1, s2, s3;
    double sum, min, max;

    // Process all cells of different types and dimensions
    for (; cellId < endCellId; ++cellId)
    {
      this->Mesh->GetCell(cellId, cell);
      cellDim = cell->GetCellDimension();

      cell->Triangulate(0, prims, primPts);
      numPrims = prims->GetNumberOfIds() / (cellDim + 1);

      // Loop over each primitive from the tessellation
      min = SVTK_FLOAT_MAX;
      max = SVTK_FLOAT_MIN;
      sum = 0.0;
      for (p = 0; p < numPrims; p++)
      {
        // Compute center of primitive
        center[0] = center[1] = 0.0;
        for (pi = 0; pi <= cellDim; ++pi)
        {
          primPts->GetPoint((cellDim + 1) * p + pi, x0);
          center[0] += x0[0];
          center[1] += x0[1];
        }
        center[0] /= static_cast<double>(cellDim + 1);
        center[1] /= static_cast<double>(cellDim + 1);

        // Location in image
        i = (center[0] - o[0]) / h[0];
        j = (center[1] - o[1]) / h[1];

        // Clamp to image. i,j is integral index into image pixels. It has to be done carefully
        // to manage the parametric coordinates correctly.
        if (i < 0.0)
        {
          ix = 0.0;
          pc[0] = 0.0;
        }
        else if (i >= (d[0] - 1))
        {
          ix = d[0] - 2;
          pc[0] = 1.0;
        }
        else
        {
          pc[0] = modf(i, &ix);
        }

        if (j < 0.0)
        {
          iy = 0.0;
          pc[1] = 0.0;
        }
        else if (j >= (d[1] - 1))
        {
          iy = d[1] - 2;
          pc[1] = 1.0;
        }
        else
        {
          pc[1] = modf(j, &iy);
        }

        // Parametric coordinates for interpolation
        svtkPixel::InterpolationFunctions(pc, w);
        ii = static_cast<int>(ix);
        jj = static_cast<int>(iy);

        // Interpolate height from surrounding data values
        s0 = ii + jj * d[0];
        s1 = s0 + 1;
        s2 = s0 + d[0];
        s3 = s2 + 1;
        z = w[0] * scalars[s0] + w[1] * scalars[s1] + w[2] * scalars[s2] + w[3] * scalars[s3];

        // Gather information for final determination of height z
        if (z < min)
        {
          min = z;
        }
        if (z > max)
        {
          max = z;
        }
        sum += z; // to compute average
      }           // for all tessellated primitives

      // Now set the cell height
      if (this->Strategy == svtkFitToHeightMapFilter::CELL_AVERAGE_HEIGHT)
      {
        z = fabs(sum / static_cast<double>(numPrims));
      }
      else if (this->Strategy == svtkFitToHeightMapFilter::CELL_MINIMUM_HEIGHT)
      {
        z = min;
      }
      else // if ( this->Strategy == svtkFitToHeightMapFilter::CELL_MAXIMUM_HEIGHT )
      {
        z = max;
      }

      *cellHts++ = z;
    }
  }

  void Reduce() {}

  static void Execute(int strategy, svtkPolyData* mesh, double* cellHts, TScalars* s, int dims[3],
    double origin[3], double h[3])
  {
    FitCells fit(strategy, mesh, cellHts, s, dims, origin, h);
    svtkIdType numCells = mesh->GetNumberOfCells();
    svtkSMPTools::For(0, numCells, fit);
  }
}; // FitCells

} // anonymous namespace

//----------------------------------------------------------------------------
// Construct object. Two inputs are mandatory.
svtkFitToHeightMapFilter::svtkFitToHeightMapFilter()
{
  this->SetNumberOfInputPorts(2);

  this->FittingStrategy = svtkFitToHeightMapFilter::POINT_PROJECTION;
  this->UseHeightMapOffset = true;
  this->Offset = 0.0;
}

//----------------------------------------------------------------------------
svtkFitToHeightMapFilter::~svtkFitToHeightMapFilter() = default;

//----------------------------------------------------------------------------
int svtkFitToHeightMapFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* in2Info = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDebugMacro(<< "Executing fit to height map");

  // get the two inputs and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* image = svtkImageData::SafeDownCast(in2Info->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Check input
  if (input == nullptr || image == nullptr || output == nullptr)
  {
    svtkErrorMacro(<< "Bad input");
    return 1;
  }
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
  {
    svtkDebugMacro(<< "Empty input");
    return 1;
  }

  // Only process real-type points
  svtkPoints* inPts = input->GetPoints();
  int ptsType = inPts->GetDataType();
  if (ptsType != SVTK_FLOAT && ptsType != SVTK_DOUBLE)
  {
    svtkErrorMacro(<< "This filter only handles (float,double) points");
    return 1;
  }

  // Looking for a xy-oriented image
  int dims[3];
  double origin[3], h[3];
  image->GetDimensions(dims);
  image->GetOrigin(origin);
  image->GetSpacing(h);
  int imgType = image->GetScalarType();

  if (dims[0] < 2 || dims[1] < 2 || dims[2] != 1)
  {
    svtkErrorMacro(<< "Filter operates on 2D x-y images");
    return 1;
  }

  // Finally warn if the image data does not fully contain the
  // input polydata.
  double inputBds[6], imageBds[6];
  input->GetBounds(inputBds);
  image->GetBounds(imageBds);
  if (inputBds[0] < imageBds[0] || inputBds[1] > imageBds[1] || inputBds[2] < imageBds[2] ||
    inputBds[3] > imageBds[3])
  {
    svtkWarningMacro(<< "Input polydata extends beyond height map");
  }
  this->Offset = (this->UseHeightMapOffset ? imageBds[4] : 0.0);

  // Okay we are ready to rock and roll...
  output->CopyStructure(input);

  svtkPoints* newPts = svtkPoints::New();
  newPts->SetDataType(inPts->GetDataType());
  newPts->SetNumberOfPoints(numPts);

  svtkPointData* pd = input->GetPointData();
  svtkPointData* outputPD = output->GetPointData();
  outputPD->CopyNormalsOff(); // normals are almost certainly messed up
  outputPD->PassData(pd);

  svtkCellData* cd = input->GetCellData();
  svtkCellData* outputCD = output->GetCellData();
  outputCD->PassData(cd);

  // We need random access to cells
  output->BuildCells();

  // This performs the projection of the points or cells.
  void* inPtr = inPts->GetVoidPointer(0);
  void* outPtr = newPts->GetVoidPointer(0);
  void* inScalarPtr = image->GetScalarPointer();

  // We either process points or cells depending on strategy
  if (this->FittingStrategy <= svtkFitToHeightMapFilter::POINT_AVERAGE_HEIGHT)
  {

    switch (svtkTemplate2PackMacro(ptsType, imgType))
    {
      svtkTemplate2MacroFP((FitPoints<SVTK_T1, SVTK_T2>::Execute(
        numPts, (SVTK_T1*)inPtr, (SVTK_T1*)outPtr, (SVTK_T2*)inScalarPtr, dims, origin, h)));

      default:
        svtkErrorMacro(<< "Only (float,double) fast path supported");
        return 0;
    }

    // Now final rollup and adjustment of points
    this->AdjustPoints(output, numCells, newPts);

  } // points strategies

  else // We are processing cells
  {
    double* cellHts = new double[numCells];
    switch (imgType)
    {
      svtkTemplateMacro(FitCells<SVTK_TT>::Execute(
        this->FittingStrategy, output, cellHts, (SVTK_TT*)inScalarPtr, dims, origin, h));

      default:
        svtkErrorMacro(<< "Only (float,double) fast path supported");
        return 0;
    }

    // Now final rollup and adjustment of points
    this->AdjustCells(output, numCells, cellHts, inPts, newPts);
    delete[] cellHts;

  } // cells strategies

  // Clean up and get out. Replace the output's shallow-copied points with
  // the new, projected points.
  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Based on the fitting strategy, adjust the point coordinates.
void svtkFitToHeightMapFilter::AdjustPoints(
  svtkPolyData* output, svtkIdType numCells, svtkPoints* newPts)
{
  svtkIdType cellId;
  svtkIdType npts;
  const svtkIdType* ptIds;
  svtkIdType pId;
  svtkIdType i;
  double sum, min, max, p0[3], z;
  svtkIdType numHits;

  // Nothing to do except adjust offset if point projection
  if (this->FittingStrategy == svtkFitToHeightMapFilter::POINT_PROJECTION)
  {
    if (this->UseHeightMapOffset)
    {
      npts = newPts->GetNumberOfPoints();
      for (pId = 0; pId < npts; ++pId)
      {
        newPts->GetPoint(pId, p0);
        newPts->SetPoint(pId, p0[0], p0[1], p0[2] + this->Offset);
      }
    }
    return;
  }

  // Otherwise fancier point adjustment
  for (cellId = 0; cellId < numCells; ++cellId)
  {
    output->GetCellPoints(cellId, npts, ptIds);

    // Gather information about cell
    min = SVTK_FLOAT_MAX;
    max = SVTK_FLOAT_MIN;
    sum = 0.0;
    numHits = 0;
    for (i = 0; i < npts; ++i)
    {
      pId = ptIds[i];
      numHits++;
      newPts->GetPoint(pId, p0);
      z = p0[2];

      if (z < min)
      {
        min = z;
      }
      if (z > max)
      {
        max = z;
      }
      sum += z; // to compute average
    }           // over primitive points

    // Adjust points as specified.
    if (numHits > 0)
    {
      if (this->FittingStrategy == svtkFitToHeightMapFilter::POINT_AVERAGE_HEIGHT)
      {
        z = fabs(sum / static_cast<double>(numHits));
      }
      else if (this->FittingStrategy == svtkFitToHeightMapFilter::POINT_MINIMUM_HEIGHT)
      {
        z = min;
      }
      else // if ( this->FittingStrategy == svtkFitToHeightMapFilter::POINT_MAXIMUM_HEIGHT )
      {
        z = max;
      }

      for (i = 0; i < npts; ++i)
      {
        pId = ptIds[i];
        newPts->GetPoint(pId, p0);
        newPts->SetPoint(pId, p0[0], p0[1], z + this->Offset);
      }
    } // if valid polygon

  } // for all cells
}

//----------------------------------------------------------------------------
// Based on the fitting strategy, adjust the points based on cell height
// information.
void svtkFitToHeightMapFilter::AdjustCells(
  svtkPolyData* output, svtkIdType numCells, double* cellHts, svtkPoints* inPts, svtkPoints* newPts)
{
  svtkIdType cellId;
  svtkIdType npts;
  const svtkIdType* ptIds;
  svtkIdType pId;
  svtkIdType i;
  double z, p0[3];

  for (cellId = 0; cellId < numCells; ++cellId)
  {
    z = cellHts[cellId];
    output->GetCellPoints(cellId, npts, ptIds);

    for (i = 0; i < npts; ++i)
    {
      pId = ptIds[i];
      inPts->GetPoint(pId, p0);
      newPts->SetPoint(pId, p0[0], p0[1], z + this->Offset);
    }
  } // for all cells
}

//----------------------------------------------------------------------------
// Specify the height map connection
void svtkFitToHeightMapFilter::SetHeightMapConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Specify the height map data
void svtkFitToHeightMapFilter::SetHeightMapData(svtkImageData* id)
{
  this->SetInputData(1, id);
}

//----------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
svtkImageData* svtkFitToHeightMapFilter::GetHeightMap()
{
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
svtkImageData* svtkFitToHeightMapFilter::GetHeightMap(svtkInformationVector* sourceInfo)
{
  svtkInformation* info = sourceInfo->GetInformationObject(1);
  if (!info)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int svtkFitToHeightMapFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkFitToHeightMapFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Fitting Strategy: " << this->FittingStrategy << "\n";
  os << indent << "Use Height Map Offset: " << (this->UseHeightMapOffset ? "On\n" : "Off\n");
}
