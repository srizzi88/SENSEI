/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostDividedEdgeBundling.cxx

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
#include "svtkBoostDividedEdgeBundling.h"

#include "svtkBoostGraphAdapter.h"
#include "svtkDataSetAttributes.h"
#include "svtkDirectedGraph.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkVectorOperators.h"

#include <algorithm>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/property_map/property_map.hpp>

svtkStandardNewMacro(svtkBoostDividedEdgeBundling);

svtkBoostDividedEdgeBundling::svtkBoostDividedEdgeBundling() {}

class svtkBundlingMetadata
{
public:
  svtkBundlingMetadata(svtkBoostDividedEdgeBundling* alg, svtkDirectedGraph* g)
    : Outer(alg)
    , Graph(g)
  {
    this->Nodes = reinterpret_cast<svtkVector3f*>(
      svtkArrayDownCast<svtkFloatArray>(g->GetPoints()->GetData())->GetPointer(0));
    this->Edges.resize(g->GetNumberOfEdges());
    for (svtkIdType e = 0; e < g->GetNumberOfEdges(); ++e)
    {
      this->Edges[e] = std::make_pair(g->GetSourceVertex(e), g->GetTargetVertex(e));
    }
    this->VelocityDamping = 0.1f;
    this->EdgeCoulombConstant = 0.5f;
    // this->EdgeCoulombConstant = 50.0f;
    this->EdgeCoulombDecay = 35.0f;
    this->EdgeSpringConstant = 0.1f;
    // this->EdgeSpringConstant = 0.0005f;
    this->EdgeLaneWidth = 25.0f;
    this->UseNewForce = true;
  }

  void ProjectOnto(svtkIdType e1, svtkIdType e2, svtkVector3f& s, svtkVector3f& t);
  void NormalizeNodePositions();
  void DenormalizeNodePositions();
  void CalculateNodeDistances();
  float AngleCompatibility(svtkIdType e1, svtkIdType e2);
  float ScaleCompatibility(svtkIdType e1, svtkIdType e2);
  float PositionCompatibility(svtkIdType e1, svtkIdType e2);
  float VisibilityCompatibility(svtkIdType e1, svtkIdType e2);
  float ConnectivityCompatibility(svtkIdType e1, svtkIdType e2);
  void CalculateEdgeLengths();
  void CalculateEdgeCompatibilities();
  void InitializeEdgeMesh();
  void DoubleEdgeMeshResolution();
  void SimulateEdgeStep();
  void LayoutEdgePoints();
  void SmoothEdges();

  float SimulationStep;
  int CycleIterations;
  int MeshCount;
  float VelocityDamping;
  float EdgeCoulombConstant;
  float EdgeCoulombDecay;
  float EdgeSpringConstant;
  float EdgeLaneWidth;
  bool UseNewForce;
  svtkBoostDividedEdgeBundling* Outer;
  svtkDirectedGraph* Graph;
  svtkVector3f* Nodes;
  std::vector<std::pair<svtkIdType, svtkIdType> > Edges;
  std::vector<std::vector<float> > NodeDistances;
  std::vector<float> EdgeLengths;
  std::vector<std::vector<float> > EdgeCompatibilities;
  std::vector<std::vector<float> > EdgeDots;
  std::vector<std::vector<svtkVector3f> > EdgeMesh;
  std::vector<std::vector<svtkVector3f> > EdgeMeshVelocities;
  std::vector<std::vector<svtkVector3f> > EdgeMeshAccelerations;
  // std::vector<std::vector<float> > EdgeMeshGroupCounts;
  svtkVector2f XRange;
  svtkVector2f YRange;
  svtkVector2f ZRange;
  float Scale;
};

void svtkBundlingMetadata::NormalizeNodePositions()
{
  this->XRange = svtkVector2f(SVTK_FLOAT_MAX, SVTK_FLOAT_MIN);
  this->YRange = svtkVector2f(SVTK_FLOAT_MAX, SVTK_FLOAT_MIN);
  this->ZRange = svtkVector2f(SVTK_FLOAT_MAX, SVTK_FLOAT_MIN);
  for (svtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
  {
    svtkVector3f p = this->Nodes[i];
    this->XRange[0] = std::min(this->XRange[0], p[0]);
    this->XRange[1] = std::max(this->XRange[1], p[0]);
    this->YRange[0] = std::min(this->YRange[0], p[1]);
    this->YRange[1] = std::max(this->YRange[1], p[1]);
    this->ZRange[0] = std::min(this->ZRange[0], p[2]);
    this->ZRange[1] = std::max(this->ZRange[1], p[2]);
  }
  float dx = this->XRange[1] - this->XRange[0];
  float dy = this->YRange[1] - this->YRange[0];
  float dz = this->ZRange[1] - this->ZRange[0];
  this->Scale = std::max(dx, std::max(dy, dz));
  for (svtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
  {
    svtkVector3f p = this->Nodes[i];
    this->Nodes[i] = svtkVector3f((p[0] - this->XRange[0]) / this->Scale * 1000.0f,
      (p[1] - this->YRange[0]) / this->Scale * 1000.0f,
      (p[2] - this->ZRange[0]) / this->Scale * 1000.0f);
  }
}

void svtkBundlingMetadata::DenormalizeNodePositions()
{
  for (svtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
  {
    svtkVector3f p = this->Nodes[i];
    this->Nodes[i] = svtkVector3f(p[0] / 1000.0f * this->Scale + this->XRange[0],
      p[1] / 1000.0f * this->Scale + this->YRange[0],
      p[2] / 1000.0f * this->Scale + this->ZRange[0]);
  }
  for (svtkIdType i = 0; i < (int)this->EdgeMesh.size(); ++i)
  {
    for (svtkIdType j = 0; j < (int)this->EdgeMesh[i].size(); ++j)
    {
      svtkVector3f p = this->EdgeMesh[i][j];
      this->EdgeMesh[i][j] = svtkVector3f(p[0] / 1000.0f * this->Scale + this->XRange[0],
        p[1] / 1000.0f * this->Scale + this->YRange[0],
        p[2] / 1000.0f * this->Scale + this->ZRange[0]);
    }
  }
}

void svtkBundlingMetadata::CalculateNodeDistances()
{
  svtkIdType numVerts = this->Graph->GetNumberOfVertices();
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->NodeDistances.resize(numVerts, std::vector<float>(numVerts, SVTK_FLOAT_MAX));
  std::vector<float> weights(numVerts, 1.0f);
  svtkNew<svtkFloatArray> weightMap;
  weightMap->SetNumberOfTuples(numEdges);
  for (svtkIdType e = 0; e < numEdges; ++e)
  {
    weightMap->SetValue(e, 1.0f);
  }
  boost::svtkGraphEdgePropertyMapHelper<svtkFloatArray*> weightProp(weightMap);
  boost::johnson_all_pairs_shortest_paths(
    this->Graph, this->NodeDistances, boost::weight_map(weightProp));
}

float svtkBundlingMetadata::AngleCompatibility(svtkIdType e1, svtkIdType e2)
{
  if (this->EdgeLengths[e1] == 0.0f || this->EdgeLengths[e2] == 0.0f)
  {
    return 0.0f;
  }
  svtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  svtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  svtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  svtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  svtkVector3f p1 = s1 - t1;
  svtkVector3f p2 = s2 - t2;
  float compatibility = p1.Dot(p2) / (this->EdgeLengths[e1] * this->EdgeLengths[e2]);
  return fabs(compatibility);
}

float svtkBundlingMetadata::ScaleCompatibility(svtkIdType e1, svtkIdType e2)
{
  float len1 = this->EdgeLengths[e1];
  float len2 = this->EdgeLengths[e2];
  float average = (len1 + len2) / 2.0f;
  if (average == 0.0f)
  {
    return 0.0f;
  }
  return 2.0f / (average / std::min(len1, len2) + std::max(len1, len2) / average);
}

float svtkBundlingMetadata::PositionCompatibility(svtkIdType e1, svtkIdType e2)
{
  float len1 = this->EdgeLengths[e1];
  float len2 = this->EdgeLengths[e2];
  float average = (len1 + len2) / 2.0f;
  if (average == 0.0f)
  {
    return 0.0f;
  }
  svtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  svtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  svtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  svtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  svtkVector3f mid1 = 0.5 * (s1 + t1);
  svtkVector3f mid2 = 0.5 * (s2 + t2);
  return average / (average + (mid1 - mid2).Norm());
}

void svtkBundlingMetadata::ProjectOnto(svtkIdType e1, svtkIdType e2, svtkVector3f& s, svtkVector3f& t)
{
  svtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  svtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  svtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  svtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  svtkVector3f norm = t2 - s2;
  norm.Normalize();
  svtkVector3f toHead = s1 - s2;
  svtkVector3f toTail = t1 - s2;
  svtkVector3f headOnOther = norm * norm.Dot(toHead);
  svtkVector3f tailOnOther = norm * norm.Dot(toTail);
  s = s2 + headOnOther;
  t = s2 + tailOnOther;
}

float svtkBundlingMetadata::VisibilityCompatibility(svtkIdType e1, svtkIdType e2)
{
  svtkVector3f is;
  svtkVector3f it;
  svtkVector3f js;
  svtkVector3f jt;
  this->ProjectOnto(e1, e2, is, it);
  this->ProjectOnto(e2, e1, js, jt);
  float ilen = (is - it).Norm();
  float jlen = (js - jt).Norm();
  if (ilen == 0.0f || jlen == 0.0f)
  {
    return 0.0f;
  }
  svtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  svtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  svtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  svtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  svtkVector3f mid1 = 0.5 * (s1 + t1);
  svtkVector3f mid2 = 0.5 * (s2 + t2);
  svtkVector3f imid = 0.5 * (is + it);
  svtkVector3f jmid = 0.5 * (js + jt);
  float midQI = (mid2 - imid).Norm();
  float vpq = std::max(0.0f, 1.0f - (2.0f * midQI) / ilen);
  float midPJ = (mid1 - jmid).Norm();
  float vqp = std::max(0.0f, 1.0f - (2.0f * midPJ) / jlen);

  return std::min(vpq, vqp);
}

float svtkBundlingMetadata::ConnectivityCompatibility(svtkIdType e1, svtkIdType e2)
{
  svtkIdType s1 = this->Edges[e1].first;
  svtkIdType t1 = this->Edges[e1].second;
  svtkIdType s2 = this->Edges[e2].first;
  svtkIdType t2 = this->Edges[e2].second;
  if (s1 == s2 || s1 == t2 || t1 == s2 || t1 == t2)
  {
    return 1.0f;
  }
  float minPath = std::min(this->NodeDistances[s1][s2],
    std::min(this->NodeDistances[s1][t2],
      std::min(this->NodeDistances[t1][s2], this->NodeDistances[t1][t2])));
  return 1.0f / (minPath + 1.0f);
}

void svtkBundlingMetadata::CalculateEdgeLengths()
{
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->EdgeLengths.resize(numEdges);
  for (svtkIdType e = 0; e < numEdges; ++e)
  {
    svtkVector3f s = this->Nodes[this->Edges[e].first];
    svtkVector3f t = this->Nodes[this->Edges[e].second];
    this->EdgeLengths[e] = (s - t).Norm();
  }
}

void svtkBundlingMetadata::CalculateEdgeCompatibilities()
{
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->EdgeCompatibilities.resize(numEdges, std::vector<float>(numEdges, 1.0f));
  this->EdgeDots.resize(numEdges, std::vector<float>(numEdges, 1.0f));
  for (svtkIdType e1 = 0; e1 < numEdges; ++e1)
  {
    svtkVector3f s1 = this->Nodes[this->Edges[e1].first];
    svtkVector3f t1 = this->Nodes[this->Edges[e1].second];
    svtkVector3f r1 = s1 - t1;
    r1.Normalize();
    for (svtkIdType e2 = e1 + 1; e2 < numEdges; ++e2)
    {
      float compatibility = 1.0f;
      compatibility *= this->AngleCompatibility(e1, e2);
      compatibility *= this->ScaleCompatibility(e1, e2);
      compatibility *= this->PositionCompatibility(e1, e2);
      compatibility *= this->VisibilityCompatibility(e1, e2);
      compatibility *= this->ConnectivityCompatibility(e1, e2);
      this->EdgeCompatibilities[e1][e2] = compatibility;
      this->EdgeCompatibilities[e2][e1] = compatibility;

      svtkVector3f s2 = this->Nodes[this->Edges[e2].first];
      svtkVector3f t2 = this->Nodes[this->Edges[e2].second];
      svtkVector3f r2 = s2 - t2;
      r2.Normalize();
      float dot = r1.Dot(r2);
      this->EdgeDots[e1][e2] = dot;
      this->EdgeDots[e2][e1] = dot;
    }
  }
}

void svtkBundlingMetadata::InitializeEdgeMesh()
{
  this->MeshCount = 2;
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->EdgeMesh.resize(numEdges, std::vector<svtkVector3f>(2));
  this->EdgeMeshVelocities.resize(numEdges, std::vector<svtkVector3f>(2));
  this->EdgeMeshAccelerations.resize(numEdges, std::vector<svtkVector3f>(2));
  // this->EdgeMeshGroupCounts.resize(numEdges, std::vector<float>(2, 1.0f));
  for (svtkIdType e = 0; e < numEdges; ++e)
  {
    this->EdgeMesh[e][0] = this->Nodes[this->Edges[e].first];
    this->EdgeMesh[e][1] = this->Nodes[this->Edges[e].second];
  }
}

void svtkBundlingMetadata::DoubleEdgeMeshResolution()
{
  int newMeshCount = (this->MeshCount - 1) * 2 + 1;
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  std::vector<std::vector<svtkVector3f> > newEdgeMesh(
    numEdges, std::vector<svtkVector3f>(newMeshCount));
  std::vector<std::vector<svtkVector3f> > newEdgeMeshVelocities(
    numEdges, std::vector<svtkVector3f>(newMeshCount, svtkVector3f(0.0f, 0.0f, 0.0f)));
  std::vector<std::vector<svtkVector3f> > newEdgeMeshAccelerations(
    numEdges, std::vector<svtkVector3f>(newMeshCount, svtkVector3f(0.0f, 0.0f, 0.0f)));
  // std::vector<std::vector<float> > newEdgeMeshGroupCounts(
  //    numEdges, std::vector<float>(newMeshCount, 1.0f));
  for (svtkIdType e = 0; e < numEdges; ++e)
  {
    for (int m = 0; m < newMeshCount; ++m)
    {
      float indexFloat = (this->MeshCount - 1.0f) * m / (newMeshCount - 1.0f);
      int index = static_cast<int>(indexFloat);
      float alpha = indexFloat - index;
      svtkVector3f before = this->EdgeMesh[e][index];
      if (alpha > 0)
      {
        svtkVector3f after = this->EdgeMesh[e][index + 1];
        newEdgeMesh[e][m] = before + alpha * (after - before);
      }
      else
      {
        newEdgeMesh[e][m] = before;
      }
    }
  }
  this->MeshCount = newMeshCount;
  this->EdgeMesh = newEdgeMesh;
  this->EdgeMeshVelocities = newEdgeMeshVelocities;
  this->EdgeMeshAccelerations = newEdgeMeshAccelerations;
  // this->EdgeMeshGroupCounts = newEdgeMeshGroupCounts;
}

void svtkBundlingMetadata::SimulateEdgeStep()
{
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();

  for (svtkIdType e1 = 0; e1 < numEdges; ++e1)
  {
    float weight1 = 1.0f;
    for (int m1 = 0; m1 < this->MeshCount; ++m1)
    {
      // Immovable
      if (m1 <= 0 || m1 >= this->MeshCount - 1)
      {
        continue;
      }

      // Move the point according to dynamics
      svtkVector3f position = this->EdgeMesh[e1][m1];
      svtkVector3f velocity = this->EdgeMeshVelocities[e1][m1];
      svtkVector3f acceleration = this->EdgeMeshAccelerations[e1][m1];
      velocity = velocity + acceleration * this->SimulationStep * 0.5f;
      velocity = velocity * this->VelocityDamping;
      position = position + velocity * this->SimulationStep;
      this->EdgeMesh[e1][m1] = position;

      acceleration = svtkVector3f(0.0f, 0.0f, 0.0f);

      // Spring force
      svtkVector3f prevPosition = this->EdgeMesh[e1][m1 - 1];
      svtkVector3f prevDirection = prevPosition - position;
      float prevDist = prevDirection.Norm();
      float prevForce =
        this->EdgeSpringConstant / 1000.0f * (this->MeshCount - 1) * prevDist * weight1;
      prevDirection.Normalize();
      acceleration = acceleration + prevForce * prevDirection;

      svtkVector3f nextPosition = this->EdgeMesh[e1][m1 + 1];
      svtkVector3f nextDirection = nextPosition - position;
      float nextDist = nextDirection.Norm();
      float nextForce =
        this->EdgeSpringConstant / 1000.0f * (this->MeshCount - 1) * nextDist * weight1;
      nextDirection.Normalize();
      acceleration = acceleration + nextForce * nextDirection;

      // Coulomb force
      float normalizedEdgeCoulombConstant =
        this->EdgeCoulombConstant / sqrt(static_cast<float>(numEdges));

      for (svtkIdType e2 = 0; e2 < numEdges; ++e2)
      {
        if (e1 == e2)
        {
          continue;
        }

        float compatibility = this->EdgeCompatibilities[e1][e2];
        if (compatibility <= 0.05)
        {
          continue;
        }

        float dot = this->EdgeDots[e1][e2];
        float weight2 = 1.0f;

        int m2;
        if (dot >= 0.0f)
        {
          m2 = m1;
        }
        else
        {
          m2 = this->MeshCount - 1 - m1;
        }

        svtkVector3f position2;
        // If we're going the same direction is edge1, then the potential minimum is at the point.
        if (dot >= 0.0f)
        {
          position2 = this->EdgeMesh[e2][m2];
        }
        // If we're going the opposite direction, the potential minimum is edgeLaneWidth to the
        // "right."
        else
        {
          svtkVector3f tangent = this->EdgeMesh[e2][m2 + 1] - this->EdgeMesh[e2][m2 - 1];
          tangent.Normalize();
          // This assumes 2D
          svtkVector3f normal(-tangent[1], tangent[0], 0.0f);
          position2 = this->EdgeMesh[e2][m2] + normal * this->EdgeLaneWidth;
        }

        svtkVector3f direction = position2 - position;
        float distance = direction.Norm();

        // Inverse force.
        float force;
        if (!this->UseNewForce)
        {
          force =
            normalizedEdgeCoulombConstant * 30.0f / (this->MeshCount - 1) / (distance + 0.01f);
        }
        // New force.
        else
        {
          force = 4.0f * 10000.0f / (this->MeshCount - 1) * this->EdgeCoulombDecay *
            normalizedEdgeCoulombConstant * distance /
            (3.1415926f *
              pow(this->EdgeCoulombDecay * this->EdgeCoulombDecay + distance * distance, 2));
        }
        force *= weight2;
        force *= compatibility;

        if (distance > 0.0f)
        {
          direction.Normalize();
          acceleration = acceleration + force * direction;
        }
      }

      velocity = velocity + acceleration * this->SimulationStep * 0.5f;
      this->EdgeMeshVelocities[e1][m1] = velocity;
      this->EdgeMeshAccelerations[e1][m1] = acceleration;
    }
  }
}

void svtkBundlingMetadata::SmoothEdges()
{
  // From Mathematica Total[GaussianMatrix[{3, 3}]]
  int kernelSize = 3;
  // Has to sum to 1.0 to be correct.
  float gaussianKernel[] = { 0.10468, 0.139936, 0.166874, 0.177019, 0.166874, 0.139936, 0.10468 };
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  std::vector<std::vector<svtkVector3f> > smoothedEdgeMesh(
    numEdges, std::vector<svtkVector3f>(this->MeshCount));
  for (svtkIdType e = 0; e < numEdges; ++e)
  {
    for (int m = 1; m < this->MeshCount - 1; ++m)
    {
      svtkVector3f smoothed(0.0f, 0.0f, 0.0f);
      for (int kernelIndex = 0; kernelIndex < kernelSize * 2 + 1; kernelIndex++)
      {
        int m2 = m + kernelIndex - kernelSize;
        m2 = std::max(0, std::min(this->MeshCount - 1, m2));

        svtkVector3f pt = this->EdgeMesh[e][m2];
        smoothed = smoothed + gaussianKernel[kernelIndex] * pt;
      }
      smoothedEdgeMesh[e][m] = smoothed;
    }
  }
  this->EdgeMesh = smoothedEdgeMesh;
}

void svtkBundlingMetadata::LayoutEdgePoints()
{
  this->InitializeEdgeMesh();
  this->SimulationStep = 40.0f;
  this->CycleIterations = 30;
  for (int i = 0; i < 5; ++i)
  {
    svtkDebugWithObjectMacro(this->Outer, "svtkBoostDividedEdgeBundling cycle " << i);
    this->CycleIterations = this->CycleIterations * 2 / 3;
    this->SimulationStep = 0.85f * this->SimulationStep;
    this->DoubleEdgeMeshResolution();
    for (int j = 0; j < this->CycleIterations; ++j)
    {
      svtkDebugWithObjectMacro(this->Outer, "svtkBoostDividedEdgeBundling iteration " << j);
      this->SimulateEdgeStep();
    }
  }
  this->SmoothEdges();
}

int svtkBoostDividedEdgeBundling::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* graphInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDirectedGraph* g =
    svtkDirectedGraph::SafeDownCast(graphInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDirectedGraph* output =
    svtkDirectedGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkBundlingMetadata* meta = new svtkBundlingMetadata(this, g);

  meta->NormalizeNodePositions();
  meta->CalculateEdgeLengths();
  meta->CalculateNodeDistances();
  meta->CalculateEdgeCompatibilities();
  meta->LayoutEdgePoints();
  meta->DenormalizeNodePositions();

  output->ShallowCopy(g);

  for (svtkIdType e = 0; e < g->GetNumberOfEdges(); ++e)
  {
    output->ClearEdgePoints(e);
    for (int m = 1; m < meta->MeshCount - 1; ++m)
    {
      svtkVector3f edgePoint = meta->EdgeMesh[e][m];
      output->AddEdgePoint(e, edgePoint[0], edgePoint[1], edgePoint[2]);
    }
  }

  delete meta;

  return 1;
}

void svtkBoostDividedEdgeBundling::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
