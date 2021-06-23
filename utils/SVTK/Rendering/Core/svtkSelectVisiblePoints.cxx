/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelectVisiblePoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSelectVisiblePoints.h"

#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkSelectVisiblePoints);

// Instantiate object with no renderer; window selection turned off;
// tolerance set to 0.01; and select invisible off.
svtkSelectVisiblePoints::svtkSelectVisiblePoints()
{
  this->Renderer = nullptr;
  this->SelectionWindow = 0;
  this->Selection[0] = this->Selection[2] = 0;
  this->Selection[1] = this->Selection[3] = 1600;
  this->InternalSelection[0] = this->InternalSelection[2] = 0;
  this->InternalSelection[1] = this->InternalSelection[3] = 1600;
  this->CompositePerspectiveTransform = svtkMatrix4x4::New();
  this->DirectionOfProjection[0] = this->DirectionOfProjection[1] = this->DirectionOfProjection[2] =
    0.0;
  this->Tolerance = 0.01;
  this->ToleranceWorld = 0.0;
  this->SelectInvisible = 0;
}

svtkSelectVisiblePoints::~svtkSelectVisiblePoints()
{
  this->SetRenderer(nullptr);
  this->CompositePerspectiveTransform->Delete();
}

int svtkSelectVisiblePoints::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType ptId, cellId;
  int visible;
  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  svtkIdType numPts = input->GetNumberOfPoints();
  double x[4];

  // Nothing to extract if there are no points in the data set.
  if (numPts < 1)
  {
    return 1;
  }

  if (this->Renderer == nullptr)
  {
    svtkErrorMacro(<< "Renderer must be set");
    return 0;
  }

  if (!this->Renderer->GetRenderWindow())
  {
    svtkErrorMacro("No render window -- can't get window size to query z buffer.");
    return 0;
  }

  // This will trigger if you do something like ResetCamera before the Renderer or
  // RenderWindow have allocated their appropriate system resources (like creating
  // an OpenGL context)." Resource allocation must occur before we can use the Z
  // buffer.
  if (this->Renderer->GetRenderWindow()->GetNeverRendered())
  {
    svtkDebugMacro("RenderWindow not initialized -- aborting update.");
    return 1;
  }

  svtkCamera* cam = this->Renderer->GetActiveCamera();
  if (!cam)
  {
    return 1;
  }

  svtkPoints* outPts = svtkPoints::New();
  outPts->Allocate(numPts / 2 + 1);
  outPD->CopyAllocate(inPD);

  svtkCellArray* outputVertices = svtkCellArray::New();
  output->SetVerts(outputVertices);
  outputVertices->Delete();

  const int SimpleQueryLimit = 25;
  bool getZbuff = numPts > SimpleQueryLimit ? true : false;

  float* zPtr = this->Initialize(getZbuff);

  int abort = 0;
  svtkIdType progressInterval = numPts / 20 + 1;
  x[3] = 1.0;
  for (cellId = (-1), ptId = 0; ptId < numPts && !abort; ptId++)
  {
    // perform conversion
    input->GetPoint(ptId, x);

    if (!(ptId % progressInterval))
    {
      this->UpdateProgress(static_cast<double>(ptId) / numPts);
      abort = this->GetAbortExecute();
    }

    visible = IsPointOccluded(x, zPtr);

    if ((visible && !this->SelectInvisible) || (!visible && this->SelectInvisible))
    {
      cellId = outPts->InsertNextPoint(x);
      output->InsertNextCell(SVTK_VERTEX, 1, &cellId);
      outPD->CopyData(inPD, ptId, cellId);
    }
  } // for all points

  output->SetPoints(outPts);
  outPts->Delete();
  output->Squeeze();

  delete[] zPtr;

  svtkDebugMacro(<< "Selected " << cellId + 1 << " out of " << numPts << " original points");

  return 1;
}

svtkMTimeType svtkSelectVisiblePoints::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Renderer != nullptr)
  {
    time = this->Renderer->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

int svtkSelectVisiblePoints::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkSelectVisiblePoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Renderer: " << this->Renderer << "\n";
  os << indent << "Selection Window: " << (this->SelectionWindow ? "On\n" : "Off\n");

  os << indent << "Selection: \n";
  os << indent << "  Xmin,Xmax: (" << this->Selection[0] << ", " << this->Selection[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Selection[2] << ", " << this->Selection[3] << ")\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Select Invisible: " << (this->SelectInvisible ? "On\n" : "Off\n");
}

float* svtkSelectVisiblePoints::Initialize(bool getZbuff)
{
  svtkCamera* cam = this->Renderer->GetActiveCamera();
  if (!cam)
  {
    return nullptr;
  }
  cam->GetDirectionOfProjection(this->DirectionOfProjection);

  const int* size = this->Renderer->GetRenderWindow()->GetSize();

  // specify a selection window to avoid querying
  if (this->SelectionWindow)
  {
    for (int i = 0; i < 4; i++)
    {
      this->InternalSelection[i] = this->Selection[i];
    }
  }
  else
  {
    this->InternalSelection[0] = this->InternalSelection[2] = 0;
    this->InternalSelection[1] = size[0] - 1;
    this->InternalSelection[3] = size[1] - 1;
  }

  // Grab the composite perspective transform.  This matrix is used to convert
  // each point to view coordinates.  svtkRenderer provides a WorldToView()
  // method but it computes the composite perspective transform each time
  // WorldToView() is called.  This is expensive, so we get the matrix once
  // and handle the transformation ourselves.
  this->CompositePerspectiveTransform->DeepCopy(
    this->Renderer->GetActiveCamera()->GetCompositeProjectionTransformMatrix(
      this->Renderer->GetTiledAspectRatio(), 0, 1));

  // If we have more than a few query points, we grab the z-buffer for the
  // selection region all at once and probe the resulting array.  When we
  // have just a few points, we perform individual z-buffer queries.
  if (getZbuff)
  {
    return this->Renderer->GetRenderWindow()->GetZbufferData(this->InternalSelection[0],
      this->InternalSelection[2], this->InternalSelection[1], this->InternalSelection[3]);
  }
  return nullptr;
}

bool svtkSelectVisiblePoints::IsPointOccluded(const double x[3], const float* zPtr)
{
  double view[4];
  double dx[3], z;
  double xx[4] = { x[0], x[1], x[2], 1.0 };
  if (this->ToleranceWorld > 0.0)
  {
    xx[0] -= this->DirectionOfProjection[0] * this->ToleranceWorld;
    xx[1] -= this->DirectionOfProjection[1] * this->ToleranceWorld;
    xx[2] -= this->DirectionOfProjection[2] * this->ToleranceWorld;
  }

  this->CompositePerspectiveTransform->MultiplyPoint(xx, view);
  if (view[3] == 0.0)
  {
    return false;
  }
  this->Renderer->SetViewPoint(view[0] / view[3], view[1] / view[3], view[2] / view[3]);
  this->Renderer->ViewToDisplay();
  this->Renderer->GetDisplayPoint(dx);

  // check whether visible and in selection window
  if (dx[0] >= this->InternalSelection[0] && dx[0] <= this->InternalSelection[1] &&
    dx[1] >= this->InternalSelection[2] && dx[1] <= this->InternalSelection[3])
  {
    if (zPtr != nullptr)
    {
      // Access the value from the captured zbuffer.  Note, we only
      // captured a portion of the zbuffer, so we need to offset dx by
      // the selection window.
      z = zPtr[static_cast<int>(dx[0]) - this->InternalSelection[0] +
        (static_cast<int>(dx[1]) - this->InternalSelection[2]) *
          (this->InternalSelection[1] - this->InternalSelection[0] + 1)];
    }
    else
    {
      z = this->Renderer->GetZ(static_cast<int>(dx[0]), static_cast<int>(dx[1]));
    }

    if (dx[2] < (z + this->Tolerance))
    {
      return true;
    }
  }

  return false;
}
