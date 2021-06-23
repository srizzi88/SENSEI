/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPicker.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkActor.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkBox.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkImageSlice.h"
#include "svtkLODProp3D.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPoints.h"
#include "svtkProp3DCollection.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"
#include "svtkVertex.h"
#include "svtkVolume.h"

svtkStandardNewMacro(svtkPicker);

//----------------------------------------------------------------------
// Construct object with initial tolerance of 1/40th of window. There are no
// pick methods and picking is performed from the renderer's actors.
svtkPicker::svtkPicker()
{
  this->Tolerance = 0.025; // 1/40th of the renderer window

  this->MapperPosition[0] = 0.0;
  this->MapperPosition[1] = 0.0;
  this->MapperPosition[2] = 0.0;

  this->Mapper = nullptr;
  this->DataSet = nullptr;
  this->CompositeDataSet = nullptr;
  this->FlatBlockIndex = -1;
  this->GlobalTMin = SVTK_DOUBLE_MAX;
  this->Actors = svtkActorCollection::New();
  this->Prop3Ds = svtkProp3DCollection::New();
  this->PickedPositions = svtkPoints::New();
  this->Transform = svtkTransform::New();
}

//----------------------------------------------------------------------
svtkPicker::~svtkPicker()
{
  this->Actors->Delete();
  this->Prop3Ds->Delete();
  this->PickedPositions->Delete();
  this->Transform->Delete();
}

//----------------------------------------------------------------------
// Update state when prop3D is picked.
void svtkPicker::MarkPicked(svtkAssemblyPath* path, svtkProp3D* svtkNotUsed(prop3D),
  svtkAbstractMapper3D* m, double tMin, double mapperPos[3])
{
  svtkMapper* mapper;
  svtkAbstractVolumeMapper* volumeMapper;
  svtkImageMapper3D* imageMapper;

  if ((mapper = svtkMapper::SafeDownCast(m)) != nullptr)
  {
    this->MarkPickedData(path, tMin, mapperPos, mapper, mapper->GetInput());
  }
  else if ((volumeMapper = svtkAbstractVolumeMapper::SafeDownCast(m)) != nullptr)
  {
    this->MarkPickedData(path, tMin, mapperPos, volumeMapper, volumeMapper->GetDataSetInput());
  }
  else if ((imageMapper = svtkImageMapper3D::SafeDownCast(m)) != nullptr)
  {
    this->MarkPickedData(path, tMin, mapperPos, imageMapper, imageMapper->GetInput());
  }
  else
  {
    this->MarkPickedData(path, tMin, mapperPos, nullptr, nullptr);
  }
}

void svtkPicker::MarkPickedData(svtkAssemblyPath* path, double tMin, double mapperPos[3],
  svtkAbstractMapper3D* mapper, svtkDataSet* input, svtkIdType flatIndex)
{
  this->SetPath(path);
  this->GlobalTMin = tMin;

  for (int i = 0; i < 3; i++)
  {
    this->MapperPosition[i] = mapperPos[i];
  }
  // The point has to be transformed back into world coordinates.
  // Note: it is assumed that the transform is in the correct state.
  this->Transform->TransformPoint(mapperPos, this->PickPosition);

  this->Mapper = mapper;
  this->DataSet = input;
  this->CompositeDataSet = svtkCompositeDataSet::SafeDownCast(mapper->GetInputDataObject(0, 0));
  this->FlatBlockIndex = flatIndex;
}

int svtkPicker::Pick3DPoint(double pos[3], svtkRenderer* renderer)
{
  int i;
  svtkProp* prop;
  svtkAbstractMapper3D* mapper = nullptr;
  int winSize[2] = { 1, 1 };
  double x, y;
  double* viewport;
  int pickable;
  int LODId;
  double windowLowerLeft[4], windowUpperRight[4];
  double bounds[6], tol;
  svtkActor* actor;
  svtkLODProp3D* prop3D;
  svtkVolume* volume;
  svtkImageSlice* imageSlice = nullptr;
  svtkAssemblyPath* path;
  svtkProperty* tempProperty;
  svtkCollectionSimpleIterator pit;

  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  bounds[0] = bounds[1] = bounds[2] = bounds[3] = bounds[4] = bounds[5] = 0;

  // Compute the tolerance in world coordinates.  Do this by
  // determining the world coordinates of the diagonal points of the
  // window, computing the width of the window in world coordinates, and
  // multiplying by the tolerance.
  //
  viewport = renderer->GetViewport();
  if (renderer->GetRenderWindow())
  {
    const int* winSizePtr = renderer->GetRenderWindow()->GetSize();
    if (winSizePtr)
    {
      winSize[0] = winSizePtr[0];
      winSize[1] = winSizePtr[1];
    }
  }
  x = winSize[0] * viewport[0];
  y = winSize[1] * viewport[1];
  renderer->SetDisplayPoint(x, y, 0.0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowLowerLeft);

  x = winSize[0] * viewport[2];
  y = winSize[1] * viewport[3];
  renderer->SetDisplayPoint(x, y, 0.0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowUpperRight);

  for (tol = 0.0, i = 0; i < 3; i++)
  {
    tol += (windowUpperRight[i] - windowLowerLeft[i]) * (windowUpperRight[i] - windowLowerLeft[i]);
  }

  tol = sqrt(tol) * this->Tolerance;

  //  Loop over all props.  Transform ray (defined from position of
  //  camera to selection point) into coordinates of mapper (not
  //  transformed to actors coordinates!  Reduces overall computation!!!).
  //  Note that only svtkProp3D's can be picked by svtkPicker.
  //
  svtkPropCollection* props;
  svtkProp* propCandidate;
  if (this->PickFromList)
  {
    props = this->GetPickList();
  }
  else
  {
    props = renderer->GetViewProps();
  }

  for (props->InitTraversal(pit); (prop = props->GetNextProp(pit));)
  {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
    {
      pickable = 0;
      actor = nullptr;
      propCandidate = path->GetLastNode()->GetViewProp();
      if (propCandidate->GetPickable() && propCandidate->GetVisibility())
      {
        pickable = 1;
        if ((actor = svtkActor::SafeDownCast(propCandidate)) != nullptr)
        {
          mapper = actor->GetMapper();
          if (actor->GetProperty()->GetOpacity() <= 0.0)
          {
            pickable = 0;
          }
        }
        else if ((prop3D = svtkLODProp3D::SafeDownCast(propCandidate)) != nullptr)
        {
          LODId = prop3D->GetPickLODID();
          mapper = prop3D->GetLODMapper(LODId);

          // if the mapper is a svtkMapper (as opposed to a svtkVolumeMapper),
          // then check the transparency to see if the object is pickable
          if (svtkMapper::SafeDownCast(mapper) != nullptr)
          {
            prop3D->GetLODProperty(LODId, &tempProperty);
            if (tempProperty->GetOpacity() <= 0.0)
            {
              pickable = 0;
            }
          }
        }
        else if ((volume = svtkVolume::SafeDownCast(propCandidate)) != nullptr)
        {
          mapper = volume->GetMapper();
        }
        else if ((imageSlice = svtkImageSlice::SafeDownCast(propCandidate)))
        {
          mapper = imageSlice->GetMapper();
        }
        else
        {
          pickable = 0; // only svtkProp3D's (actors and volumes) can be picked
        }
      }

      //  If actor can be picked, get its composite matrix, invert it, and
      //  use the inverted matrix to transform the points into mapper
      //  coordinates.
      if (pickable)
      {
        const double* bnds = propCandidate->GetBounds();
        if (bnds)
        {
          bounds[0] = bnds[0] - tol;
          bounds[1] = bnds[1] + tol;
          bounds[2] = bnds[2] - tol;
          bounds[3] = bnds[3] + tol;
          bounds[4] = bnds[4] - tol;
          bounds[5] = bnds[5] + tol;
          if (pos[0] >= bounds[0] && pos[0] <= bounds[1] && pos[1] >= bounds[2] &&
            pos[1] <= bounds[3] && pos[2] >= bounds[4] && pos[2] <= bounds[5])
          {
            this->MarkPicked(path, static_cast<svtkProp3D*>(propCandidate), mapper, 0.0, pos);

            // The IsItemPresent method returns "index+1"
            int prevIndex = this->Prop3Ds->IsItemPresent(prop) - 1;

            if (prevIndex < 0)
            {
              this->Prop3Ds->AddItem(static_cast<svtkProp3D*>(prop));

              this->PickedPositions->InsertNextPoint(pos);

              // backwards compatibility: also add to this->Actors
              if (actor)
              {
                this->Actors->AddItem(actor);
              }
            }
          }
        }
      } // if visible and pickable and not transparent
    }   // for all parts
  }     // for all actors

  int picked = 0;

  if (this->Path)
  {
    // Invoke pick method if one defined - prop goes first
    this->Path->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(svtkCommand::PickEvent, nullptr);
    picked = 1;
  }

  // Invoke end pick method if defined
  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  return picked;
}

//----------------------------------------------------------------------
int svtkPicker::Pick3DPoint(double selectionPt[3], double focalPt[3], svtkRenderer* ren)
{
  // Initialize the picking process
  this->Initialize();
  this->Renderer = ren;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  int result = this->Pick3DInternal(ren, selectionPt, focalPt);

  // Invoke end pick method if defined
  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  return result;
}

//----------------------------------------------------------------------
// Perform pick operation with selection point provided. Normally the
// first two values for the selection point are x-y pixel coordinate, and
// the third value is =0. Return non-zero if something was successfully picked.
int svtkPicker::Pick(double selectionX, double selectionY, double selectionZ, svtkRenderer* renderer)
{
  int i;
  svtkCamera* camera;
  double cameraPos[4], cameraFP[4];
  double *displayCoords, *worldCoords;
  double ray[3], rayLength;
  double tF, tB;
  double cameraDOP[3];
  double p1World[4], p2World[4];
  double* clipRange;

  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = selectionZ;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  if (renderer == nullptr)
  {
    svtkErrorMacro(<< "Must specify renderer!");
    return 0;
  }

  // Get camera focal point and position. Convert to display (screen)
  // coordinates. We need a depth value for z-buffer.
  //
  camera = renderer->GetActiveCamera();
  camera->GetPosition(cameraPos);
  cameraPos[3] = 1.0;
  camera->GetFocalPoint(cameraFP);
  cameraFP[3] = 1.0;

  renderer->SetWorldPoint(cameraFP[0], cameraFP[1], cameraFP[2], cameraFP[3]);
  renderer->WorldToDisplay();
  displayCoords = renderer->GetDisplayPoint();
  selectionZ = displayCoords[2];

  // Convert the selection point into world coordinates.
  //
  renderer->SetDisplayPoint(selectionX, selectionY, selectionZ);
  renderer->DisplayToWorld();
  worldCoords = renderer->GetWorldPoint();
  if (worldCoords[3] == 0.0)
  {
    svtkErrorMacro(<< "Bad homogeneous coordinates");
    return 0;
  }
  for (i = 0; i < 3; i++)
  {
    this->PickPosition[i] = worldCoords[i] / worldCoords[3];
  }

  // For robustness, re-project the point on the focal point plane
  double planeNormal[3];
  svtkMath::Subtract(cameraFP, cameraPos, planeNormal);
  svtkMath::Normalize(planeNormal);
  svtkPlane::ProjectPoint(this->PickPosition, cameraFP, planeNormal, this->PickPosition);

  //  Compute the ray endpoints.  The ray is along the line running from
  //  the camera position to the selection point, starting where this line
  //  intersects the front clipping plane, and terminating where this
  //  line intersects the back clipping plane.
  for (i = 0; i < 3; i++)
  {
    ray[i] = this->PickPosition[i] - cameraPos[i];
  }
  for (i = 0; i < 3; i++)
  {
    cameraDOP[i] = cameraFP[i] - cameraPos[i];
  }

  svtkMath::Normalize(cameraDOP);

  if ((rayLength = svtkMath::Dot(cameraDOP, ray)) == 0.0)
  {
    svtkWarningMacro("Cannot process points");
    return 0;
  }

  clipRange = camera->GetClippingRange();

  if (camera->GetParallelProjection())
  {
    tF = clipRange[0] - rayLength;
    tB = clipRange[1] - rayLength;
    for (i = 0; i < 3; i++)
    {
      p1World[i] = this->PickPosition[i] + tF * cameraDOP[i];
      p2World[i] = this->PickPosition[i] + tB * cameraDOP[i];
    }
  }
  else
  {
    tF = clipRange[0] / rayLength;
    tB = clipRange[1] / rayLength;
    for (i = 0; i < 3; i++)
    {
      p1World[i] = cameraPos[i] + tF * ray[i];
      p2World[i] = cameraPos[i] + tB * ray[i];
    }
  }
  p1World[3] = p2World[3] = 1.0;

  int result = this->Pick3DInternal(renderer, p1World, p2World);

  // Invoke end pick method if defined
  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  return result;
}

int svtkPicker::Pick3DRay(double pos[3], double wori[4], svtkRenderer* renderer)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  double wp1[4], wp2[4];

  double dist = renderer->GetActiveCamera()->GetClippingRange()[1];

  svtkNew<svtkTransform> trans;
  trans->RotateWXYZ(wori[0], wori[1], wori[2], wori[3]);
  double* rayDirection = trans->TransformDoubleVector(0.0, 0.0, -1.0);

  for (int i = 0; i < 3; i++)
  {
    this->PickPosition[i] = pos[i];
    wp1[i] = pos[i];
    wp2[i] = pos[i] + dist * rayDirection[i];
  }

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  wp1[3] = 1.0;
  wp2[3] = 1.0;

  int result = this->Pick3DInternal(renderer, wp1, wp2);

  // Invoke end pick method if defined
  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);

  return result;
}

int svtkPicker::Pick3DInternal(svtkRenderer* renderer, double p1World[4], double p2World[4])
{
  int i;
  svtkProp* prop;
  svtkAbstractMapper3D* mapper = nullptr;
  double p1Mapper[4], p2Mapper[4];
  int winSize[2] = { 1, 1 };
  double x, y, t;
  double* viewport;
  double ray[3];
  int pickable;
  int LODId;
  double windowLowerLeft[4], windowUpperRight[4];
  double bounds[6], tol;
  double hitPosition[3];

  bounds[0] = bounds[1] = bounds[2] = bounds[3] = bounds[4] = bounds[5] = 0;

  if (renderer == nullptr)
  {
    svtkErrorMacro(<< "Must specify renderer!");
    return 0;
  }

  // Compute the tolerance in world coordinates.  Do this by
  // determining the world coordinates of the diagonal points of the
  // window, computing the width of the window in world coordinates, and
  // multiplying by the tolerance.
  //
  renderer->SetWorldPoint(0.5 * (p1World[0] + p2World[0]), 0.5 * (p1World[1] + p2World[1]),
    0.5 * (p1World[2] + p2World[2]), 1.0);
  renderer->WorldToDisplay();
  double* displayCoords = renderer->GetDisplayPoint();
  double tolZ = displayCoords[2];

  viewport = renderer->GetViewport();
  if (renderer->GetRenderWindow())
  {
    const int* winSizePtr = renderer->GetRenderWindow()->GetSize();
    if (winSizePtr)
    {
      winSize[0] = winSizePtr[0];
      winSize[1] = winSizePtr[1];
    }
  }
  x = winSize[0] * viewport[0];
  y = winSize[1] * viewport[1];
  renderer->SetDisplayPoint(x, y, tolZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowLowerLeft);

  x = winSize[0] * viewport[2];
  y = winSize[1] * viewport[3];
  renderer->SetDisplayPoint(x, y, tolZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowUpperRight);

  for (tol = 0.0, i = 0; i < 3; i++)
  {
    tol += (windowUpperRight[i] - windowLowerLeft[i]) * (windowUpperRight[i] - windowLowerLeft[i]);
  }

  tol = sqrt(tol) * this->Tolerance;

  //  Loop over all props.  Transform ray (defined from position of
  //  camera to selection point) into coordinates of mapper (not
  //  transformed to actors coordinates!  Reduces overall computation!!!).
  //  Note that only svtkProp3D's can be picked by svtkPicker.
  //
  svtkPropCollection* props;
  svtkProp* propCandidate;
  if (this->PickFromList)
  {
    props = this->GetPickList();
  }
  else
  {
    props = renderer->GetViewProps();
  }

  svtkActor* actor;
  svtkLODProp3D* prop3D;
  svtkVolume* volume;
  svtkImageSlice* imageSlice = nullptr;
  svtkAssemblyPath* path;
  svtkProperty* tempProperty;
  this->Transform->PostMultiply();
  svtkCollectionSimpleIterator pit;
  double scale[3];
  for (props->InitTraversal(pit); (prop = props->GetNextProp(pit));)
  {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
    {
      pickable = 0;
      actor = nullptr;
      propCandidate = path->GetLastNode()->GetViewProp();
      if (propCandidate->GetPickable() && propCandidate->GetVisibility())
      {
        pickable = 1;
        if ((actor = svtkActor::SafeDownCast(propCandidate)) != nullptr)
        {
          mapper = actor->GetMapper();
          if (actor->GetProperty()->GetOpacity() <= 0.0)
          {
            pickable = 0;
          }
        }
        else if ((prop3D = svtkLODProp3D::SafeDownCast(propCandidate)) != nullptr)
        {
          LODId = prop3D->GetPickLODID();
          mapper = prop3D->GetLODMapper(LODId);

          // if the mapper is a svtkMapper (as opposed to a svtkVolumeMapper),
          // then check the transparency to see if the object is pickable
          if (svtkMapper::SafeDownCast(mapper) != nullptr)
          {
            prop3D->GetLODProperty(LODId, &tempProperty);
            if (tempProperty->GetOpacity() <= 0.0)
            {
              pickable = 0;
            }
          }
        }
        else if ((volume = svtkVolume::SafeDownCast(propCandidate)) != nullptr)
        {
          mapper = volume->GetMapper();
        }
        else if ((imageSlice = svtkImageSlice::SafeDownCast(propCandidate)))
        {
          mapper = imageSlice->GetMapper();
        }
        else
        {
          pickable = 0; // only svtkProp3D's (actors and volumes) can be picked
        }
      }

      //  If actor can be picked, get its composite matrix, invert it, and
      //  use the inverted matrix to transform the ray points into mapper
      //  coordinates.
      if (pickable)
      {
        svtkMatrix4x4* lastMatrix = path->GetLastNode()->GetMatrix();
        if (lastMatrix == nullptr)
        {
          svtkErrorMacro(<< "Pick: Null matrix.");
          return 0;
        }
        this->Transform->SetMatrix(lastMatrix);
        this->Transform->Push();
        this->Transform->Inverse();
        this->Transform->GetScale(scale); // need to scale the tolerance

        this->Transform->TransformPoint(p1World, p1Mapper);
        this->Transform->TransformPoint(p2World, p2Mapper);

        for (i = 0; i < 3; i++)
        {
          ray[i] = p2Mapper[i] - p1Mapper[i];
        }

        this->Transform->Pop();

        //  Have the ray endpoints in mapper space, now need to compare this
        //  with the mapper bounds to see whether intersection is possible.
        //
        //  Get the bounding box of the modeller.  Note that the tolerance is
        //  added to the bounding box to make sure things on the edge of the
        //  bounding box are picked correctly.
        if (mapper != nullptr)
        {
          mapper->GetBounds(bounds);
        }

        bounds[0] -= tol;
        bounds[1] += tol;
        bounds[2] -= tol;
        bounds[3] += tol;
        bounds[4] -= tol;
        bounds[5] += tol;

        if (svtkBox::IntersectBox(bounds, p1Mapper, ray, hitPosition, t))
        {
          t = this->IntersectWithLine(p1Mapper, p2Mapper,
            tol * 0.333 * (scale[0] + scale[1] + scale[2]), path,
            static_cast<svtkProp3D*>(propCandidate), mapper);

          if (t < SVTK_DOUBLE_MAX)
          {
            double p[3];
            p[0] = (1.0 - t) * p1World[0] + t * p2World[0];
            p[1] = (1.0 - t) * p1World[1] + t * p2World[1];
            p[2] = (1.0 - t) * p1World[2] + t * p2World[2];

            // The IsItemPresent method returns "index+1"
            int prevIndex = this->Prop3Ds->IsItemPresent(prop) - 1;

            if (prevIndex >= 0)
            {
              // If already in list, set point to the closest point
              double oldp[3];
              this->PickedPositions->GetPoint(prevIndex, oldp);
              if (svtkMath::Distance2BetweenPoints(p1World, p) <
                svtkMath::Distance2BetweenPoints(p1World, oldp))
              {
                this->PickedPositions->SetPoint(prevIndex, p);
              }
            }
            else
            {
              this->Prop3Ds->AddItem(static_cast<svtkProp3D*>(prop));

              this->PickedPositions->InsertNextPoint(p);

              // backwards compatibility: also add to this->Actors
              if (actor)
              {
                this->Actors->AddItem(actor);
              }
            }
          }
        }
      } // if visible and pickable and not transparent
    }   // for all parts
  }     // for all actors

  int picked = 0;

  if (this->Path)
  {
    // Invoke pick method if one defined - prop goes first
    this->Path->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(svtkCommand::PickEvent, nullptr);
    picked = 1;
  }

  return picked;
}

//----------------------------------------------------------------------
// Intersect data with specified ray.
double svtkPicker::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  svtkAssemblyPath* path, svtkProp3D* prop3D, svtkAbstractMapper3D* mapper)
{
  double center[3], t, ray[3], rayFactor;

  // Get the data from the modeler
  if (mapper != nullptr)
  {
    mapper->GetCenter(center);
  }
  else
  {
    return SVTK_DOUBLE_MAX;
  }

  if (!svtkPicker::CalculateRay(p1, p2, ray, rayFactor))
  {
    svtkDebugMacro("Zero length ray");
    return 2.0;
  }

  // Project the center point onto the ray and determine its parametric value
  //
  t = (ray[0] * (center[0] - p1[0]) + ray[1] * (center[1] - p1[1]) + ray[2] * (center[2] - p1[2])) /
    rayFactor;

  if (t >= 0.0 && t <= 1.0 && t < this->GlobalTMin)
  {
    // If this is a composite dataset, find the nearest picked dataset
    svtkCompositeDataSet* composite =
      svtkCompositeDataSet::SafeDownCast(mapper->GetInputDataObject(0, 0));
    if (composite)
    {
      double tMinDS = SVTK_DOUBLE_MAX;
      double centerMinDS[3];
      svtkDataSet* minDS = nullptr;
      svtkIdType minDSIndex = -1;
      svtkSmartPointer<svtkCompositeDataIterator> iter;
      iter.TakeReference(composite->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (!ds)
        {
          continue;
        }

        // First check if the bounding box of the data set is hit.
        double bounds[6];
        ds->GetBounds(bounds);
        bounds[0] -= tol;
        bounds[1] += tol;
        bounds[2] -= tol;
        bounds[3] += tol;
        bounds[4] -= tol;
        bounds[5] += tol;
        double tDummy;
        double xyzDummy[3];
        if (!svtkBox::IntersectBox(bounds, p1, ray, xyzDummy, tDummy))
        {
          // box not hit: no need to intersect...
          continue;
        }

        double centerDS[3];
        ds->GetCenter(centerDS);

        // Project the center point onto the ray and
        // determine its parametric value
        double tDS = (ray[0] * (centerDS[0] - p1[0]) + ray[1] * (centerDS[1] - p1[1]) +
                       ray[2] * (centerDS[2] - p1[2])) /
          rayFactor;

        if (tDS >= 0.0 && tDS <= 1.0 && tDS < tMinDS)
        {
          tMinDS = tDS;
          centerMinDS[0] = centerDS[0];
          centerMinDS[1] = centerDS[1];
          centerMinDS[2] = centerDS[2];
          minDS = ds;
          minDSIndex = iter->GetCurrentFlatIndex();
        }
      }
      // Note that the mapper position is not the center of the entire
      // composite data set but the center of the nearest data set
      this->MarkPickedData(path, tMinDS, centerMinDS, mapper, minDS, minDSIndex);
    }
    else
    {
      this->MarkPicked(path, prop3D, mapper, t, center);
    }
  }
  return t;
}

bool svtkPicker::CalculateRay(
  const double p1[3], const double p2[3], double ray[3], double& rayFactor)
{
  ray[0] = p2[0] - p1[0];
  ray[1] = p2[1] - p1[1];
  ray[2] = p2[2] - p1[2];

  rayFactor = svtkMath::Dot(ray, ray);
  return (rayFactor > 0.0);
}

//----------------------------------------------------------------------
// Initialize the picking process.
void svtkPicker::Initialize()
{
  this->svtkAbstractPropPicker::Initialize();

  this->Actors->RemoveAllItems();
  this->Prop3Ds->RemoveAllItems();
  this->PickedPositions->Reset();

  this->MapperPosition[0] = 0.0;
  this->MapperPosition[1] = 0.0;
  this->MapperPosition[2] = 0.0;

  this->Mapper = nullptr;
  this->DataSet = nullptr;
  this->CompositeDataSet = nullptr;
  this->FlatBlockIndex = -1;
  this->GlobalTMin = SVTK_DOUBLE_MAX;
}

//----------------------------------------------------------------------
svtkActorCollection* svtkPicker::GetActors()
{
  if (this->Actors->GetNumberOfItems() != this->PickedPositions->GetNumberOfPoints())
  {
    svtkWarningMacro(<< "Not all Prop3Ds are actors, use GetProp3Ds instead");
  }
  return this->Actors;
}

//----------------------------------------------------------------------
void svtkPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->DataSet)
  {
    os << indent << "DataSet: " << this->DataSet << "\n";
  }
  else
  {
    os << indent << "DataSet: (none)";
  }
  if (this->CompositeDataSet)
  {
    os << indent << "CompositeDataSet: " << this->CompositeDataSet << "\n";
  }
  else
  {
    os << indent << "CompositeDataSet: (none)\n";
  }
  if (this->FlatBlockIndex > -1)
  {
    os << indent << "FlatBlockIndex: " << this->FlatBlockIndex << "\n";
  }
  else
  {
    os << indent << "FlatBlockIndex: (none)\n";
  }

  os << indent << "Mapper: " << this->Mapper << "\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "MapperPosition: (" << this->MapperPosition[0] << "," << this->MapperPosition[1]
     << "," << this->MapperPosition[2] << ")\n";
}
