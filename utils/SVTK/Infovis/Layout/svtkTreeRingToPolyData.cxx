/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeRingToPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkTreeRingToPolyData.h"

#include "svtkAppendPolyData.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSectorSource.h"
#include "svtkStripper.h"
#include "svtkTimerLog.h"
#include "svtkTree.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkTreeRingToPolyData);

svtkTreeRingToPolyData::svtkTreeRingToPolyData()
{
  this->SetSectorsArrayName("sectors");
  this->ShrinkPercentage = 0.0;
}

svtkTreeRingToPolyData::~svtkTreeRingToPolyData() = default;

int svtkTreeRingToPolyData::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
  return 1;
}

int svtkTreeRingToPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkTree* inputTree = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* outputPoly = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (inputTree->GetNumberOfVertices() == 0)
  {
    return 1;
  }

  // Now set the point coordinates, normals, and insert the cell
  svtkDataArray* coordArray = this->GetInputArrayToProcess(0, inputTree);
  if (!coordArray)
  {
    svtkErrorMacro("Sectors array not found.");
    return 0;
  }

  double pt1x[3] = { 0.0, 0.0, 0.0 };
  double pt2x[3] = { 0.0, 0.0, 0.0 };
  double ang = 0.0;
  double cos_ang = 0.0;
  double sin_ang = 0.0;
  svtkIdType pt1 = 0;
  svtkIdType pt2 = 0;
  svtkIdType rootId = inputTree->GetRoot();
  SVTK_CREATE(svtkCellArray, strips);
  SVTK_CREATE(svtkPoints, pts);
  double progress = 0.0;
  this->InvokeEvent(svtkCommand::ProgressEvent, &progress);

  for (int i = 0; i < inputTree->GetNumberOfVertices(); i++)
  {
    // Grab coords from the input
    double coords[4];
    if (i == rootId)
    {
      // don't draw the root node...
      coords[0] = 0.0;
      coords[1] = 0.0;
      coords[2] = 1.0;
      coords[3] = 1.0;
    }
    else
    {
      coordArray->GetTuple(i, coords);
    }

    double radial_length = coords[3] - coords[2];

    // Calculate the amount of change in the arcs based on the shrink
    // percentage of the arc_length
    double conversion = svtkMath::Pi() / 180.;
    double arc_length = (conversion * (coords[1] - coords[0]) * coords[3]);
    double radial_shrink = radial_length * this->ShrinkPercentage;
    double arc_length_shrink;
    if (radial_shrink > 0.25 * arc_length)
    {
      arc_length_shrink = 0.25 * arc_length;
    }
    else
    {
      arc_length_shrink = radial_shrink;
    }

    double arc_length_new = arc_length - arc_length_shrink;
    double angle_change = ((arc_length_new / coords[3]) / conversion);
    double delta_change_each = 0.5 * ((coords[1] - coords[0]) - angle_change);

    double inner_radius = coords[2] + (0.5 * (radial_length * this->ShrinkPercentage));
    double outer_radius = coords[3] - (0.5 * (radial_length * this->ShrinkPercentage));
    double start_angle;
    double end_angle;
    if (coords[1] - coords[0] == 360.)
    {
      start_angle = coords[0];
      end_angle = coords[1];
    }
    else
    {
      start_angle = coords[0] + delta_change_each;
      end_angle = coords[1] - delta_change_each;
    }

    int num_angles = static_cast<int>(end_angle - start_angle);
    if (num_angles < 1)
    {
      num_angles = 1;
    }
    int num_points = 2 * num_angles + 2;
    strips->InsertNextCell(num_points);
    for (int j = 0; j < num_angles; ++j)
    {
      ang = start_angle + j;
      cos_ang = cos(svtkMath::RadiansFromDegrees(ang));
      sin_ang = sin(svtkMath::RadiansFromDegrees(ang));
      pt1x[0] = cos_ang * inner_radius;
      pt1x[1] = sin_ang * inner_radius;
      pt2x[0] = cos_ang * outer_radius;
      pt2x[1] = sin_ang * outer_radius;
      pt1 = pts->InsertNextPoint(pt1x);
      pt2 = pts->InsertNextPoint(pt2x);
      strips->InsertCellPoint(pt1);
      strips->InsertCellPoint(pt2);
    }
    ang = end_angle;
    cos_ang = cos(svtkMath::RadiansFromDegrees(ang));
    sin_ang = sin(svtkMath::RadiansFromDegrees(ang));
    pt1x[0] = cos_ang * inner_radius;
    pt1x[1] = sin_ang * inner_radius;
    pt2x[0] = cos_ang * outer_radius;
    pt2x[1] = sin_ang * outer_radius;
    pt1 = pts->InsertNextPoint(pt1x);
    pt2 = pts->InsertNextPoint(pt2x);
    strips->InsertCellPoint(pt1);
    strips->InsertCellPoint(pt2);

    if (i % 1000 == 0)
    {
      progress = static_cast<double>(i) / inputTree->GetNumberOfVertices() * 0.8;
      this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
    }
  }

  outputPoly->SetPoints(pts);
  outputPoly->SetStrips(strips);

  // Pass the input vertex data to the output cell data :)
  svtkDataSetAttributes* const input_vertex_data = inputTree->GetVertexData();
  svtkDataSetAttributes* const output_cell_data = outputPoly->GetCellData();
  output_cell_data->PassData(input_vertex_data);

  return 1;
}

void svtkTreeRingToPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ShrinkPercentage: " << this->ShrinkPercentage << endl;
}
