/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellQuality.h"
#include "svtk_verdict.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMeshQuality.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"
#include <utility>

svtkStandardNewMacro(svtkCellQuality);

double svtkCellQuality::CurrentTriNormal[3];

svtkCellQuality::~svtkCellQuality()
{
  this->PointIds->Delete();
  this->Points->Delete();
}

svtkCellQuality::svtkCellQuality()
{
  this->QualityMeasure = NONE;
  this->UnsupportedGeometry = -1;
  this->UndefinedQuality = -1;
  this->PointIds = svtkIdList::New();
  this->Points = svtkPoints::New();
}

void svtkCellQuality::PrintSelf(ostream& os, svtkIndent indent)
{
  static const char* CellQualityMeasureNames[] = {
    "None",
    "Area",
    "AspectBeta",
    "AspectFrobenius",
    "AspectGamma",
    "AspectRatio",
    "CollapseRatio",
    "Condition",
    "Diagonal",
    "Dimension",
    "Distortion",
    "EdgeRatio",
    "Jacobian",
    "MaxAngle",
    "MaxAspectFrobenius",
    "MaxEdgeRatio",
    "MedAspectFrobenius",
    "MinAngle",
    "Oddy",
    "RadiusRatio",
    "RelativeSizeSquared",
    "ScaledJacobian",
    "Shape",
    "ShapeAndSize",
    "Shear",
    "ShearAndSize",
    "Skew",
    "Stretch",
    "Taper",
    "Volume",
    "Warpage",
  };

  const char* name = CellQualityMeasureNames[this->QualityMeasure];
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TriangleQualityMeasure : " << name << endl;
  os << indent << "QuadQualityMeasure : " << name << endl;
  os << indent << "TetQualityMeasure : " << name << endl;
  os << indent << "HexQualityMeasure : " << name << endl;
  os << indent << "TriangleStripQualityMeasure : " << name << endl;
  os << indent << "PixelQualityMeasure : " << name << endl;

  os << indent << "UnsupportedGeometry : " << this->UnsupportedGeometry << endl;
  os << indent << "UndefinedQuality : " << this->UndefinedQuality << endl;
}

int svtkCellQuality::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* in = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* out = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Copy input to get a start point
  out->ShallowCopy(in);

  // Allocate storage for cell quality
  svtkIdType const nCells = in->GetNumberOfCells();
  svtkSmartPointer<svtkDoubleArray> quality = svtkSmartPointer<svtkDoubleArray>::New();
  quality->SetName("CellQuality");
  quality->SetNumberOfValues(nCells);

  svtkDataArray* CellNormals = in->GetCellData()->GetNormals();
  if (CellNormals)
  {
    v_set_tri_normal_func(
      reinterpret_cast<ComputeNormal>(svtkCellQuality::GetCurrentTriangleNormal));
  }
  else
  {
    v_set_tri_normal_func(nullptr);
  }

  // Support progress and abort.
  svtkIdType const tenth = (nCells >= 10 ? nCells / 10 : 1);
  double const nCellInv = 1. / nCells;

  // Actual computation of the selected quality
  for (svtkIdType cid = 0; cid < nCells; ++cid)
  {
    // Periodically update progress and check for an abort request.
    if (0 == cid % tenth)
    {
      this->UpdateProgress((cid + 1) * nCellInv);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    svtkCell* cell = out->GetCell(cid);
    double q = 0;
    switch (cell->GetCellType())
    {
      default:
        q = this->GetUnsupportedGeometry();
        break;

      // Below are supported types. Please be noted that not every quality is
      // defined for all supported geometries. For those quality that is not
      // defined with respective to a particular cell type,
      // this->GetUndefinedQuality() is returned.
      case SVTK_TRIANGLE:
        if (CellNormals)
          CellNormals->GetTuple(cid, svtkCellQuality::CurrentTriNormal);
        q = this->ComputeTriangleQuality(cell);
        break;
      case SVTK_TRIANGLE_STRIP:
        q = this->ComputeTriangleStripQuality(cell);
        break;
      case SVTK_PIXEL:
        q = this->ComputePixelQuality(cell);
        break;
      case SVTK_QUAD:
        q = this->ComputeQuadQuality(cell);
        break;
      case SVTK_TETRA:
        q = this->ComputeTetQuality(cell);
        break;
      case SVTK_HEXAHEDRON:
        q = this->ComputeHexQuality(cell);
        break;
    }
    quality->SetValue(cid, q);
  }

  out->GetCellData()->AddArray(quality);
  out->GetCellData()->SetActiveAttribute("CellQuality", svtkDataSetAttributes::SCALARS);

  return 1;
}

double svtkCellQuality::ComputeTriangleQuality(svtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA:
      return svtkMeshQuality::TriangleArea(cell);
    case ASPECT_FROBENIUS:
      return svtkMeshQuality::TriangleAspectFrobenius(cell);
    case ASPECT_RATIO:
      return svtkMeshQuality::TriangleAspectRatio(cell);
    case CONDITION:
      return svtkMeshQuality::TriangleCondition(cell);
    case DISTORTION:
      return svtkMeshQuality::TriangleDistortion(cell);
    case EDGE_RATIO:
      return svtkMeshQuality::TriangleEdgeRatio(cell);
    case MAX_ANGLE:
      return svtkMeshQuality::TriangleMaxAngle(cell);
    case MIN_ANGLE:
      return svtkMeshQuality::TriangleMinAngle(cell);
    case RADIUS_RATIO:
      return svtkMeshQuality::TriangleRadiusRatio(cell);
    case RELATIVE_SIZE_SQUARED:
      return svtkMeshQuality::TriangleRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:
      return svtkMeshQuality::TriangleScaledJacobian(cell);
    case SHAPE_AND_SIZE:
      return svtkMeshQuality::TriangleShapeAndSize(cell);
    case SHAPE:
      return svtkMeshQuality::TriangleShape(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

double svtkCellQuality::ComputeQuadQuality(svtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA:
      return svtkMeshQuality::QuadArea(cell);
    case ASPECT_RATIO:
      return svtkMeshQuality::QuadAspectRatio(cell);
    case CONDITION:
      return svtkMeshQuality::QuadCondition(cell);
    case DISTORTION:
      return svtkMeshQuality::QuadDistortion(cell);
    case EDGE_RATIO:
      return svtkMeshQuality::QuadEdgeRatio(cell);
    case JACOBIAN:
      return svtkMeshQuality::QuadJacobian(cell);
    case MAX_ANGLE:
      return svtkMeshQuality::QuadMaxAngle(cell);
    case MAX_ASPECT_FROBENIUS:
      return svtkMeshQuality::QuadMaxAspectFrobenius(cell);
    case MAX_EDGE_RATIO:
      return svtkMeshQuality::QuadMaxEdgeRatios(cell);
    case MED_ASPECT_FROBENIUS:
      return svtkMeshQuality::QuadMedAspectFrobenius(cell);
    case MIN_ANGLE:
      return svtkMeshQuality::QuadMinAngle(cell);
    case ODDY:
      return svtkMeshQuality::QuadOddy(cell);
    case RADIUS_RATIO:
      return svtkMeshQuality::QuadRadiusRatio(cell);
    case RELATIVE_SIZE_SQUARED:
      return svtkMeshQuality::QuadRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:
      return svtkMeshQuality::QuadScaledJacobian(cell);
    case SHAPE_AND_SIZE:
      return svtkMeshQuality::QuadShapeAndSize(cell);
    case SHAPE:
      return svtkMeshQuality::QuadShape(cell);
    case SHEAR_AND_SIZE:
      return svtkMeshQuality::QuadShearAndSize(cell);
    case SHEAR:
      return svtkMeshQuality::QuadShear(cell);
    case SKEW:
      return svtkMeshQuality::QuadSkew(cell);
    case STRETCH:
      return svtkMeshQuality::QuadStretch(cell);
    case TAPER:
      return svtkMeshQuality::QuadTaper(cell);
    case WARPAGE:
      return svtkMeshQuality::QuadWarpage(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

double svtkCellQuality::ComputeTetQuality(svtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case ASPECT_BETA:
      return svtkMeshQuality::TetAspectBeta(cell);
    case ASPECT_FROBENIUS:
      return svtkMeshQuality::TetAspectFrobenius(cell);
    case ASPECT_GAMMA:
      return svtkMeshQuality::TetAspectGamma(cell);
    case ASPECT_RATIO:
      return svtkMeshQuality::TetAspectRatio(cell);
    case COLLAPSE_RATIO:
      return svtkMeshQuality::TetCollapseRatio(cell);
    case CONDITION:
      return svtkMeshQuality::TetCondition(cell);
    case DISTORTION:
      return svtkMeshQuality::TetDistortion(cell);
    case EDGE_RATIO:
      return svtkMeshQuality::TetEdgeRatio(cell);
    case JACOBIAN:
      return svtkMeshQuality::TetJacobian(cell);
    case MIN_ANGLE:
      return svtkMeshQuality::TetMinAngle(cell);
    case RADIUS_RATIO:
      return svtkMeshQuality::TetRadiusRatio(cell);
    case RELATIVE_SIZE_SQUARED:
      return svtkMeshQuality::TetRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:
      return svtkMeshQuality::TetScaledJacobian(cell);
    case SHAPE_AND_SIZE:
      return svtkMeshQuality::TetShapeandSize(cell);
    case SHAPE:
      return svtkMeshQuality::TetShape(cell);
    case VOLUME:
      return svtkMeshQuality::TetVolume(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

double svtkCellQuality::ComputeHexQuality(svtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case CONDITION:
      return svtkMeshQuality::HexCondition(cell);
    case DIAGONAL:
      return svtkMeshQuality::HexDiagonal(cell);
    case DIMENSION:
      return svtkMeshQuality::HexDimension(cell);
    case DISTORTION:
      return svtkMeshQuality::HexDistortion(cell);
    case EDGE_RATIO:
      return svtkMeshQuality::HexEdgeRatio(cell);
    case JACOBIAN:
      return svtkMeshQuality::HexJacobian(cell);
    case MAX_ASPECT_FROBENIUS:
      return svtkMeshQuality::HexMaxAspectFrobenius(cell);
    case MAX_EDGE_RATIO:
      return svtkMeshQuality::HexMaxEdgeRatio(cell);
    case MED_ASPECT_FROBENIUS:
      return svtkMeshQuality::HexMedAspectFrobenius(cell);
    case ODDY:
      return svtkMeshQuality::HexOddy(cell);
    case RELATIVE_SIZE_SQUARED:
      return svtkMeshQuality::HexRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:
      return svtkMeshQuality::HexScaledJacobian(cell);
    case SHAPE_AND_SIZE:
      return svtkMeshQuality::HexShapeAndSize(cell);
    case SHAPE:
      return svtkMeshQuality::HexShape(cell);
    case SHEAR_AND_SIZE:
      return svtkMeshQuality::HexShearAndSize(cell);
    case SHEAR:
      return svtkMeshQuality::HexShear(cell);
    case SKEW:
      return svtkMeshQuality::HexSkew(cell);
    case STRETCH:
      return svtkMeshQuality::HexStretch(cell);
    case TAPER:
      return svtkMeshQuality::HexTaper(cell);
    case VOLUME:
      return svtkMeshQuality::HexVolume(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

double svtkCellQuality::ComputeTriangleStripQuality(svtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA:
      return svtkCellQuality::TriangleStripArea(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

double svtkCellQuality::ComputePixelQuality(svtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA:
      return svtkCellQuality::PixelArea(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

int svtkCellQuality::GetCurrentTriangleNormal(double[3], double normal[3])
{
  // copy the cell normal
  normal[0] = svtkCellQuality::CurrentTriNormal[0];
  normal[1] = svtkCellQuality::CurrentTriNormal[1];
  normal[2] = svtkCellQuality::CurrentTriNormal[2];
  return 1;
}

// Triangle strip quality metrics

double svtkCellQuality::TriangleStripArea(svtkCell* cell)
{
  return this->PolygonArea(cell);
}

// Pixel quality metrics

double svtkCellQuality::PixelArea(svtkCell* cell)
{
  return this->PolygonArea(cell);
}

// Polygon quality metrics

double svtkCellQuality::PolygonArea(svtkCell* cell)
{
  cell->Triangulate(0, this->PointIds, this->Points);
  double abc[3][3], quality = 0;
  for (svtkIdType i = 0, n = this->Points->GetNumberOfPoints(); i < n; i += 3)
  {
    this->Points->GetPoint(i + 0, abc[0]);
    this->Points->GetPoint(i + 1, abc[1]);
    this->Points->GetPoint(i + 2, abc[2]);
    quality += svtkTriangle::TriangleArea(abc[0], abc[1], abc[2]);
  }
  return quality;
}
