/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataSilhouette.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// Contribution by Thierry Carrard <br>
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>

#include "svtkPolyDataSilhouette.h"

#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkProp3D.h"
#include "svtkTransform.h"
#include "svtkUnsignedIntArray.h"

#include <map>

svtkStandardNewMacro(svtkPolyDataSilhouette);

svtkCxxSetObjectMacro(svtkPolyDataSilhouette, Camera, svtkCamera);

struct svtkOrderedEdge
{
  inline svtkOrderedEdge(svtkIdType a, svtkIdType b)
  {
    if (a <= b)
    {
      p1 = a;
      p2 = b;
    }
    else
    {
      p1 = b;
      p2 = a;
    }
  }
  inline bool operator<(const svtkOrderedEdge& oe) const
  {
    return (p1 < oe.p1) || ((p1 == oe.p1) && (p2 < oe.p2));
  }
  svtkIdType p1, p2;
};

struct svtkTwoNormals
{
  double leftNormal[3];  // normal of the left polygon
  double rightNormal[3]; // normal of the right polygon
  inline svtkTwoNormals()
  {
    leftNormal[0] = 0.0;
    leftNormal[1] = 0.0;
    leftNormal[2] = 0.0;
    rightNormal[0] = 0.0;
    rightNormal[1] = 0.0;
    rightNormal[2] = 0.0;
  }
};

class svtkPolyDataEdges
{
public:
  svtkTimeStamp mtime;
  double vec[3];
  std::map<svtkOrderedEdge, svtkTwoNormals> edges;
  bool* edgeFlag;
  svtkCellArray* lines;
  inline svtkPolyDataEdges()
    : edgeFlag(nullptr)
    , lines(nullptr)
  {
    vec[0] = vec[1] = vec[2] = 0.0;
  }
};

svtkPolyDataSilhouette::svtkPolyDataSilhouette()
{
  this->Camera = nullptr;
  this->Prop3D = nullptr;
  this->Direction = SVTK_DIRECTION_CAMERA_ORIGIN;
  this->Vector[0] = this->Vector[1] = this->Vector[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Transform = svtkTransform::New();
  this->EnableFeatureAngle = 1;
  this->FeatureAngle = 60;
  this->BorderEdges = 0;
  this->PieceInvariant = 1;
  this->PreComp = new svtkPolyDataEdges();
}

svtkPolyDataSilhouette::~svtkPolyDataSilhouette()
{
  this->Transform->Delete();

  if (this->Camera)
  {
    this->Camera->Delete();
  }

  delete[] this->PreComp->edgeFlag;
  if (this->PreComp->lines)
  {
    this->PreComp->lines->Delete();
  }
  delete this->PreComp;

  // Note: svtkProp3D is not deleted to avoid reference count cycle
}

// Don't reference count to avoid nasty cycle
void svtkPolyDataSilhouette::SetProp3D(svtkProp3D* prop3d)
{
  if (this->Prop3D != prop3d)
  {
    this->Prop3D = prop3d;
    this->Modified();
  }
}

svtkProp3D* svtkPolyDataSilhouette::GetProp3D()
{
  return this->Prop3D;
}

int svtkPolyDataSilhouette::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input == nullptr || output == nullptr)
  {
    svtkErrorMacro(<< "Need correct connections");
    return 0;
  }

  svtkDebugMacro(<< "RequestData\n");

  const double featureAngleCos = cos(svtkMath::RadiansFromDegrees(this->FeatureAngle));

  bool vectorMode = true;
  double vector[3];
  double origin[3];

  // Compute the sort vector
  switch (this->Direction)
  {
    case SVTK_DIRECTION_SPECIFIED_VECTOR:
      vector[0] = this->Vector[0];
      vector[1] = this->Vector[1];
      vector[2] = this->Vector[2];
      break;

    case SVTK_DIRECTION_SPECIFIED_ORIGIN:
      origin[0] = this->Origin[0];
      origin[1] = this->Origin[1];
      origin[2] = this->Origin[2];
      vectorMode = false;
      break;

    case SVTK_DIRECTION_CAMERA_ORIGIN:
      vectorMode = false;
      SVTK_FALLTHROUGH;

    case SVTK_DIRECTION_CAMERA_VECTOR:
      if (this->Camera == nullptr)
      {
        svtkErrorMacro(<< "Need a camera when direction is set to SVTK_DIRECTION_CAMERA_*");
        return 0;
      }
      this->ComputeProjectionVector(vector, origin);
      break;
  }

  svtkPoints* inPoints = input->GetPoints();

  if (input->GetMTime() > this->PreComp->mtime.GetMTime())
  {
    svtkDebugMacro(<< "Compute edge-face connectivity and face normals\n");

    this->PreComp->mtime.Modified();
    this->PreComp->edges.clear();

    svtkCellArray* polyCells = input->GetPolys();
    auto polyIter = svtkSmartPointer<svtkCellArrayIterator>::Take(polyCells->NewIterator());

    for (polyIter->GoToFirstCell(); !polyIter->IsDoneWithTraversal(); polyIter->GoToNextCell())
    {
      svtkIdType np;
      const svtkIdType* polys;
      polyIter->GetCurrentCell(np, polys);

      double normal[3];
      svtkPolygon::ComputeNormal(
        inPoints, static_cast<int>(np), const_cast<svtkIdType*>(polys), normal);

      for (int j = 0; j < np; j++)
      {
        svtkIdType p1 = j, p2 = (j + 1) % np;
        svtkOrderedEdge oe(polys[p1], polys[p2]);
        svtkTwoNormals& tn = this->PreComp->edges[oe];
        if (polys[p1] < polys[p2])
        {
#ifdef DEBUG
          if (svtkMath::Dot(tn.leftNormal, tn.leftNormal) > 0)
          {
            svtkDebugMacro(<< "Warning: svtkPolyDataSilhouette: non-manifold mesh: edge-L ("
                          << polys[p1] << "," << polys[p2] << ") counted more than once\n");
          }
#endif
          tn.leftNormal[0] = normal[0];
          tn.leftNormal[1] = normal[1];
          tn.leftNormal[2] = normal[2];
        }
        else
        {
#ifdef DEBUG
          if (svtkMath::Dot(tn.rightNormal, tn.rightNormal) > 0)
          {
            svtkDebugMacro(<< "Warning: svtkPolyDataSilhouette: non-manifold mesh: edge-R ("
                          << polys[p1] << "," << polys[p2] << ") counted more than once\n");
          }
#endif
          tn.rightNormal[0] = normal[0];
          tn.rightNormal[1] = normal[1];
          tn.rightNormal[2] = normal[2];
        }
      }
    }

    delete[] this->PreComp->edgeFlag;
    this->PreComp->edgeFlag = new bool[this->PreComp->edges.size()];
  }

  bool vecChanged = false;
  for (int d = 0; d < 3; d++)
  {
    vecChanged = vecChanged || this->PreComp->vec[d] != vector[d];
  }

  if ((this->PreComp->mtime.GetMTime() > output->GetMTime()) ||
    (this->Camera && this->Camera->GetMTime() > output->GetMTime()) ||
    (this->Prop3D && this->Prop3D->GetMTime() > output->GetMTime()) || vecChanged)
  {
    svtkDebugMacro(<< "Extract edges\n");

    svtkIdType i = 0, silhouetteEdges = 0;

    for (std::map<svtkOrderedEdge, svtkTwoNormals>::iterator it = this->PreComp->edges.begin();
         it != this->PreComp->edges.end(); ++it)
    {
      double d1, d2;

      // does this edge have two co-faces ?
      bool winged =
        svtkMath::Norm(it->second.leftNormal) > 0.5 && svtkMath::Norm(it->second.rightNormal) > 0.5;

      // cosine of feature angle, to be compared with scalar product of two co-faces normals
      double edgeAngleCos = svtkMath::Dot(it->second.leftNormal, it->second.rightNormal);

      if (vectorMode) // uniform direction
      {
        d1 = svtkMath::Dot(vector, it->second.leftNormal);
        d2 = svtkMath::Dot(vector, it->second.rightNormal);
      }
      else // origin to edge's center direction
      {
        double p1[3];
        double p2[3];
        double vec[3];
        inPoints->GetPoint(it->first.p1, p1);
        inPoints->GetPoint(it->first.p2, p2);
        vec[0] = origin[0] - ((p1[0] + p2[0]) * 0.5);
        vec[1] = origin[1] - ((p1[1] + p2[1]) * 0.5);
        vec[2] = origin[2] - ((p1[2] + p2[2]) * 0.5);
        d1 = svtkMath::Dot(vec, it->second.leftNormal);
        d2 = svtkMath::Dot(vec, it->second.rightNormal);
      }

      // shall we output this edge ?
      bool outputEdge = (winged && (d1 * d2) < 0) ||
        (this->EnableFeatureAngle && edgeAngleCos < featureAngleCos) ||
        (this->BorderEdges && !winged);

      if (outputEdge) // add this edge
      {
        this->PreComp->edgeFlag[i] = true;
        ++silhouetteEdges;
      }
      else // skip this edge
      {
        this->PreComp->edgeFlag[i] = false;
      }
      ++i;
    }

    // build output data set (lines)
    svtkIdTypeArray* la = svtkIdTypeArray::New();
    la->SetNumberOfValues(3 * silhouetteEdges);
    svtkIdType* laPtr = la->WritePointer(0, 3 * silhouetteEdges);

    i = 0;
    silhouetteEdges = 0;
    for (std::map<svtkOrderedEdge, svtkTwoNormals>::iterator it = this->PreComp->edges.begin();
         it != this->PreComp->edges.end(); ++it)
    {
      if (this->PreComp->edgeFlag[i])
      {
        laPtr[silhouetteEdges * 3 + 0] = 2;
        laPtr[silhouetteEdges * 3 + 1] = it->first.p1;
        laPtr[silhouetteEdges * 3 + 2] = it->first.p2;
        ++silhouetteEdges;
      }
      ++i;
    }

    if (this->PreComp->lines == nullptr)
    {
      this->PreComp->lines = svtkCellArray::New();
    }
    this->PreComp->lines->AllocateEstimate(silhouetteEdges, 2);
    this->PreComp->lines->ImportLegacyFormat(la);
    la->Delete();
  }

  output->Initialize();
  output->SetPoints(inPoints);
  output->SetLines(this->PreComp->lines);

  return 1;
}

void svtkPolyDataSilhouette::ComputeProjectionVector(double vector[3], double origin[3])
{
  double* focalPoint = this->Camera->GetFocalPoint();
  double* position = this->Camera->GetPosition();

  // If a camera is present, use it
  if (!this->Prop3D)
  {
    for (int i = 0; i < 3; i++)
    {
      vector[i] = focalPoint[i] - position[i];
      origin[i] = position[i];
    }
  }

  else // Otherwise, use Prop3D
  {
    double focalPt[4], pos[4];
    int i;

    this->Transform->SetMatrix(this->Prop3D->GetMatrix());
    this->Transform->Push();
    this->Transform->Inverse();

    for (i = 0; i < 4; i++)
    {
      focalPt[i] = focalPoint[i];
      pos[i] = position[i];
    }

    this->Transform->TransformPoint(focalPt, focalPt);
    this->Transform->TransformPoint(pos, pos);

    for (i = 0; i < 3; i++)
    {
      vector[i] = focalPt[i] - pos[i];
      origin[i] = pos[i];
    }
    this->Transform->Pop();
  }
}

svtkMTimeType svtkPolyDataSilhouette::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->Direction != SVTK_DIRECTION_SPECIFIED_VECTOR)
  {
    svtkMTimeType time;
    if (this->Camera != nullptr)
    {
      time = this->Camera->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }

    if (this->Prop3D != nullptr)
    {
      time = this->Prop3D->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }
  }

  return mTime;
}

void svtkPolyDataSilhouette::PrintSelf(ostream& os, svtkIndent indent)
{

  this->Superclass::PrintSelf(os, indent);

  if (this->Camera)
  {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Camera: (none)\n";
  }

  if (this->Prop3D)
  {
    os << indent << "Prop3D:\n";
    this->Prop3D->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Prop3D: (none)\n";
  }

  os << indent << "Direction: ";
#define DIRECTION_CASE(name)                                                                       \
  case SVTK_DIRECTION_##name:                                                                       \
    os << "SVTK_DIRECTION_" << #name << "\n";                                                       \
    break
  switch (this->Direction)
  {
    DIRECTION_CASE(SPECIFIED_ORIGIN);
    DIRECTION_CASE(SPECIFIED_VECTOR);
    DIRECTION_CASE(CAMERA_ORIGIN);
    DIRECTION_CASE(CAMERA_VECTOR);
  }
#undef DIRECTION_CASE

  if (this->Direction == SVTK_DIRECTION_SPECIFIED_VECTOR)
  {
    os << "Specified Vector: (" << this->Vector[0] << ", " << this->Vector[1] << ", "
       << this->Vector[2] << ")\n";
  }
  if (this->Direction == SVTK_DIRECTION_SPECIFIED_ORIGIN)
  {
    os << "Specified Origin: (" << this->Origin[0] << ", " << this->Origin[1] << ", "
       << this->Origin[2] << ")\n";
  }

  os << indent << "PieceInvariant: " << this->PieceInvariant << "\n";
  os << indent << "FeatureAngle: " << this->FeatureAngle << "\n";
  os << indent << "EnableFeatureAngle: " << this->EnableFeatureAngle << "\n";
  os << indent << "BorderEdges: " << this->BorderEdges << "\n";
}
