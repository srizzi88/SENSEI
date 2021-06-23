/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTessellatorFilter.cxx
  Language:  C++

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#include "svtkObjectFactory.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDataSetEdgeSubdivisionCriterion.h"
#include "svtkEdgeSubdivisionCriterion.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStreamingTessellator.h"
#include "svtkTessellatorFilter.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkTessellatorFilter);

namespace
{
void svtkCopyTuples(svtkDataSetAttributes* inDSA, svtkIdType inId, svtkDataSetAttributes* outDSA,
  svtkIdType beginId, svtkIdType endId)
{
  for (svtkIdType cc = beginId; cc < endId; ++cc)
  {
    outDSA->CopyData(inDSA, inId, cc);
  }
}
}

// ========================================
// svtkCommand subclass for reporting progress of merge filter
class svtkProgressCommand : public svtkCommand
{
public:
  svtkProgressCommand(svtkTessellatorFilter* tf) { this->Tessellator = tf; }
  ~svtkProgressCommand() override = default;
  void Execute(svtkObject*, unsigned long, void* callData) override
  {
    double subprogress = *(static_cast<double*>(callData));
    cout << "  ++ <" << ((subprogress / 2. + 0.5) * 100.) << ">\n";
    this->Tessellator->UpdateProgress(subprogress / 2. + 0.5);
  }

protected:
  svtkTessellatorFilter* Tessellator;
};

// ========================================
// convenience routines for paraview
void svtkTessellatorFilter::SetMaximumNumberOfSubdivisions(int N)
{
  if (this->Tessellator)
  {
    this->Tessellator->SetMaximumNumberOfSubdivisions(N);
  }
}

int svtkTessellatorFilter::GetMaximumNumberOfSubdivisions()
{
  return this->Tessellator ? this->Tessellator->GetMaximumNumberOfSubdivisions() : 0;
}

void svtkTessellatorFilter::SetChordError(double E)
{
  if (this->Subdivider)
  {
    this->Subdivider->SetChordError2(E > 0. ? E * E : E);
  }
}

double svtkTessellatorFilter::GetChordError()
{
  double tmp = this->Subdivider ? this->Subdivider->GetChordError2() : 0.;
  return tmp > 0. ? sqrt(tmp) : tmp;
}

// ========================================
// callbacks for simplex output
void svtkTessellatorFilter::AddATetrahedron(const double* a, const double* b, const double* c,
  const double* d, svtkEdgeSubdivisionCriterion*, void* pd, const void*)
{
  svtkTessellatorFilter* self = (svtkTessellatorFilter*)pd;
  self->OutputTetrahedron(a, b, c, d);
}

void svtkTessellatorFilter::OutputTetrahedron(
  const double* a, const double* b, const double* c, const double* d)
{
  svtkIdType cellIds[4];

  cellIds[0] = this->OutputPoints->InsertNextPoint(a);
  cellIds[1] = this->OutputPoints->InsertNextPoint(b);
  cellIds[2] = this->OutputPoints->InsertNextPoint(c);
  cellIds[3] = this->OutputPoints->InsertNextPoint(d);

  this->OutputMesh->InsertNextCell(SVTK_TETRA, 4, cellIds);

  const int* off = this->Subdivider->GetFieldOffsets();
  svtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;
  b += 6;
  c += 6;
  d += 6;

  for (int at = 0; at < this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off)
  {
    (*att)->InsertTuple(cellIds[0], a + *off);
    (*att)->InsertTuple(cellIds[1], b + *off);
    (*att)->InsertTuple(cellIds[2], c + *off);
    (*att)->InsertTuple(cellIds[3], d + *off);
  }
}

void svtkTessellatorFilter::AddATriangle(const double* a, const double* b, const double* c,
  svtkEdgeSubdivisionCriterion*, void* pd, const void*)
{
  svtkTessellatorFilter* self = (svtkTessellatorFilter*)pd;
  self->OutputTriangle(a, b, c);
}

void svtkTessellatorFilter::OutputTriangle(const double* a, const double* b, const double* c)
{
  svtkIdType cellIds[3];

  cellIds[0] = this->OutputPoints->InsertNextPoint(a);
  cellIds[1] = this->OutputPoints->InsertNextPoint(b);
  cellIds[2] = this->OutputPoints->InsertNextPoint(c);

  this->OutputMesh->InsertNextCell(SVTK_TRIANGLE, 3, cellIds);

  const int* off = this->Subdivider->GetFieldOffsets();
  svtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;
  b += 6;
  c += 6;

  for (int at = 0; at < this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off)
  {
    (*att)->InsertTuple(cellIds[0], a + *off);
    (*att)->InsertTuple(cellIds[1], b + *off);
    (*att)->InsertTuple(cellIds[2], c + *off);
  }
}

void svtkTessellatorFilter::AddALine(
  const double* a, const double* b, svtkEdgeSubdivisionCriterion*, void* pd, const void*)
{
  svtkTessellatorFilter* self = (svtkTessellatorFilter*)pd;
  self->OutputLine(a, b);
}

void svtkTessellatorFilter::OutputLine(const double* a, const double* b)
{
  svtkIdType cellIds[2];

  cellIds[0] = this->OutputPoints->InsertNextPoint(a);
  cellIds[1] = this->OutputPoints->InsertNextPoint(b);

  this->OutputMesh->InsertNextCell(SVTK_LINE, 2, cellIds);

  const int* off = this->Subdivider->GetFieldOffsets();
  svtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;
  b += 6;

  for (int at = 0; at < this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off)
  {
    (*att)->InsertTuple(cellIds[0], a + *off);
    (*att)->InsertTuple(cellIds[1], b + *off);
  }
}

void svtkTessellatorFilter::AddAPoint(
  const double* a, svtkEdgeSubdivisionCriterion*, void* pd, const void*)
{
  svtkTessellatorFilter* self = (svtkTessellatorFilter*)pd;
  self->OutputPoint(a);
}

void svtkTessellatorFilter::OutputPoint(const double* a)
{
  svtkIdType cellId;

  cellId = this->OutputPoints->InsertNextPoint(a);
  this->OutputMesh->InsertNextCell(SVTK_VERTEX, 1, &cellId);

  const int* off = this->Subdivider->GetFieldOffsets();
  svtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;

  for (int at = 0; at < this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off)
  {
    (*att)->InsertTuple(cellId, a + *off);
  }
}

// ========================================

// constructor/boilerplate members
svtkTessellatorFilter::svtkTessellatorFilter()
  : Tessellator(nullptr)
  , Subdivider(nullptr)
{
  this->OutputDimension = 3; // Tessellate elements directly, not boundaries
  this->SetTessellator(svtkStreamingTessellator::New());
  this->Tessellator->Delete();
  this->SetSubdivider(svtkDataSetEdgeSubdivisionCriterion::New());
  this->Subdivider->Delete();
  this->MergePoints = 1;
  this->Locator = svtkMergePoints::New();

  this->Tessellator->SetEmbeddingDimension(1, 3);
  this->Tessellator->SetEmbeddingDimension(2, 3);
}

svtkTessellatorFilter::~svtkTessellatorFilter()
{
  this->SetSubdivider(nullptr);
  this->SetTessellator(nullptr);
  this->Locator->Delete();
  this->Locator = nullptr;
}

void svtkTessellatorFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputDimension: " << this->OutputDimension << "\n"
     << indent << "Tessellator: " << this->Tessellator << "\n"
     << indent << "Subdivider: " << this->Subdivider << " (" << this->Subdivider->GetClassName()
     << ")"
     << "\n"
     << indent << "MergePoints: " << this->MergePoints << "\n"
     << indent << "Locator: " << this->Locator << "\n";
}

// override for proper Update() behavior
svtkMTimeType svtkTessellatorFilter::GetMTime()
{
  svtkMTimeType mt = this->MTime;
  svtkMTimeType tmp;

  if (this->Tessellator)
  {
    tmp = this->Tessellator->GetMTime();
    if (tmp > mt)
      mt = tmp;
  }

  if (this->Subdivider)
  {
    tmp = this->Subdivider->GetMTime();
    if (tmp > mt)
      mt = tmp;
  }

  return mt;
}

void svtkTessellatorFilter::SetTessellator(svtkStreamingTessellator* t)
{
  if (this->Tessellator == t)
  {
    return;
  }

  if (this->Tessellator)
  {
    this->Tessellator->UnRegister(this);
  }

  this->Tessellator = t;

  if (this->Tessellator)
  {
    this->Tessellator->Register(this);
    this->Tessellator->SetSubdivisionAlgorithm(this->Subdivider);
  }

  this->Modified();
}

void svtkTessellatorFilter::SetSubdivider(svtkDataSetEdgeSubdivisionCriterion* s)
{
  if (this->Subdivider == s)
  {
    return;
  }

  if (this->Subdivider)
  {
    this->Subdivider->UnRegister(this);
  }

  this->Subdivider = s;

  if (this->Subdivider)
  {
    this->Subdivider->Register(this);
  }

  if (this->Tessellator)
  {
    this->Tessellator->SetSubdivisionAlgorithm(this->Subdivider);
  }

  this->Modified();
}

void svtkTessellatorFilter::SetFieldCriterion(int s, double err)
{
  if (this->Subdivider)
  {
    this->Subdivider->SetFieldError2(s, err > 0. ? err * err : -1.);
  }
}

void svtkTessellatorFilter::ResetFieldCriteria()
{
  if (this->Subdivider)
  {
    this->Subdivider->ResetFieldError2();
  }
}

// ========================================
// pipeline procedures
void svtkTessellatorFilter::SetupOutput(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  this->OutputMesh = output;

  // avoid doing all the stupid checks on NumberOfOutputs for every
  // triangle/line.
  this->OutputMesh->Reset();
  this->OutputMesh->Allocate(0, 0);

  if (!(this->OutputPoints = OutputMesh->GetPoints()))
  {
    this->OutputPoints = svtkPoints::New();
    this->OutputMesh->SetPoints(this->OutputPoints);
    this->OutputPoints->Delete();
  }

  // This returns the id numbers of arrays that are default scalars, vectors,
  // normals, texture coords, and tensors.  These are the fields that will be
  // interpolated and passed on to the output mesh.
  svtkPointData* fields = input->GetPointData();
  svtkDataSetAttributes* outarrays = this->OutputMesh->GetPointData();
  outarrays->Initialize();
  // empty, turn off all attributes, and set CopyAllOn to true.

  this->OutputAttributes = new svtkDataArray*[fields->GetNumberOfArrays()];
  this->OutputAttributeIndices = new int[fields->GetNumberOfArrays()];

  // OK, we always add normals as the 0-th array so that there's less work to
  // do inside the tight loop (OutputTriangle)
  int attrib = 0;
  for (int a = 0; a < fields->GetNumberOfArrays(); ++a)
  {
    if (fields->IsArrayAnAttribute(a) == svtkDataSetAttributes::NORMALS)
    {
      continue;
    }

    svtkDataArray* array = fields->GetArray(a);
    if (this->Subdivider->PassField(a, array->GetNumberOfComponents(), this->Tessellator) == -1)
    {
      svtkErrorMacro("Could not pass field ("
        << array->GetName() << ") because a compile-time limit of ("
        << svtkStreamingTessellator::MaxFieldSize
        << ") data values has been reached. Increase svtkStreamingTessellator::MaxFieldSize at "
           "compile time to pass more fields.");
      continue;
    }
    this->OutputAttributes[attrib] = svtkDataArray::CreateDataArray(array->GetDataType());
    this->OutputAttributes[attrib]->SetNumberOfComponents(array->GetNumberOfComponents());
    this->OutputAttributes[attrib]->SetName(array->GetName());
    this->OutputAttributeIndices[attrib] = outarrays->AddArray(this->OutputAttributes[attrib]);
    this->OutputAttributes[attrib]->Delete(); // output mesh now owns the array
    int attribType;
    if ((attribType = fields->IsArrayAnAttribute(a)) != -1)
      outarrays->SetActiveAttribute(this->OutputAttributeIndices[attrib], attribType);

    ++attrib;
  }

  output->GetCellData()->CopyAllocate(input->GetCellData(), input->GetNumberOfCells());
}

void svtkTessellatorFilter::MergeOutputPoints(
  svtkUnstructuredGrid* input, svtkUnstructuredGrid* output)
{
  // this method cleverly lifted from ParaView's
  // Servers/Filters/svtkCleanUnstructuredGrid::RequestData()
  if (input->GetNumberOfCells() == 0)
  {
    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.
    output->Allocate(1);
    output->GetPointData()->CopyAllocate(input->GetPointData(), SVTK_CELL_SIZE);
    output->GetCellData()->CopyAllocate(input->GetCellData(), 1);
    svtkPoints* pts = svtkPoints::New();
    output->SetPoints(pts);
    pts->Delete();
    return;
  }

  output->GetPointData()->CopyAllocate(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // First, create a new points array that eliminate duplicate points.
  // Also create a mapping from the old point id to the new.
  svtkPoints* newPts = svtkPoints::New();
  svtkIdType num = input->GetNumberOfPoints();
  svtkIdType id;
  svtkIdType newId;
  svtkIdType* ptMap = new svtkIdType[num];
  double pt[3];

  this->Locator->InitPointInsertion(newPts, input->GetBounds(), num);

  svtkIdType progressStep = num / 100;
  if (progressStep == 0)
  {
    progressStep = 1;
  }
  for (id = 0; id < num; ++id)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress(0.5 * (1. + id * 0.8 / num));
    }
    input->GetPoint(id, pt);
    if (this->Locator->InsertUniquePoint(pt, newId))
    {
      output->GetPointData()->CopyData(input->GetPointData(), id, newId);
    }
    ptMap[id] = newId;
  }
  output->SetPoints(newPts);
  newPts->Delete();

  // New copy the cells.
  svtkIdList* cellPoints = svtkIdList::New();
  num = input->GetNumberOfCells();
  output->Allocate(num);
  for (id = 0; id < num; ++id)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress(0.9 + 0.1 * ((float)id / num));
    }
    input->GetCellPoints(id, cellPoints);
    for (int i = 0; i < cellPoints->GetNumberOfIds(); i++)
    {
      int cellPtId = cellPoints->GetId(i);
      newId = ptMap[cellPtId];
      cellPoints->SetId(i, newId);
    }
    output->InsertNextCell(input->GetCellType(id), cellPoints);
  }

  delete[] ptMap;
  cellPoints->Delete();
}

void svtkTessellatorFilter::Teardown()
{
  this->OutputMesh = nullptr;
  this->OutputPoints = nullptr;
  delete[] this->OutputAttributes;
  delete[] this->OutputAttributeIndices;
  this->Subdivider->ResetFieldList();
  this->Subdivider->SetMesh(nullptr);
}

// ========================================
// output element topology

static const double extraLagrangeCurveParams[3] = { 0.5, 0.0, 0.0 };

static const double extraWedgeParams[][3] = {
  // mid-edge points, bottom
  { 0.5, 0.0, 0.0 },
  { 0.5, 0.5, 0.0 },
  { 0.0, 0.5, 0.0 },
  // mid-edge points, top
  { 0.5, 0.0, 1.0 },
  { 0.5, 0.5, 1.0 },
  { 0.0, 0.5, 1.0 },
  // mid-edge points, vertical
  { 0.0, 0.0, 0.5 },
  { 1.0, 0.0, 0.5 },
  { 0.0, 1.0, 0.5 },
  // mid-face points
  { 1 / 3., 1 / 3., 0.0 },
  { 1 / 3., 1 / 3., 1.0 },
  { 0.5, 0.0, 0.5 },
  { 0.5, 0.5, 0.5 },
  { 0.0, 0.5, 0.5 },
  // body point
  { 1 / 3., 1 / 3., 1 / 3. },
};

static const double extraLinHexParams[12][3] = {
  { 0.5, 0.0, 0.0 },
  { 1.0, 0.5, 0.0 },
  { 0.5, 1.0, 0.0 },
  { 0.0, 0.5, 0.0 },
  { 0.5, 0.0, 1.0 },
  { 1.0, 0.5, 1.0 },
  { 0.5, 1.0, 1.0 },
  { 0.0, 0.5, 1.0 },
  { 0.0, 0.0, 0.5 },
  { 1.0, 0.0, 0.5 },
  { 1.0, 1.0, 0.5 },
  { 0.0, 1.0, 0.5 },
};

static const double extraQuadHexParams[7][3] = {
  { 0.5, 0.5, 0.0 },
  { 0.5, 0.5, 1.0 },
  { 0.5, 0.0, 0.5 },
  { 0.5, 1.0, 0.5 },
  { 0.0, 0.5, 0.5 },
  { 1.0, 0.5, 0.5 },
  { 0.5, 0.5, 0.5 },
};

static const double extraLagrangeQuadParams[4][3] = {
  { 0.5, 0.0, 0.0 },
  { 1.0, 0.5, 0.0 },
  { 0.5, 1.0, 0.0 },
  { 0.0, 0.5, 0.0 },
};

static const double extraQuadQuadParams[1][3] = {
  { 0.5, 0.5, 0.0 },
};

static const double extraLagrangeTriParams[3][3] = {
  { 0.5, 0.0, 0.0 },
  { 0.5, 0.5, 0.0 },
  { 0.0, 0.5, 0.0 },
};

static const double extraLagrangeTetraParams[6][3] = {
  { 0.5, 0.0, 0.0 },
  { 0.5, 0.5, 0.0 },
  { 0.0, 0.5, 0.0 },
  { 0.0, 0.0, 0.5 },
  { 0.5, 0.0, 0.5 },
  { 0.0, 0.5, 0.5 },
};

static svtkIdType linEdgeEdges[][2] = {
  { 0, 1 },
};

static svtkIdType quadEdgeEdges[][2] = {
  { 0, 2 },
  { 2, 1 },
};

static svtkIdType cubicLinEdges[][2] = {
  { 0, 2 },
  { 2, 3 },
  { 3, 1 },
};

static svtkIdType linTriTris[][3] = {
  { 0, 1, 2 },
};

static svtkIdType linTriEdges[][2] = {
  { 0, 1 },
  { 1, 2 },
  { 2, 0 },
};

static svtkIdType quadTriTris[][3] = {
  { 0, 3, 5 },
  { 5, 3, 1 },
  { 5, 1, 4 },
  { 4, 2, 5 },
};

static svtkIdType biQuadTriTris[][3] = {
  { 0, 3, 6 },
  { 3, 1, 6 },
  { 6, 1, 4 },
  { 6, 4, 2 },
  { 6, 2, 5 },
  { 0, 6, 5 },
};

static svtkIdType biQuadTriEdges[][2] = {
  { 0, 3 },
  { 3, 1 },
  { 1, 4 },
  { 4, 2 },
  { 2, 5 },
  { 5, 0 },
};

static svtkIdType quadTriEdges[][2] = {
  { 0, 3 },
  { 3, 1 },
  { 1, 4 },
  { 4, 2 },
  { 2, 5 },
  { 5, 0 },
};

static svtkIdType linQuadTris[][3] = {
  { 0, 1, 2 },
  { 0, 2, 3 },
};

static svtkIdType linQuadEdges[][2] = {
  { 0, 1 },
  { 1, 2 },
  { 2, 3 },
  { 3, 0 },
};

static svtkIdType quadQuadTris[][3] = {
  { 0, 4, 7 },
  { 7, 4, 8 },
  { 7, 8, 3 },
  { 3, 8, 6 },
  { 4, 1, 5 },
  { 8, 4, 5 },
  { 8, 5, 2 },
  { 2, 6, 8 },
};

static svtkIdType quadQuadEdges[][2] = {
  { 0, 4 },
  { 4, 1 },
  { 1, 5 },
  { 5, 2 },
  { 2, 6 },
  { 6, 3 },
  { 3, 7 },
  { 7, 0 },
};

static svtkIdType quadWedgeTetrahedra[][4] = {
  { 20, 15, 0, 8 },
  { 20, 15, 8, 2 },
  { 20, 15, 2, 7 },
  { 20, 15, 7, 1 },
  { 20, 15, 1, 6 },
  { 20, 15, 6, 0 },

  { 20, 16, 3, 9 },
  { 20, 16, 9, 4 },
  { 20, 16, 4, 10 },
  { 20, 16, 10, 5 },
  { 20, 16, 5, 11 },
  { 20, 16, 11, 3 },

  { 20, 17, 0, 6 },
  { 20, 17, 6, 1 },
  { 20, 17, 1, 13 },
  { 20, 17, 13, 4 },
  { 20, 17, 4, 9 },
  { 20, 17, 9, 3 },
  { 20, 17, 3, 12 },
  { 20, 17, 12, 0 },

  { 20, 18, 1, 7 },
  { 20, 18, 7, 2 },
  { 20, 18, 2, 14 },
  { 20, 18, 14, 5 },
  { 20, 18, 5, 10 },
  { 20, 18, 10, 4 },
  { 20, 18, 4, 13 },
  { 20, 18, 13, 1 },

  { 20, 19, 0, 12 },
  { 20, 19, 12, 3 },
  { 20, 19, 3, 11 },
  { 20, 19, 11, 5 },
  { 20, 19, 5, 14 },
  { 20, 19, 14, 2 },
  { 20, 19, 2, 8 },
  { 20, 19, 8, 0 },
};

static svtkIdType quadWedgeTris[][3] = {
  { 15, 0, 8 },
  { 15, 8, 2 },
  { 15, 2, 7 },
  { 15, 7, 1 },
  { 15, 1, 6 },
  { 15, 6, 0 },

  { 16, 3, 9 },
  { 16, 9, 4 },
  { 16, 4, 10 },
  { 16, 10, 5 },
  { 16, 5, 11 },
  { 16, 11, 3 },

  { 17, 0, 6 },
  { 17, 6, 1 },
  { 17, 1, 13 },
  { 17, 13, 4 },
  { 17, 4, 9 },
  { 17, 9, 3 },
  { 17, 3, 12 },
  { 17, 12, 0 },

  { 18, 1, 7 },
  { 18, 7, 2 },
  { 18, 2, 14 },
  { 18, 14, 5 },
  { 18, 5, 10 },
  { 18, 10, 4 },
  { 18, 4, 13 },
  { 18, 13, 1 },

  { 19, 0, 12 },
  { 19, 12, 3 },
  { 19, 3, 11 },
  { 19, 11, 5 },
  { 19, 5, 14 },
  { 19, 14, 2 },
  { 19, 2, 8 },
  { 19, 8, 0 },
};

static svtkIdType quadWedgeEdges[][2] = {
  { 0, 6 },
  { 6, 1 },
  { 1, 7 },
  { 7, 2 },
  { 2, 8 },
  { 8, 0 },
  { 3, 9 },
  { 9, 4 },
  { 4, 10 },
  { 10, 5 },
  { 5, 11 },
  { 11, 3 },
  { 0, 12 },
  { 12, 3 },
  { 1, 13 },
  { 13, 4 },
  { 2, 14 },
  { 14, 5 },
};

static svtkIdType linPyrTetrahedra[][4] = {
  { 0, 1, 2, 4 },
  { 0, 2, 3, 4 },
};

static svtkIdType linPyrTris[][3] = {
  { 0, 1, 2 },
  { 0, 2, 3 },
  { 0, 1, 4 },
  { 1, 2, 4 },
  { 2, 3, 4 },
  { 3, 0, 4 },
};

static svtkIdType linPyrEdges[][2] = {
  { 0, 1 },
  { 1, 2 },
  { 2, 3 },
  { 3, 0 },
  { 0, 4 },
  { 1, 4 },
  { 2, 4 },
  { 3, 4 },
};

static svtkIdType linTetTetrahedra[][4] = {
  { 0, 1, 2, 3 },
};

static svtkIdType linTetTris[][3] = {
  { 0, 2, 1 },
  { 0, 1, 3 },
  { 1, 2, 3 },
  { 2, 0, 3 },
};

static svtkIdType linTetEdges[][2] = {
  { 0, 1 },
  { 1, 2 },
  { 2, 0 },
  { 0, 3 },
  { 1, 3 },
  { 2, 3 },
};

static svtkIdType quadTetTetrahedra[][4] = {
  { 4, 7, 6, 0 },
  { 5, 6, 9, 2 },
  { 7, 8, 9, 3 },
  { 4, 5, 8, 1 },
  { 6, 8, 7, 4 },
  { 6, 8, 4, 5 },
  { 6, 8, 5, 9 },
  { 6, 8, 9, 7 },
};

static svtkIdType quadTetTris[][3] = {
  { 0, 6, 4 },
  { 4, 6, 5 },
  { 5, 6, 2 },
  { 4, 5, 1 },

  { 0, 4, 7 },
  { 7, 4, 8 },
  { 8, 4, 1 },
  { 7, 8, 3 },

  { 1, 5, 8 },
  { 8, 5, 9 },
  { 9, 5, 2 },
  { 8, 9, 3 },

  { 2, 6, 9 },
  { 9, 6, 7 },
  { 7, 6, 0 },
  { 9, 7, 3 },
};

static svtkIdType quadTetEdges[][2] = {
  { 0, 4 },
  { 4, 1 },
  { 1, 5 },
  { 5, 2 },
  { 2, 6 },
  { 6, 0 },
  { 0, 7 },
  { 7, 3 },
  { 1, 8 },
  { 8, 3 },
  { 2, 9 },
  { 9, 3 },
};

/* Each face should look like this:
 *             +-+-+
 *             |\|/|
 *             +-+-+
 *             |/|\|
 *             +-+-+
 * This tessellation is required for
 * neighboring hexes to have compatible
 * boundaries.
 */
static svtkIdType quadHexTetrahedra[][4] = {
  { 0, 8, 20, 26 },
  { 8, 1, 20, 26 },
  { 1, 9, 20, 26 },
  { 9, 2, 20, 26 },
  { 2, 10, 20, 26 },
  { 10, 3, 20, 26 },
  { 3, 11, 20, 26 },
  { 11, 0, 20, 26 },

  { 4, 15, 21, 26 },
  { 15, 7, 21, 26 },
  { 7, 14, 21, 26 },
  { 14, 6, 21, 26 },
  { 6, 13, 21, 26 },
  { 13, 5, 21, 26 },
  { 5, 12, 21, 26 },
  { 12, 4, 21, 26 },

  { 0, 16, 22, 26 },
  { 16, 4, 22, 26 },
  { 4, 12, 22, 26 },
  { 12, 5, 22, 26 },
  { 5, 17, 22, 26 },
  { 17, 1, 22, 26 },
  { 1, 8, 22, 26 },
  { 8, 0, 22, 26 },

  { 3, 10, 23, 26 },
  { 10, 2, 23, 26 },
  { 2, 18, 23, 26 },
  { 18, 6, 23, 26 },
  { 6, 14, 23, 26 },
  { 14, 7, 23, 26 },
  { 7, 19, 23, 26 },
  { 19, 3, 23, 26 },

  { 0, 11, 24, 26 },
  { 11, 3, 24, 26 },
  { 3, 19, 24, 26 },
  { 19, 7, 24, 26 },
  { 7, 15, 24, 26 },
  { 15, 4, 24, 26 },
  { 4, 16, 24, 26 },
  { 16, 0, 24, 26 },

  { 1, 17, 25, 26 },
  { 17, 5, 25, 26 },
  { 5, 13, 25, 26 },
  { 13, 6, 25, 26 },
  { 6, 18, 25, 26 },
  { 18, 2, 25, 26 },
  { 2, 9, 25, 26 },
  { 9, 1, 25, 26 },
};

static svtkIdType quadHexTris[][3] = {
  { 0, 8, 20 },
  { 8, 1, 20 },
  { 1, 9, 20 },
  { 9, 2, 20 },
  { 2, 10, 20 },
  { 10, 3, 20 },
  { 3, 11, 20 },
  { 11, 0, 20 },

  { 4, 15, 21 },
  { 15, 7, 21 },
  { 7, 14, 21 },
  { 14, 6, 21 },
  { 6, 13, 21 },
  { 13, 5, 21 },
  { 5, 12, 21 },
  { 12, 4, 21 },

  { 0, 16, 22 },
  { 16, 4, 22 },
  { 4, 12, 22 },
  { 12, 5, 22 },
  { 5, 17, 22 },
  { 17, 1, 22 },
  { 1, 8, 22 },
  { 8, 0, 22 },

  { 3, 10, 23 },
  { 10, 2, 23 },
  { 2, 18, 23 },
  { 18, 6, 23 },
  { 6, 14, 23 },
  { 14, 7, 23 },
  { 7, 19, 23 },
  { 19, 3, 23 },

  { 0, 11, 24 },
  { 11, 3, 24 },
  { 3, 19, 24 },
  { 19, 7, 24 },
  { 7, 15, 24 },
  { 15, 4, 24 },
  { 4, 16, 24 },
  { 16, 0, 24 },

  { 1, 17, 25 },
  { 17, 5, 25 },
  { 5, 13, 25 },
  { 13, 6, 25 },
  { 6, 18, 25 },
  { 18, 2, 25 },
  { 2, 9, 25 },
  { 9, 1, 25 },
};

static svtkIdType quadHexEdges[][2] = {
  { 0, 8 },
  { 8, 1 },
  { 1, 9 },
  { 9, 2 },
  { 2, 10 },
  { 10, 3 },
  { 3, 11 },
  { 11, 0 },
  { 4, 15 },
  { 15, 7 },
  { 7, 14 },
  { 14, 6 },
  { 6, 13 },
  { 13, 5 },
  { 5, 12 },
  { 12, 4 },
  { 0, 16 },
  { 16, 4 },
  { 5, 17 },
  { 17, 1 },
  { 2, 18 },
  { 18, 6 },
  { 7, 19 },
  { 19, 3 },
};

static svtkIdType quadVoxTetrahedra[][4] = {
  { 0, 8, 20, 26 },
  { 8, 1, 20, 26 },
  { 1, 9, 20, 26 },
  { 9, 3, 20, 26 },
  { 3, 10, 20, 26 },
  { 10, 2, 20, 26 },
  { 2, 11, 20, 26 },
  { 11, 0, 20, 26 },

  { 4, 15, 21, 26 },
  { 15, 6, 21, 26 },
  { 6, 14, 21, 26 },
  { 14, 7, 21, 26 },
  { 7, 13, 21, 26 },
  { 13, 5, 21, 26 },
  { 5, 12, 21, 26 },
  { 12, 4, 21, 26 },

  { 0, 16, 22, 26 },
  { 16, 4, 22, 26 },
  { 4, 12, 22, 26 },
  { 12, 5, 22, 26 },
  { 5, 17, 22, 26 },
  { 17, 1, 22, 26 },
  { 1, 8, 22, 26 },
  { 8, 0, 22, 26 },

  { 2, 10, 23, 26 },
  { 10, 3, 23, 26 },
  { 3, 18, 23, 26 },
  { 18, 7, 23, 26 },
  { 7, 14, 23, 26 },
  { 14, 6, 23, 26 },
  { 6, 19, 23, 26 },
  { 19, 2, 23, 26 },

  { 0, 11, 24, 26 },
  { 11, 2, 24, 26 },
  { 2, 19, 24, 26 },
  { 19, 6, 24, 26 },
  { 6, 15, 24, 26 },
  { 15, 4, 24, 26 },
  { 4, 16, 24, 26 },
  { 16, 0, 24, 26 },

  { 1, 17, 25, 26 },
  { 17, 5, 25, 26 },
  { 5, 13, 25, 26 },
  { 13, 7, 25, 26 },
  { 7, 18, 25, 26 },
  { 18, 3, 25, 26 },
  { 3, 9, 25, 26 },
  { 9, 1, 25, 26 },
};

static svtkIdType quadVoxTris[][3] = {
  { 0, 8, 20 },
  { 8, 1, 20 },
  { 1, 9, 20 },
  { 9, 3, 20 },
  { 3, 10, 20 },
  { 10, 2, 20 },
  { 2, 11, 20 },
  { 11, 0, 20 },

  { 4, 15, 21 },
  { 15, 6, 21 },
  { 6, 14, 21 },
  { 14, 7, 21 },
  { 7, 13, 21 },
  { 13, 5, 21 },
  { 5, 12, 21 },
  { 12, 4, 21 },

  { 0, 16, 22 },
  { 16, 4, 22 },
  { 4, 12, 22 },
  { 12, 5, 22 },
  { 5, 17, 22 },
  { 17, 1, 22 },
  { 1, 8, 22 },
  { 8, 0, 22 },

  { 2, 10, 23 },
  { 10, 3, 23 },
  { 3, 18, 23 },
  { 18, 7, 23 },
  { 7, 14, 23 },
  { 14, 6, 23 },
  { 6, 19, 23 },
  { 19, 2, 23 },

  { 0, 11, 24 },
  { 11, 2, 24 },
  { 2, 19, 24 },
  { 19, 6, 24 },
  { 6, 15, 24 },
  { 15, 4, 24 },
  { 4, 16, 24 },
  { 16, 0, 24 },

  { 1, 17, 25 },
  { 17, 5, 25 },
  { 5, 13, 25 },
  { 13, 7, 25 },
  { 7, 18, 25 },
  { 18, 3, 25 },
  { 3, 9, 25 },
  { 9, 1, 25 },
};

static svtkIdType quadVoxEdges[][2] = {
  { 0, 8 },
  { 8, 1 },
  { 1, 9 },
  { 9, 3 },
  { 3, 10 },
  { 10, 2 },
  { 2, 11 },
  { 11, 0 },
  { 4, 15 },
  { 15, 6 },
  { 6, 14 },
  { 14, 7 },
  { 7, 13 },
  { 13, 5 },
  { 5, 12 },
  { 12, 4 },
  { 0, 16 },
  { 16, 4 },
  { 5, 17 },
  { 17, 1 },
  { 3, 18 },
  { 18, 7 },
  { 6, 19 },
  { 19, 2 },
};

// This is used by the Execute() method to avoid printing out one
// "Not Supported" error message per cell. Instead, we print one
// per Execute().
static int svtkNotSupportedErrorPrinted = 0;
static int svtkTessellatorHasPolys = 0;

// ========================================
// the meat of the class: execution!
int svtkTessellatorFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int dummySubId = -1;
  int p;

  svtkNotSupportedErrorPrinted = 0;

  // get the output info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataSet* mesh = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkSmartPointer<svtkUnstructuredGrid> tmpOut;
  if (this->MergePoints)
  {
    tmpOut = svtkSmartPointer<svtkUnstructuredGrid>::New();
  }
  else
  {
    tmpOut = output;
  }

  this->SetupOutput(mesh, tmpOut);

  this->Subdivider->SetMesh(mesh);
  this->Tessellator->SetVertexCallback(AddAPoint);
  this->Tessellator->SetEdgeCallback(AddALine);
  this->Tessellator->SetTriangleCallback(AddATriangle);
  this->Tessellator->SetTetrahedronCallback(AddATetrahedron);
  this->Tessellator->SetPrivateData(this);

  svtkIdType cell = 0;
  int nprim = 0;
  svtkIdType* outconn = nullptr;
  double pts[27][11 + svtkStreamingTessellator::MaxFieldSize];
  int c;
  svtkIdType numCells = mesh->GetNumberOfCells();
  int progress = 0;
  int progMax = this->MergePoints ? 50 : 100;
  svtkIdType deltaProg = numCells / progMax + 1; // the extra + 1 means we always reach the end
  svtkIdType progCells = 0;

  svtkTessellatorHasPolys = 0; // print error message once per invocation, if needed
  for (progress = 0; progress < progMax; ++progress)
  {
    progCells += deltaProg;
    for (; (cell < progCells) && (cell < numCells); ++cell)
    {
      const svtkIdType nextOutCellId = this->OutputMesh->GetNumberOfCells();

      this->Subdivider->SetCellId(cell);

      svtkCell* cp = this->Subdivider->GetCell(); // We set the cell ID, get the svtkCell pointer
      int np = cp->GetCellType();
      std::vector<double> weights(cp->GetNumberOfPoints());
      double* pcoord = cp->GetParametricCoords();
      if (!pcoord || np == SVTK_POLYGON || np == SVTK_TRIANGLE_STRIP || np == SVTK_CONVEX_POINT_SET ||
        np == SVTK_POLY_LINE || np == SVTK_POLY_VERTEX || np == SVTK_POLYHEDRON ||
        np == SVTK_QUADRATIC_POLYGON)
      {
        if (!svtkTessellatorHasPolys)
        {
          svtkWarningMacro("Input dataset has cells without parameterizations "
                          "(SVTK_POLYGON,SVTK_POLY_LINE,SVTK_POLY_VERTEX,SVTK_TRIANGLE_STRIP,SVTK_"
                          "CONVEX_POINT_SET,SVTK_QUADRATIC_POLYGON). "
                          "They will be ignored. Use svtkTriangleFilter, svtkTetrahedralize, etc. to "
                          "parameterize them first.");
          svtkTessellatorHasPolys = 1;
        }
        continue;
      }
      double* gcoord;
      svtkDataArray* field;
      for (p = 0; p < (cp->GetNumberOfPoints() < 27 ? cp->GetNumberOfPoints() : 27); ++p)
      {
        gcoord = cp->Points->GetPoint(p);
        for (c = 0; c < 3; ++c, ++gcoord, ++pcoord)
        {
          pts[p][c] = *gcoord;
          pts[p][c + 3] = *pcoord;
        }
        // fill in field data
        const int* offsets = this->Subdivider->GetFieldOffsets();
        for (int f = 0; f < this->Subdivider->GetNumberOfFields(); ++f)
        {
          field = mesh->GetPointData()->GetArray(this->Subdivider->GetFieldIds()[f]);
          double* tuple = field->GetTuple(cp->GetPointId(p));
          for (c = 0; c < field->GetNumberOfComponents(); ++c)
          {
            pts[p][6 + offsets[f] + c] = tuple[c];
          }
        }
      }
      int dim = this->OutputDimension;
      // Tessellate each cell:
      switch (cp->GetCellType())
      {
        case SVTK_VERTEX:
          dim = 0;
          outconn = nullptr;
          nprim = 1;
          break;
        case SVTK_LINE:
          dim = 1;
          outconn = &linEdgeEdges[0][0];
          nprim = sizeof(linEdgeEdges) / sizeof(linEdgeEdges[0]);
          break;
        case SVTK_TRIANGLE:
          if (dim > 1)
          {
            dim = 2;
            outconn = &linTriTris[0][0];
            nprim = sizeof(linTriTris) / sizeof(linTriTris[0]);
          }
          else
          {
            outconn = &linTriEdges[0][0];
            nprim = sizeof(linTriEdges) / sizeof(linTriEdges[0]);
          }
          break;
        case SVTK_QUAD:
          if (dim > 1)
          {
            dim = 2;
            outconn = &linQuadTris[0][0];
            nprim = sizeof(linQuadTris) / sizeof(linQuadTris[0]);
          }
          else
          {
            outconn = &linQuadEdges[0][0];
            nprim = sizeof(linQuadEdges) / sizeof(linQuadEdges[0]);
          }
          break;
        case SVTK_TETRA:
          if (dim == 3)
          {
            outconn = &linTetTetrahedra[0][0];
            nprim = sizeof(linTetTetrahedra) / sizeof(linTetTetrahedra[0]);
          }
          else if (dim == 2)
          {
            outconn = &linTetTris[0][0];
            nprim = sizeof(linTetTris) / sizeof(linTetTris[0]);
          }
          else
          {
            outconn = &linTetEdges[0][0];
            nprim = sizeof(linTetEdges) / sizeof(linTetEdges[0]);
          }
          break;
        case SVTK_WEDGE:
        case SVTK_LAGRANGE_WEDGE:
        case SVTK_BEZIER_WEDGE:
          // We sample additional points to get compatible triangulations
          // with neighboring hexes, tets, etc.
          for (p = 6; p < 21; ++p)
          {
            dummySubId = -1;
            for (int y = 0; y < 3; ++y)
            {
              pts[p][y + 3] = extraWedgeParams[p - 6][y];
            }
            cp->EvaluateLocation(dummySubId, pts[p] + 3, pts[p], weights.data());
            this->Subdivider->EvaluateFields(pts[p], weights.data(), 6);
          }
          if (dim == 3)
          {
            outconn = &quadWedgeTetrahedra[0][0];
            nprim = sizeof(quadWedgeTetrahedra) / sizeof(quadWedgeTetrahedra[0]);
          }
          else if (dim == 2)
          {
            outconn = &quadWedgeTris[0][0];
            nprim = sizeof(quadWedgeTris) / sizeof(quadWedgeTris[0]);
          }
          else
          {
            outconn = &quadWedgeEdges[0][0];
            nprim = sizeof(quadWedgeEdges) / sizeof(quadWedgeEdges[0]);
          }
          break;
        case SVTK_PYRAMID:
          if (dim == 3)
          {
            outconn = &linPyrTetrahedra[0][0];
            nprim = sizeof(linPyrTetrahedra) / sizeof(linPyrTetrahedra[0]);
          }
          else if (dim == 2)
          {
            outconn = &linPyrTris[0][0];
            nprim = sizeof(linPyrTris) / sizeof(linPyrTris[0]);
          }
          else
          {
            outconn = &linPyrEdges[0][0];
            nprim = sizeof(linPyrEdges) / sizeof(linPyrEdges[0]);
          }
          break;
        case SVTK_LAGRANGE_CURVE:
        case SVTK_BEZIER_CURVE:
          // Lagrange/Bezier curves may bound other elements which we
          // normally only divide in 2 along an axis, so only
          // start by dividing the curve in 2 instead of adding
          // each interior point to the approximation:
          dummySubId = -1;
          for (int y = 0; y < 3; ++y)
          {
            pts[2][y + 3] = extraLagrangeCurveParams[y];
          }
          cp->EvaluateLocation(dummySubId, pts[2] + 3, pts[2], weights.data());
          this->Subdivider->EvaluateFields(pts[2], weights.data(), 6);
          SVTK_FALLTHROUGH;
        case SVTK_QUADRATIC_EDGE:
          dim = 1;
          outconn = &quadEdgeEdges[0][0];
          nprim = sizeof(quadEdgeEdges) / sizeof(quadEdgeEdges[0]);
          break;
        case SVTK_CUBIC_LINE:
          dim = 1;
          outconn = &cubicLinEdges[0][0];
          nprim = sizeof(cubicLinEdges) / sizeof(cubicLinEdges[0]);
          break;
        case SVTK_LAGRANGE_TRIANGLE:
        case SVTK_BEZIER_TRIANGLE:
          for (p = 3; p < 6; ++p)
          {
            dummySubId = -1;
            for (int y = 0; y < 3; ++y)
            {
              pts[p][y + 3] = extraLagrangeTriParams[p - 3][y];
            }
            cp->EvaluateLocation(dummySubId, pts[p] + 3, pts[p], weights.data());
            this->Subdivider->EvaluateFields(pts[p], weights.data(), 6);
          }
          SVTK_FALLTHROUGH;
        case SVTK_QUADRATIC_TRIANGLE:
          if (dim > 1)
          {
            dim = 2;
            outconn = &quadTriTris[0][0];
            nprim = sizeof(quadTriTris) / sizeof(quadTriTris[0]);
          }
          else
          {
            outconn = &quadTriEdges[0][0];
            nprim = sizeof(quadTriEdges) / sizeof(quadTriEdges[0]);
          }
          break;
        case SVTK_BIQUADRATIC_TRIANGLE:
          if (dim > 1)
          {
            dim = 2;
            outconn = &biQuadTriTris[0][0];
            nprim = sizeof(biQuadTriTris) / sizeof(biQuadTriTris[0]);
          }
          else
          {
            outconn = &biQuadTriEdges[0][0];
            nprim = sizeof(biQuadTriEdges) / sizeof(biQuadTriEdges[0]);
          }
          break;
        case SVTK_LAGRANGE_QUADRILATERAL:
        case SVTK_BEZIER_QUADRILATERAL:
          // Arbitrary-order Lagrange elements may not have mid-edge nodes
          // (they may be more finely divided), so evaluate to match fixed
          // connectivity of our starting output.
          {
            int mm = static_cast<int>(
              sizeof(extraLagrangeQuadParams) / sizeof(extraLagrangeQuadParams[0]));
            for (int nn = 0; nn < mm; ++nn)
            {
              for (c = 0; c < 3; ++c)
              {
                pts[4 + nn][c + 3] = extraLagrangeQuadParams[nn][c];
              }
              cp->EvaluateLocation(dummySubId, pts[4 + nn] + 3, pts[4 + nn], weights.data());
              this->Subdivider->EvaluateFields(pts[4 + nn], weights.data(), 6);
            }
          }
          SVTK_FALLTHROUGH;
        case SVTK_BIQUADRATIC_QUAD:
        case SVTK_QUADRATIC_QUAD:
          for (c = 0; c < 3; ++c)
          {
            pts[8][c + 3] = extraQuadQuadParams[0][c];
          }
          cp->EvaluateLocation(dummySubId, pts[8] + 3, pts[8], weights.data());
          this->Subdivider->EvaluateFields(pts[8], weights.data(), 6);
          if (dim > 1)
          {
            dim = 2;
            outconn = &quadQuadTris[0][0];
            nprim = sizeof(quadQuadTris) / sizeof(quadQuadTris[0]);
          }
          else
          {
            outconn = &quadQuadEdges[0][0];
            nprim = sizeof(quadQuadEdges) / sizeof(quadQuadEdges[0]);
          }
          break;
        case SVTK_LAGRANGE_TETRAHEDRON:
        case SVTK_BEZIER_TETRAHEDRON:
          for (p = 4; p < 10; ++p)
          {
            dummySubId = -1;
            for (int y = 0; y < 3; ++y)
            {
              pts[p][y + 3] = extraLagrangeTetraParams[p - 4][y];
            }
            cp->EvaluateLocation(dummySubId, pts[p] + 3, pts[p], weights.data());
            this->Subdivider->EvaluateFields(pts[p], weights.data(), 6);
          }
          SVTK_FALLTHROUGH;
        case SVTK_QUADRATIC_TETRA:
          if (dim == 3)
          {
            outconn = &quadTetTetrahedra[0][0];
            nprim = sizeof(quadTetTetrahedra) / sizeof(quadTetTetrahedra[0]);
          }
          else if (dim == 2)
          {
            outconn = &quadTetTris[0][0];
            nprim = sizeof(quadTetTris) / sizeof(quadTetTris[0]);
          }
          else
          {
            outconn = &quadTetEdges[0][0];
            nprim = sizeof(quadTetEdges) / sizeof(quadTetEdges[0]);
          }
          break;
        case SVTK_HEXAHEDRON:
        case SVTK_LAGRANGE_HEXAHEDRON:
        case SVTK_BEZIER_HEXAHEDRON:
          // we sample 19 extra points to guarantee a compatible tetrahedralization
          for (p = 8; p < 20; ++p)
          {
            dummySubId = -1;
            for (int y = 0; y < 3; ++y)
            {
              pts[p][y + 3] = extraLinHexParams[p - 8][y];
            }
            cp->EvaluateLocation(dummySubId, pts[p] + 3, pts[p], weights.data());
            this->Subdivider->EvaluateFields(pts[p], weights.data(), 6);
          }
          SVTK_FALLTHROUGH;
        case SVTK_QUADRATIC_HEXAHEDRON:
          for (p = 20; p < 27; ++p)
          {
            dummySubId = -1;
            for (int x = 0; x < 3; ++x)
            {
              pts[p][x + 3] = extraQuadHexParams[p - 20][x];
            }
            cp->EvaluateLocation(dummySubId, pts[p] + 3, pts[p], weights.data());
            this->Subdivider->EvaluateFields(pts[p], weights.data(), 6);
          }
          if (dim == 3)
          {
            outconn = &quadHexTetrahedra[0][0];
            nprim = sizeof(quadHexTetrahedra) / sizeof(quadHexTetrahedra[0]);
          }
          else if (dim == 2)
          {
            outconn = &quadHexTris[0][0];
            nprim = sizeof(quadHexTris) / sizeof(quadHexTris[0]);
          }
          else
          {
            outconn = &quadHexEdges[0][0];
            nprim = sizeof(quadHexEdges) / sizeof(quadHexEdges[0]);
          }
          break;
        case SVTK_VOXEL:
          // we sample 19 extra points to guarantee a compatible tetrahedralization
          for (p = 8; p < 20; ++p)
          {
            dummySubId = -1;
            for (int y = 0; y < 3; ++y)
            {
              pts[p][y + 3] = extraLinHexParams[p - 8][y];
            }
            cp->EvaluateLocation(dummySubId, pts[p] + 3, pts[p], weights.data());
            this->Subdivider->EvaluateFields(pts[p], weights.data(), 6);
          }
          for (p = 20; p < 27; ++p)
          {
            dummySubId = -1;
            for (int x = 0; x < 3; ++x)
            {
              pts[p][x + 3] = extraQuadHexParams[p - 20][x];
            }
            cp->EvaluateLocation(dummySubId, pts[p] + 3, pts[p], weights.data());
            this->Subdivider->EvaluateFields(pts[p], weights.data(), 6);
          }
          if (dim == 3)
          {
            outconn = &quadVoxTetrahedra[0][0];
            nprim = sizeof(quadVoxTetrahedra) / sizeof(quadVoxTetrahedra[0]);
          }
          else if (dim == 2)
          {
            outconn = &quadVoxTris[0][0];
            nprim = sizeof(quadVoxTris) / sizeof(quadVoxTris[0]);
          }
          else
          {
            outconn = &quadVoxEdges[0][0];
            nprim = sizeof(quadVoxEdges) / sizeof(quadVoxEdges[0]);
          }
          break;
        case SVTK_PIXEL:
          dim = -1;
          if (!svtkNotSupportedErrorPrinted)
          {
            svtkNotSupportedErrorPrinted = 1;
            svtkWarningMacro("Oops, pixels are not supported");
          }
          break;
        default:
          dim = -1;
          if (!svtkNotSupportedErrorPrinted)
          {
            svtkNotSupportedErrorPrinted = 1;
            svtkWarningMacro("Oops, some cell type (" << cp->GetCellType() << ") not supported");
          }
      }

      // OK, now output the primitives
      int tet, tri, edg;
      switch (dim)
      {
        case 3:
          for (tet = 0; tet < nprim; ++tet, outconn += 4)
          {
            this->Tessellator->AdaptivelySample3Facet(
              pts[outconn[0]], pts[outconn[1]], pts[outconn[2]], pts[outconn[3]]);
          }
          break;
        case 2:
          for (tri = 0; tri < nprim; ++tri, outconn += 3)
          {
            this->Tessellator->AdaptivelySample2Facet(
              pts[outconn[0]], pts[outconn[1]], pts[outconn[2]]);
          }
          break;
        case 1:
          for (edg = 0; edg < nprim; ++edg, outconn += 2)
          {
            this->Tessellator->AdaptivelySample1Facet(pts[outconn[0]], pts[outconn[1]]);
          }
          break;
        case 0:
          this->Tessellator->AdaptivelySample0Facet(pts[0]);
          break;
        default:
          // do nothing
          break;
      }

      // Copy cell data.
      svtkCopyTuples(mesh->GetCellData(), cell, this->OutputMesh->GetCellData(), nextOutCellId,
        this->OutputMesh->GetNumberOfCells());
    }
    this->UpdateProgress((double)(progress / 100.));
  }

  if (this->MergePoints)
  {
    this->MergeOutputPoints(tmpOut, output);
  }
  output->Squeeze();
  this->Teardown();

  return 1;
}

//----------------------------------------------------------------------------
int svtkTessellatorFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}
