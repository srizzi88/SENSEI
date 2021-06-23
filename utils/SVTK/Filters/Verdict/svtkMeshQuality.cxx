/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMeshQuality.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2003-2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: dcthomp@sandia.gov,pppebay@sandia.gov

=========================================================================*/
#include "svtkMeshQuality.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"

#include "svtk_verdict.h"

svtkStandardNewMacro(svtkMeshQuality);

typedef double (*CellQualityType)(svtkCell*);

double TetVolume(svtkCell* cell);

static const char* QualityMeasureNames[] = { "EdgeRatio", "AspectRatio", "RadiusRatio",
  "AspectFrobenius", "MedAspectFrobenius", "MaxAspectFrobenius", "MinAngle", "CollapseRatio",
  "MaxAngle", "Condition", "ScaledJacobian", "Shear", "RelativeSizeSquared", "Shape",
  "ShapeAndSize", "Distortion", "MaxEdgeRatio", "Skew", "Taper", "Volume", "Stretch", "Diagonal",
  "Dimension", "Oddy", "ShearAndSize", "Jacobian", "Warpage", "AspectGamma", "Area", "AspectBeta" };

double svtkMeshQuality::CurrentTriNormal[3];

void svtkMeshQuality::PrintSelf(ostream& os, svtkIndent indent)
{
  const char onStr[] = "On";
  const char offStr[] = "Off";

  this->Superclass::PrintSelf(os, indent);

  os << indent << "SaveCellQuality:   " << (this->SaveCellQuality ? onStr : offStr) << endl;
  os << indent << "TriangleQualityMeasure: " << QualityMeasureNames[this->TriangleQualityMeasure]
     << endl;
  os << indent << "QuadQualityMeasure: " << QualityMeasureNames[this->QuadQualityMeasure] << endl;
  os << indent << "TetQualityMeasure: " << QualityMeasureNames[this->TetQualityMeasure] << endl;
  os << indent << "HexQualityMeasure: " << QualityMeasureNames[this->HexQualityMeasure] << endl;
  os << indent << "Volume: " << (this->Volume ? onStr : offStr) << endl;
  os << indent << "CompatibilityMode: " << (this->CompatibilityMode ? onStr : offStr) << endl;
}

svtkMeshQuality::svtkMeshQuality()
{
  this->SaveCellQuality = 1; // Default is On
  this->TriangleQualityMeasure = SVTK_QUALITY_ASPECT_RATIO;
  this->QuadQualityMeasure = SVTK_QUALITY_EDGE_RATIO;
  this->TetQualityMeasure = SVTK_QUALITY_ASPECT_RATIO;
  this->HexQualityMeasure = SVTK_QUALITY_MAX_ASPECT_FROBENIUS;
  this->Volume = 0;
  this->CompatibilityMode = 0;
}

svtkMeshQuality::~svtkMeshQuality()
{
  // Nothing yet.
}

int svtkMeshQuality::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* in = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* out = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  CellQualityType TriangleQuality, QuadQuality, TetQuality, HexQuality;
  svtkDoubleArray* quality = nullptr;
  svtkDoubleArray* volume = nullptr;
  svtkIdType N = in->GetNumberOfCells();
  double qtrim, qtriM, Eqtri, Eqtri2;
  double qquam, qquaM, Eqqua, Eqqua2;
  double qtetm, qtetM, Eqtet, Eqtet2;
  double qhexm, qhexM, Eqhex, Eqhex2;
  double q;
  double V = 0.;
  svtkIdType ntri = 0;
  svtkIdType nqua = 0;
  svtkIdType ntet = 0;
  svtkIdType nhex = 0;
  svtkCell* cell;
  int progressNumer = 0;
  double progressDenom = 20.;

  this->CellNormals = in->GetCellData()->GetNormals();

  if (this->CellNormals)
    v_set_tri_normal_func(
      reinterpret_cast<ComputeNormal>(svtkMeshQuality::GetCurrentTriangleNormal));
  else
    v_set_tri_normal_func(nullptr);

  // Initialize the min and max values, std deviations, etc.
  qtriM = qquaM = qtetM = qhexM = SVTK_DOUBLE_MIN;
  qtrim = qquam = qtetm = qhexm = SVTK_DOUBLE_MAX;
  Eqtri = Eqtri2 = Eqqua = Eqqua2 = Eqtet = Eqtet2 = Eqhex = Eqhex2 = 0.;

  switch (this->GetTriangleQualityMeasure())
  {
    case SVTK_QUALITY_AREA:
      TriangleQuality = TriangleArea;
      break;
    case SVTK_QUALITY_EDGE_RATIO:
      TriangleQuality = TriangleEdgeRatio;
      break;
    case SVTK_QUALITY_ASPECT_RATIO:
      TriangleQuality = TriangleAspectRatio;
      break;
    case SVTK_QUALITY_RADIUS_RATIO:
      TriangleQuality = TriangleRadiusRatio;
      break;
    case SVTK_QUALITY_ASPECT_FROBENIUS:
      TriangleQuality = TriangleAspectFrobenius;
      break;
    case SVTK_QUALITY_MIN_ANGLE:
      TriangleQuality = TriangleMinAngle;
      break;
    case SVTK_QUALITY_MAX_ANGLE:
      TriangleQuality = TriangleMaxAngle;
      break;
    case SVTK_QUALITY_CONDITION:
      TriangleQuality = TriangleCondition;
      break;
    case SVTK_QUALITY_SCALED_JACOBIAN:
      TriangleQuality = TriangleScaledJacobian;
      break;
    case SVTK_QUALITY_RELATIVE_SIZE_SQUARED:
      TriangleQuality = TriangleRelativeSizeSquared;
      break;
    case SVTK_QUALITY_SHAPE:
      TriangleQuality = TriangleShape;
      break;
    case SVTK_QUALITY_SHAPE_AND_SIZE:
      TriangleQuality = TriangleShapeAndSize;
      break;
    case SVTK_QUALITY_DISTORTION:
      TriangleQuality = TriangleDistortion;
      break;
    default:
      svtkWarningMacro("Bad TriangleQualityMeasure (" << this->GetTriangleQualityMeasure()
                                                     << "), using RadiusRatio instead");
      TriangleQuality = TriangleRadiusRatio;
      break;
  }

  switch (this->GetQuadQualityMeasure())
  {
    case SVTK_QUALITY_EDGE_RATIO:
      QuadQuality = QuadEdgeRatio;
      break;
    case SVTK_QUALITY_ASPECT_RATIO:
      QuadQuality = QuadAspectRatio;
      break;
    case SVTK_QUALITY_RADIUS_RATIO:
      QuadQuality = QuadRadiusRatio;
      break;
    case SVTK_QUALITY_MED_ASPECT_FROBENIUS:
      QuadQuality = QuadMedAspectFrobenius;
      break;
    case SVTK_QUALITY_MAX_ASPECT_FROBENIUS:
      QuadQuality = QuadMaxAspectFrobenius;
      break;
    case SVTK_QUALITY_MIN_ANGLE:
      QuadQuality = QuadMinAngle;
      break;
    case SVTK_QUALITY_MAX_EDGE_RATIO:
      QuadQuality = QuadMaxEdgeRatios;
      break;
    case SVTK_QUALITY_SKEW:
      QuadQuality = QuadSkew;
      break;
    case SVTK_QUALITY_TAPER:
      QuadQuality = QuadTaper;
      break;
    case SVTK_QUALITY_WARPAGE:
      QuadQuality = QuadWarpage;
      break;
    case SVTK_QUALITY_AREA:
      QuadQuality = QuadArea;
      break;
    case SVTK_QUALITY_STRETCH:
      QuadQuality = QuadStretch;
      break;
    // case SVTK_QUALITY_MIN_ANGLE:
    case SVTK_QUALITY_MAX_ANGLE:
      QuadQuality = QuadMaxAngle;
      break;
    case SVTK_QUALITY_ODDY:
      QuadQuality = QuadOddy;
      break;
    case SVTK_QUALITY_CONDITION:
      QuadQuality = QuadCondition;
      break;
    case SVTK_QUALITY_JACOBIAN:
      QuadQuality = QuadJacobian;
      break;
    case SVTK_QUALITY_SCALED_JACOBIAN:
      QuadQuality = QuadScaledJacobian;
      break;
    case SVTK_QUALITY_SHEAR:
      QuadQuality = QuadShear;
      break;
    case SVTK_QUALITY_SHAPE:
      QuadQuality = QuadShape;
      break;
    case SVTK_QUALITY_RELATIVE_SIZE_SQUARED:
      QuadQuality = QuadRelativeSizeSquared;
      break;
    case SVTK_QUALITY_SHAPE_AND_SIZE:
      QuadQuality = QuadShapeAndSize;
      break;
    case SVTK_QUALITY_SHEAR_AND_SIZE:
      QuadQuality = QuadShearAndSize;
      break;
    case SVTK_QUALITY_DISTORTION:
      QuadQuality = QuadDistortion;
      break;
    default:
      svtkWarningMacro("Bad QuadQualityMeasure (" << this->GetQuadQualityMeasure()
                                                 << "), using EdgeRatio instead");
      QuadQuality = QuadEdgeRatio;
      break;
  }

  switch (this->GetTetQualityMeasure())
  {
    case SVTK_QUALITY_EDGE_RATIO:
      TetQuality = TetEdgeRatio;
      break;
    case SVTK_QUALITY_ASPECT_RATIO:
      TetQuality = TetAspectRatio;
      break;
    case SVTK_QUALITY_RADIUS_RATIO:
      TetQuality = TetRadiusRatio;
      break;
    case SVTK_QUALITY_ASPECT_FROBENIUS:
      TetQuality = TetAspectFrobenius;
      break;
    case SVTK_QUALITY_MIN_ANGLE:
      TetQuality = TetMinAngle;
      break;
    case SVTK_QUALITY_COLLAPSE_RATIO:
      TetQuality = TetCollapseRatio;
      break;
    case SVTK_QUALITY_ASPECT_BETA:
      TetQuality = TetAspectBeta;
      break;
    case SVTK_QUALITY_ASPECT_GAMMA:
      TetQuality = TetAspectGamma;
      break;
    case SVTK_QUALITY_VOLUME:
      TetQuality = TetVolume;
      break;
    case SVTK_QUALITY_CONDITION:
      TetQuality = TetCondition;
      break;
    case SVTK_QUALITY_JACOBIAN:
      TetQuality = TetJacobian;
      break;
    case SVTK_QUALITY_SCALED_JACOBIAN:
      TetQuality = TetScaledJacobian;
      break;
    case SVTK_QUALITY_SHAPE:
      TetQuality = TetShape;
      break;
    case SVTK_QUALITY_RELATIVE_SIZE_SQUARED:
      TetQuality = TetRelativeSizeSquared;
      break;
    case SVTK_QUALITY_SHAPE_AND_SIZE:
      TetQuality = TetShapeandSize;
      break;
    case SVTK_QUALITY_DISTORTION:
      TetQuality = TetDistortion;
      break;
    default:
      svtkWarningMacro("Bad TetQualityMeasure (" << this->GetTetQualityMeasure()
                                                << "), using RadiusRatio instead");
      TetQuality = TetRadiusRatio;
      break;
  }

  switch (this->GetHexQualityMeasure())
  {
    case SVTK_QUALITY_EDGE_RATIO:
      HexQuality = HexEdgeRatio;
      break;
    case SVTK_QUALITY_MED_ASPECT_FROBENIUS:
      HexQuality = HexMedAspectFrobenius;
      break;
    case SVTK_QUALITY_MAX_ASPECT_FROBENIUS:
      HexQuality = HexMaxAspectFrobenius;
      break;
    case SVTK_QUALITY_MAX_EDGE_RATIO:
      HexQuality = HexMaxEdgeRatio;
      break;
    case SVTK_QUALITY_SKEW:
      HexQuality = HexSkew;
      break;
    case SVTK_QUALITY_TAPER:
      HexQuality = HexTaper;
      break;
    case SVTK_QUALITY_VOLUME:
      HexQuality = HexVolume;
      break;
    case SVTK_QUALITY_STRETCH:
      HexQuality = HexStretch;
      break;
    case SVTK_QUALITY_DIAGONAL:
      HexQuality = HexDiagonal;
      break;
    case SVTK_QUALITY_DIMENSION:
      HexQuality = HexDimension;
      break;
    case SVTK_QUALITY_ODDY:
      HexQuality = HexOddy;
      break;
    case SVTK_QUALITY_CONDITION:
      HexQuality = HexCondition;
      break;
    case SVTK_QUALITY_JACOBIAN:
      HexQuality = HexJacobian;
      break;
    case SVTK_QUALITY_SCALED_JACOBIAN:
      HexQuality = HexScaledJacobian;
      break;
    case SVTK_QUALITY_SHEAR:
      HexQuality = HexShear;
      break;
    case SVTK_QUALITY_SHAPE:
      HexQuality = HexShape;
      break;
    case SVTK_QUALITY_RELATIVE_SIZE_SQUARED:
      HexQuality = HexRelativeSizeSquared;
      break;
    case SVTK_QUALITY_SHAPE_AND_SIZE:
      HexQuality = HexShapeAndSize;
      break;
    case SVTK_QUALITY_SHEAR_AND_SIZE:
      HexQuality = HexShearAndSize;
      break;
    case SVTK_QUALITY_DISTORTION:
      HexQuality = HexDistortion;
      break;
    default:
      svtkWarningMacro("Bad HexQualityMeasure (" << this->GetTetQualityMeasure()
                                                << "), using MaxAspectFrobenius instead");
      HexQuality = HexMaxAspectFrobenius;
      break;
  }

  out->ShallowCopy(in);

  if (this->SaveCellQuality)
  {
    quality = svtkDoubleArray::New();
    if (this->CompatibilityMode)
    {
      if (this->Volume)
      {
        quality->SetNumberOfComponents(2);
      }
      else
      {
        quality->SetNumberOfComponents(1);
      }
    }
    else
    {
      quality->SetNumberOfComponents(1);
    }
    quality->SetNumberOfTuples(N);
    quality->SetName("Quality");
    out->GetCellData()->AddArray(quality);
    out->GetCellData()->SetActiveAttribute("Quality", svtkDataSetAttributes::SCALARS);
    quality->Delete();

    if (!this->CompatibilityMode)
    {
      if (this->Volume)
      {
        volume = svtkDoubleArray::New();
        volume->SetNumberOfComponents(1);
        volume->SetNumberOfTuples(N);
        volume->SetName("Volume");
        out->GetCellData()->AddArray(volume);
        volume->Delete();
      }
    }
  }

  // These measures require the average area/volume for all cells of the same type in the mesh.
  // Either use the hinted value (computed by a previous svtkMeshQuality filter) or compute it.
  if (this->GetTriangleQualityMeasure() == SVTK_QUALITY_RELATIVE_SIZE_SQUARED ||
    this->GetTriangleQualityMeasure() == SVTK_QUALITY_SHAPE_AND_SIZE ||
    this->GetQuadQualityMeasure() == SVTK_QUALITY_RELATIVE_SIZE_SQUARED ||
    this->GetQuadQualityMeasure() == SVTK_QUALITY_SHAPE_AND_SIZE ||
    this->GetQuadQualityMeasure() == SVTK_QUALITY_SHEAR_AND_SIZE ||
    this->GetTetQualityMeasure() == SVTK_QUALITY_RELATIVE_SIZE_SQUARED ||
    this->GetTetQualityMeasure() == SVTK_QUALITY_SHAPE_AND_SIZE ||
    this->GetHexQualityMeasure() == SVTK_QUALITY_RELATIVE_SIZE_SQUARED ||
    this->GetHexQualityMeasure() == SVTK_QUALITY_SHAPE_AND_SIZE ||
    this->GetHexQualityMeasure() == SVTK_QUALITY_SHEAR_AND_SIZE)
  {
    svtkDataArray* triAreaHint = in->GetFieldData()->GetArray("TriArea");
    svtkDataArray* quadAreaHint = in->GetFieldData()->GetArray("QuadArea");
    svtkDataArray* tetVolHint = in->GetFieldData()->GetArray("TetVolume");
    svtkDataArray* hexVolHint = in->GetFieldData()->GetArray("HexVolume");

    double triAreaTuple[5];
    double quadAreaTuple[5];
    double tetVolTuple[5];
    double hexVolTuple[5];

    if (triAreaHint && triAreaHint->GetNumberOfTuples() > 0 &&
      triAreaHint->GetNumberOfComponents() == 5 && quadAreaHint &&
      quadAreaHint->GetNumberOfTuples() > 0 && quadAreaHint->GetNumberOfComponents() == 5 &&
      tetVolHint && tetVolHint->GetNumberOfTuples() > 0 &&
      tetVolHint->GetNumberOfComponents() == 5 && hexVolHint &&
      hexVolHint->GetNumberOfTuples() > 0 && hexVolHint->GetNumberOfComponents() == 5)
    {
      triAreaHint->GetTuple(0, triAreaTuple);
      quadAreaHint->GetTuple(0, quadAreaTuple);
      tetVolHint->GetTuple(0, tetVolTuple);
      hexVolHint->GetTuple(0, hexVolTuple);
      v_set_tri_size(triAreaTuple[1] / triAreaTuple[4]);
      v_set_quad_size(quadAreaTuple[1] / quadAreaTuple[4]);
      v_set_tet_size(tetVolTuple[1] / tetVolTuple[4]);
      v_set_hex_size(hexVolTuple[1] / hexVolTuple[4]);
    }
    else
    {
      for (int i = 0; i < 5; ++i)
      {
        triAreaTuple[i] = 0;
        quadAreaTuple[i] = 0;
        tetVolTuple[i] = 0;
        hexVolTuple[i] = 0;
      }
      for (svtkIdType c = 0; c < N; ++c)
      {
        double a, v; // area and volume
        cell = out->GetCell(c);
        switch (cell->GetCellType())
        {
          case SVTK_TRIANGLE:
            a = TriangleArea(cell);
            if (a > triAreaTuple[2])
            {
              if (triAreaTuple[0] == triAreaTuple[2])
              { // min == max => min has not been set
                triAreaTuple[0] = a;
              }
              triAreaTuple[2] = a;
            }
            else if (a < triAreaTuple[0])
            {
              triAreaTuple[0] = a;
            }
            triAreaTuple[1] += a;
            triAreaTuple[3] += a * a;
            ntri++;
            break;
          case SVTK_QUAD:
            a = QuadArea(cell);
            if (a > quadAreaTuple[2])
            {
              if (quadAreaTuple[0] == quadAreaTuple[2])
              { // min == max => min has not been set
                quadAreaTuple[0] = a;
              }
              quadAreaTuple[2] = a;
            }
            else if (a < quadAreaTuple[0])
            {
              quadAreaTuple[0] = a;
            }
            quadAreaTuple[1] += a;
            quadAreaTuple[3] += a * a;
            nqua++;
            break;
          case SVTK_TETRA:
            v = TetVolume(cell);
            if (v > tetVolTuple[2])
            {
              if (tetVolTuple[0] == tetVolTuple[2])
              { // min == max => min has not been set
                tetVolTuple[0] = v;
              }
              tetVolTuple[2] = v;
            }
            else if (v < tetVolTuple[0])
            {
              tetVolTuple[0] = v;
            }
            tetVolTuple[1] += v;
            tetVolTuple[3] += v * v;
            ntet++;
            break;
          case SVTK_HEXAHEDRON:
            v = HexVolume(cell);
            if (v > hexVolTuple[2])
            {
              if (hexVolTuple[0] == hexVolTuple[2])
              { // min == max => min has not been set
                hexVolTuple[0] = v;
              }
              hexVolTuple[2] = v;
            }
            else if (v < hexVolTuple[0])
            {
              hexVolTuple[0] = v;
            }
            hexVolTuple[1] += v;
            hexVolTuple[3] += v * v;
            nhex++;
            break;
        }
      }
      triAreaTuple[4] = ntri;
      quadAreaTuple[4] = nqua;
      tetVolTuple[4] = ntet;
      hexVolTuple[4] = nhex;
      v_set_tri_size(triAreaTuple[1] / triAreaTuple[4]);
      v_set_quad_size(quadAreaTuple[1] / quadAreaTuple[4]);
      v_set_tet_size(tetVolTuple[1] / tetVolTuple[4]);
      v_set_hex_size(hexVolTuple[1] / hexVolTuple[4]);
      progressNumer = 20;
      progressDenom = 40.;
      ntri = 0;
      nqua = 0;
      ntet = 0;
      nhex = 0;

      // Save info as field data for downstream filters
      triAreaHint = svtkDoubleArray::New();
      triAreaHint->SetName("TriArea");
      triAreaHint->SetNumberOfComponents(5);
      triAreaHint->InsertNextTuple(triAreaTuple);
      out->GetFieldData()->AddArray(triAreaHint);
      triAreaHint->Delete();

      quadAreaHint = svtkDoubleArray::New();
      quadAreaHint->SetName("QuadArea");
      quadAreaHint->SetNumberOfComponents(5);
      quadAreaHint->InsertNextTuple(quadAreaTuple);
      out->GetFieldData()->AddArray(quadAreaHint);
      quadAreaHint->Delete();

      tetVolHint = svtkDoubleArray::New();
      tetVolHint->SetName("TetVolume");
      tetVolHint->SetNumberOfComponents(5);
      tetVolHint->InsertNextTuple(tetVolTuple);
      out->GetFieldData()->AddArray(tetVolHint);
      tetVolHint->Delete();

      hexVolHint = svtkDoubleArray::New();
      hexVolHint->SetName("HexVolume");
      hexVolHint->SetNumberOfComponents(5);
      hexVolHint->InsertNextTuple(hexVolTuple);
      out->GetFieldData()->AddArray(hexVolHint);
      hexVolHint->Delete();
    }
  }

  int p;
  svtkIdType c = 0;
  svtkIdType sz = N / 20 + 1;
  svtkIdType inner;
  this->UpdateProgress(progressNumer / progressDenom + 0.01);
  for (p = 0; p < 20; ++p)
  {
    for (inner = 0; (inner < sz && c < N); ++c, ++inner)
    {
      cell = out->GetCell(c);
      V = 0.;
      switch (cell->GetCellType())
      {
        case SVTK_TRIANGLE:
          if (this->CellNormals)
            this->CellNormals->GetTuple(c, svtkMeshQuality::CurrentTriNormal);
          q = TriangleQuality(cell);
          if (q > qtriM)
          {
            if (qtrim > qtriM)
            {
              qtrim = q;
            }
            qtriM = q;
          }
          else if (q < qtrim)
          {
            qtrim = q;
          }
          Eqtri += q;
          Eqtri2 += q * q;
          ++ntri;
          break;
        case SVTK_QUAD:
          q = QuadQuality(cell);
          if (q > qquaM)
          {
            if (qquam > qquaM)
            {
              qquam = q;
            }
            qquaM = q;
          }
          else if (q < qquam)
          {
            qquam = q;
          }
          Eqqua += q;
          Eqqua2 += q * q;
          ++nqua;
          break;
        case SVTK_TETRA:
          q = TetQuality(cell);
          if (q > qtetM)
          {
            if (qtetm > qtetM)
            {
              qtetm = q;
            }
            qtetM = q;
          }
          else if (q < qtetm)
          {
            qtetm = q;
          }
          Eqtet += q;
          Eqtet2 += q * q;
          ++ntet;
          if (this->Volume)
          {
            V = TetVolume(cell);
            if (!this->CompatibilityMode)
            {
              volume->SetTuple1(0, V);
            }
          }
          break;
        case SVTK_HEXAHEDRON:
          q = HexQuality(cell);
          if (q > qhexM)
          {
            if (qhexm > qhexM)
            {
              qhexm = q;
            }
            qhexM = q;
          }
          else if (q < qhexm)
          {
            qhexm = q;
          }
          Eqhex += q;
          Eqhex2 += q * q;
          ++nhex;
          break;
        default:
          q = 0.;
      }

      if (this->SaveCellQuality)
      {
        if (this->CompatibilityMode && this->Volume)
        {
          quality->SetTuple2(c, V, q);
        }
        else
        {
          quality->SetTuple1(c, q);
        }
      }
    }
    this->UpdateProgress(double(p + 1 + progressNumer) / progressDenom);
  }

  if (ntri)
  {
    Eqtri /= static_cast<double>(ntri);
    double multFactor = 1. / static_cast<double>(ntri > 1 ? ntri - 1 : ntri);
    Eqtri2 = multFactor * (Eqtri2 - static_cast<double>(ntri) * Eqtri * Eqtri);
  }
  else
  {
    qtrim = Eqtri = qtriM = Eqtri2 = 0.;
  }

  if (nqua)
  {
    Eqqua /= static_cast<double>(nqua);
    double multFactor = 1. / static_cast<double>(nqua > 1 ? nqua - 1 : nqua);
    Eqqua2 = multFactor * (Eqqua2 - static_cast<double>(nqua) * Eqqua * Eqqua);
  }
  else
  {
    qquam = Eqqua = qquaM = Eqqua2 = 0.;
  }

  if (ntet)
  {
    Eqtet /= static_cast<double>(ntet);
    double multFactor = 1. / static_cast<double>(ntet > 1 ? ntet - 1 : ntet);
    Eqtet2 = multFactor * (Eqtet2 - static_cast<double>(ntet) * Eqtet * Eqtet);
  }
  else
  {
    qtetm = Eqtet = qtetM = Eqtet2 = 0.;
  }

  if (nhex)
  {
    Eqhex /= static_cast<double>(nhex);
    double multFactor = 1. / static_cast<double>(nhex > 1 ? nhex - 1 : nhex);
    Eqhex2 = multFactor * (Eqhex2 - static_cast<double>(nhex) * Eqhex * Eqhex);
  }
  else
  {
    qhexm = Eqhex = qhexM = Eqhex2 = 0.;
  }

  double tuple[5];
  quality = svtkDoubleArray::New();
  quality->SetName("Mesh Triangle Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qtrim;
  tuple[1] = Eqtri;
  tuple[2] = qtriM;
  tuple[3] = Eqtri2;
  tuple[4] = ntri;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

  quality = svtkDoubleArray::New();
  quality->SetName("Mesh Quadrilateral Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qquam;
  tuple[1] = Eqqua;
  tuple[2] = qquaM;
  tuple[3] = Eqqua2;
  tuple[4] = nqua;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

  quality = svtkDoubleArray::New();
  quality->SetName("Mesh Tetrahedron Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qtetm;
  tuple[1] = Eqtet;
  tuple[2] = qtetM;
  tuple[3] = Eqtet2;
  tuple[4] = ntet;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

  quality = svtkDoubleArray::New();
  quality->SetName("Mesh Hexahedron Quality");
  quality->SetNumberOfComponents(5);
  tuple[0] = qhexm;
  tuple[1] = Eqhex;
  tuple[2] = qhexM;
  tuple[3] = Eqhex2;
  tuple[4] = nhex;
  quality->InsertNextTuple(tuple);
  out->GetFieldData()->AddArray(quality);
  quality->Delete();

  return 1;
}

int svtkMeshQuality::GetCurrentTriangleNormal(double point[3], double normal[3])
{
  // ignore the location where the normal should be evaluated.
  (void)point;

  // copy the cell normal
  for (int i = 0; i < 3; ++i)
    normal[i] = svtkMeshQuality::CurrentTriNormal[i];
  return 1;
}

// Triangle quality metrics

double svtkMeshQuality::TriangleArea(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_area(3, pc);
}

double svtkMeshQuality::TriangleEdgeRatio(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_edge_ratio(3, pc);
}

double svtkMeshQuality::TriangleAspectRatio(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_aspect_ratio(3, pc);
}

double svtkMeshQuality::TriangleRadiusRatio(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_radius_ratio(3, pc);
}

double svtkMeshQuality::TriangleAspectFrobenius(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_aspect_frobenius(3, pc);
}

double svtkMeshQuality::TriangleMinAngle(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_minimum_angle(3, pc);
}

double svtkMeshQuality::TriangleMaxAngle(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_maximum_angle(3, pc);
}

double svtkMeshQuality::TriangleCondition(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_condition(3, pc);
}

double svtkMeshQuality::TriangleScaledJacobian(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_scaled_jacobian(3, pc);
}

double svtkMeshQuality::TriangleRelativeSizeSquared(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_relative_size_squared(3, pc);
}

double svtkMeshQuality::TriangleShape(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_shape(3, pc);
}

double svtkMeshQuality::TriangleShapeAndSize(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_shape_and_size(3, pc);
}

double svtkMeshQuality::TriangleDistortion(svtkCell* cell)
{
  double pc[3][3];

  svtkPoints* p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_distortion(3, pc);
}

// Quadrangle quality metrics

double svtkMeshQuality::QuadEdgeRatio(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_edge_ratio(4, pc);
}

double svtkMeshQuality::QuadAspectRatio(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_aspect_ratio(4, pc);
}

double svtkMeshQuality::QuadRadiusRatio(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_radius_ratio(4, pc);
}

double svtkMeshQuality::QuadMedAspectFrobenius(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_med_aspect_frobenius(4, pc);
}

double svtkMeshQuality::QuadMaxAspectFrobenius(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_max_aspect_frobenius(4, pc);
}

double svtkMeshQuality::QuadMinAngle(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_minimum_angle(4, pc);
}

double svtkMeshQuality::QuadMaxEdgeRatios(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_max_edge_ratio(4, pc);
}

double svtkMeshQuality::QuadSkew(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_skew(4, pc);
}

double svtkMeshQuality::QuadTaper(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_taper(4, pc);
}

double svtkMeshQuality::QuadWarpage(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_warpage(4, pc);
}

double svtkMeshQuality::QuadArea(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_area(4, pc);
}

double svtkMeshQuality::QuadStretch(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_stretch(4, pc);
}

#if 0
// FIXME
double svtkMeshQuality::QuadMinAngle( svtkCell* cell )
{
  double pc[4][3];

  svtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_minimum_angle( 4, pc );
}
#endif // 0

double svtkMeshQuality::QuadMaxAngle(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_maximum_angle(4, pc);
}

double svtkMeshQuality::QuadOddy(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_oddy(4, pc);
}

double svtkMeshQuality::QuadCondition(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_condition(4, pc);
}

double svtkMeshQuality::QuadJacobian(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_jacobian(4, pc);
}

double svtkMeshQuality::QuadScaledJacobian(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_scaled_jacobian(4, pc);
}

double svtkMeshQuality::QuadShear(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_shear(4, pc);
}

double svtkMeshQuality::QuadShape(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_shape(4, pc);
}

double svtkMeshQuality::QuadRelativeSizeSquared(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_relative_size_squared(4, pc);
}

double svtkMeshQuality::QuadShapeAndSize(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_shape_and_size(4, pc);
}

double svtkMeshQuality::QuadShearAndSize(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_shear_and_size(4, pc);
}

double svtkMeshQuality::QuadDistortion(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_quad_distortion(4, pc);
}

// Volume of a tetrahedron, for compatibility with the original svtkMeshQuality

double TetVolume(svtkCell* cell)
{
  double x0[3];
  double x1[3];
  double x2[3];
  double x3[3];
  cell->Points->GetPoint(0, x0);
  cell->Points->GetPoint(1, x1);
  cell->Points->GetPoint(2, x2);
  cell->Points->GetPoint(3, x3);
  return svtkTetra::ComputeVolume(x0, x1, x2, x3);
}

// Tetrahedral quality metrics

double svtkMeshQuality::TetEdgeRatio(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_edge_ratio(4, pc);
}

double svtkMeshQuality::TetAspectRatio(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_aspect_ratio(4, pc);
}

double svtkMeshQuality::TetRadiusRatio(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_radius_ratio(4, pc);
}

double svtkMeshQuality::TetAspectBeta(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_aspect_beta(4, pc);
}

double svtkMeshQuality::TetAspectFrobenius(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_aspect_frobenius(4, pc);
}

double svtkMeshQuality::TetMinAngle(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_minimum_angle(4, pc);
}

double svtkMeshQuality::TetCollapseRatio(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_collapse_ratio(4, pc);
}

double svtkMeshQuality::TetAspectGamma(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_aspect_gamma(4, pc);
}

double svtkMeshQuality::TetVolume(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_volume(4, pc);
}

double svtkMeshQuality::TetCondition(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_condition(4, pc);
}

double svtkMeshQuality::TetJacobian(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_jacobian(4, pc);
}

double svtkMeshQuality::TetScaledJacobian(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_scaled_jacobian(4, pc);
}

double svtkMeshQuality::TetShape(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_shape(4, pc);
}

double svtkMeshQuality::TetRelativeSizeSquared(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_relative_size_squared(4, pc);
}

double svtkMeshQuality::TetShapeandSize(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_shape_and_size(4, pc);
}

double svtkMeshQuality::TetDistortion(svtkCell* cell)
{
  double pc[4][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 4; ++i)
    p->GetPoint(i, pc[i]);

  return v_tet_distortion(4, pc);
}

// Hexahedral quality metrics

double svtkMeshQuality::HexEdgeRatio(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_edge_ratio(8, pc);
}

double svtkMeshQuality::HexMedAspectFrobenius(svtkCell* cell)
{
  double pc[8][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_med_aspect_frobenius(8, pc);
}

double svtkMeshQuality::HexMaxAspectFrobenius(svtkCell* cell)
{
  double pc[8][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_max_aspect_frobenius(8, pc);
}

double svtkMeshQuality::HexMaxEdgeRatio(svtkCell* cell)
{
  double pc[8][3];

  svtkPoints* p = cell->GetPoints();
  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_max_edge_ratio(8, pc);
}

double svtkMeshQuality::HexSkew(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_skew(8, pc);
}

double svtkMeshQuality::HexTaper(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_taper(8, pc);
}

double svtkMeshQuality::HexVolume(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_volume(8, pc);
}

double svtkMeshQuality::HexStretch(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_stretch(8, pc);
}

double svtkMeshQuality::HexDiagonal(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_diagonal(8, pc);
}

double svtkMeshQuality::HexDimension(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_dimension(8, pc);
}

double svtkMeshQuality::HexOddy(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_oddy(8, pc);
}

double svtkMeshQuality::HexCondition(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_condition(8, pc);
}

double svtkMeshQuality::HexJacobian(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_jacobian(8, pc);
}

double svtkMeshQuality::HexScaledJacobian(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_scaled_jacobian(8, pc);
}

double svtkMeshQuality::HexShear(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_shear(8, pc);
}

double svtkMeshQuality::HexShape(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_shape(8, pc);
}

double svtkMeshQuality::HexRelativeSizeSquared(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_relative_size_squared(8, pc);
}

double svtkMeshQuality::HexShapeAndSize(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_shape_and_size(8, pc);
}

double svtkMeshQuality::HexShearAndSize(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_shear_and_size(8, pc);
}

double svtkMeshQuality::HexDistortion(svtkCell* cell)
{
  double pc[8][3];
  svtkPoints* p = cell->GetPoints();

  for (int i = 0; i < 8; ++i)
    p->GetPoint(i, pc[i]);

  return v_hex_distortion(8, pc);
}
