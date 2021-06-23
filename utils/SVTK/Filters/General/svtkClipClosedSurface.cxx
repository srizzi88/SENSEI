/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipClosedSurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClipClosedSurface.h"

#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkContourTriangulator.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPlaneCollection.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkSignedCharArray.h"
#include "svtkTriangleStrip.h"
#include "svtkUnsignedCharArray.h"

#include "svtkIncrementalOctreePointLocator.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkClipClosedSurface);

svtkCxxSetObjectMacro(svtkClipClosedSurface, ClippingPlanes, svtkPlaneCollection);

//----------------------------------------------------------------------------
svtkClipClosedSurface::svtkClipClosedSurface()
{
  this->ClippingPlanes = nullptr;
  this->Tolerance = 1e-6;
  this->PassPointData = 0;

  this->ScalarMode = SVTK_CCS_SCALAR_MODE_NONE;
  this->GenerateOutline = 0;
  this->GenerateFaces = 1;
  this->ActivePlaneId = -1;

  this->BaseColor[0] = 1.0;
  this->BaseColor[1] = 0.0;
  this->BaseColor[2] = 0.0;

  this->ClipColor[0] = 1.0;
  this->ClipColor[1] = 0.5;
  this->ClipColor[2] = 0.0;

  this->ActivePlaneColor[0] = 1.0;
  this->ActivePlaneColor[1] = 1.0;
  this->ActivePlaneColor[2] = 0.0;

  this->TriangulationErrorDisplay = 0;

  // A whole bunch of objects needed during execution
  this->IdList = nullptr;
}

//----------------------------------------------------------------------------
svtkClipClosedSurface::~svtkClipClosedSurface()
{
  if (this->ClippingPlanes)
  {
    this->ClippingPlanes->Delete();
  }

  if (this->IdList)
  {
    this->IdList->Delete();
  }
}

//----------------------------------------------------------------------------
const char* svtkClipClosedSurface::GetScalarModeAsString()
{
  switch (this->ScalarMode)
  {
    case SVTK_CCS_SCALAR_MODE_NONE:
      return "None";
    case SVTK_CCS_SCALAR_MODE_COLORS:
      return "Colors";
    case SVTK_CCS_SCALAR_MODE_LABELS:
      return "Labels";
  }
  return "";
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ClippingPlanes: ";
  if (this->ClippingPlanes)
  {
    os << this->ClippingPlanes << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "PassPointData: " << (this->PassPointData ? "On\n" : "Off\n");

  os << indent << "GenerateOutline: " << (this->GenerateOutline ? "On\n" : "Off\n");

  os << indent << "GenerateFaces: " << (this->GenerateFaces ? "On\n" : "Off\n");

  os << indent << "ScalarMode: " << this->GetScalarModeAsString() << "\n";

  os << indent << "BaseColor: " << this->BaseColor[0] << ", " << this->BaseColor[1] << ", "
     << this->BaseColor[2] << "\n";

  os << indent << "ClipColor: " << this->ClipColor[0] << ", " << this->ClipColor[1] << ", "
     << this->ClipColor[2] << "\n";

  os << indent << "ActivePlaneId: " << this->ActivePlaneId << "\n";

  os << indent << "ActivePlaneColor: " << this->ActivePlaneColor[0] << ", "
     << this->ActivePlaneColor[1] << ", " << this->ActivePlaneColor[2] << "\n";

  os << indent
     << "TriangulationErrorDisplay: " << (this->TriangulationErrorDisplay ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
int svtkClipClosedSurface::ComputePipelineMTime(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  int svtkNotUsed(requestFromOutputPort), svtkMTimeType* mtime)
{
  svtkMTimeType mTime = this->GetMTime();

  svtkPlaneCollection* planes = this->ClippingPlanes;
  svtkPlane* plane = nullptr;

  if (planes)
  {
    svtkMTimeType planesMTime = planes->GetMTime();
    if (planesMTime > mTime)
    {
      mTime = planesMTime;
    }

    svtkCollectionSimpleIterator iter;
    planes->InitTraversal(iter);
    while ((plane = planes->GetNextPlane(iter)))
    {
      svtkMTimeType planeMTime = plane->GetMTime();
      if (planeMTime > mTime)
      {
        mTime = planeMTime;
      }
    }
  }

  *mtime = mTime;

  return 1;
}

//----------------------------------------------------------------------------
// A helper class to quickly locate an edge, given the endpoint ids.
// It uses an stl map rather than a table partitioning scheme, since
// we have no idea how many entries there will be when we start.  So
// the performance is approximately log(n).

class svtkCCSEdgeLocatorNode
{
public:
  svtkCCSEdgeLocatorNode()
    : ptId0(-1)
    , ptId1(-1)
    , edgeId(-1)
    , next(nullptr)
  {
  }

  void FreeList()
  {
    svtkCCSEdgeLocatorNode* ptr = this->next;
    while (ptr)
    {
      svtkCCSEdgeLocatorNode* tmp = ptr;
      ptr = ptr->next;
      tmp->next = nullptr;
      delete tmp;
    }
  }

  svtkIdType ptId0;
  svtkIdType ptId1;
  svtkIdType edgeId;
  svtkCCSEdgeLocatorNode* next;
};

class svtkCCSEdgeLocator
{
private:
  typedef std::map<svtkIdType, svtkCCSEdgeLocatorNode> MapType;
  MapType EdgeMap;

public:
  static svtkCCSEdgeLocator* New() { return new svtkCCSEdgeLocator; }

  void Delete()
  {
    this->Initialize();
    delete this;
  }

  // Description:
  // Initialize the locator.
  void Initialize();

  // Description:
  // If edge (i0, i1) is not in the list, then it will be added and
  // a pointer for storing the new edgeId will be returned.
  // If edge (i0, i1) is in the list, then edgeId will be set to the
  // stored value and a null pointer will be returned.
  svtkIdType* InsertUniqueEdge(svtkIdType i0, svtkIdType i1, svtkIdType& edgeId);
};

void svtkCCSEdgeLocator::Initialize()
{
  for (MapType::iterator i = this->EdgeMap.begin(); i != this->EdgeMap.end(); ++i)
  {
    i->second.FreeList();
  }
  this->EdgeMap.clear();
}

svtkIdType* svtkCCSEdgeLocator::InsertUniqueEdge(svtkIdType i0, svtkIdType i1, svtkIdType& edgeId)
{
  // Ensure consistent ordering of edge
  if (i1 < i0)
  {
    svtkIdType tmp = i0;
    i0 = i1;
    i1 = tmp;
  }

  // Generate a integer key, try to make it unique
#ifdef SVTK_USE_64BIT_IDS
  svtkIdType key = ((i1 << 32) ^ i0);
#else
  svtkIdType key = ((i1 << 16) ^ i0);
#endif

  svtkCCSEdgeLocatorNode* node = &this->EdgeMap[key];

  if (node->ptId1 < 0)
  {
    // Didn't find key, so add a new edge entry
    node->ptId0 = i0;
    node->ptId1 = i1;
    return &node->edgeId;
  }

  // Search through the list for i0 and i1
  if (node->ptId0 == i0 && node->ptId1 == i1)
  {
    edgeId = node->edgeId;
    return nullptr;
  }

  int i = 1;
  while (node->next != nullptr)
  {
    i++;
    node = node->next;

    if (node->ptId0 == i0 && node->ptId1 == i1)
    {
      edgeId = node->edgeId;
      return nullptr;
    }
  }

  // No entry for i1, so make one and return
  node->next = new svtkCCSEdgeLocatorNode;
  node = node->next;
  node->ptId0 = i0;
  node->ptId1 = i1;
  node->edgeId = static_cast<svtkIdType>(this->EdgeMap.size() - 1);
  return &node->edgeId;
}

//----------------------------------------------------------------------------
int svtkClipClosedSurface::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Create objects needed for temporary storage
  if (this->IdList == nullptr)
  {
    this->IdList = svtkIdList::New();
  }

  // Get the input points
  svtkPoints* inputPoints = input->GetPoints();
  svtkIdType numPts = 0;
  int inputPointsType = SVTK_FLOAT;
  if (inputPoints)
  {
    numPts = inputPoints->GetNumberOfPoints();
    inputPointsType = inputPoints->GetDataType();
  }

  // Force points to double precision, copy the point attributes
  svtkPoints* points = svtkPoints::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPts);

  svtkPointData* pointData = svtkPointData::New();
  svtkPointData* inPointData = nullptr;

  if (this->PassPointData)
  {
    inPointData = input->GetPointData();
    pointData->InterpolateAllocate(inPointData, numPts, 0);
  }

  for (svtkIdType ptId = 0; ptId < numPts; ptId++)
  {
    double point[3];
    inputPoints->GetPoint(ptId, point);
    points->SetPoint(ptId, point);
    // Point data is not copied from input
    if (inPointData)
    {
      pointData->CopyData(inPointData, ptId, ptId);
    }
  }

  // An edge locator to avoid point duplication while clipping
  svtkCCSEdgeLocator* edgeLocator = svtkCCSEdgeLocator::New();

  // A temporary polydata for the contour lines that are triangulated
  svtkPolyData* tmpContourData = svtkPolyData::New();

  // The cell scalars
  svtkUnsignedCharArray* lineScalars = nullptr;
  svtkUnsignedCharArray* polyScalars = nullptr;
  svtkUnsignedCharArray* inputScalars = nullptr;

  // For input scalars: the offsets to the various cell types
  svtkIdType firstLineScalar = 0;
  svtkIdType firstPolyScalar = 0;
  svtkIdType firstStripScalar = 0;

  // Make the colors to be used on the data.
  int numberOfScalarComponents = 1;
  unsigned char colors[3][3];

  if (this->ScalarMode == SVTK_CCS_SCALAR_MODE_COLORS)
  {
    numberOfScalarComponents = 3;
    this->CreateColorValues(this->BaseColor, this->ClipColor, this->ActivePlaneColor, colors);
  }
  else if (this->ScalarMode == SVTK_CCS_SCALAR_MODE_LABELS)
  {
    colors[0][0] = 0;
    colors[1][0] = 1;
    colors[2][0] = 2;
  }

  // This is set if we have to work with scalars.  The input scalars
  // will be copied if they are unsigned char with 3 components, otherwise
  // new scalars will be generated.
  if (this->ScalarMode)
  {
    // Make the scalars
    lineScalars = svtkUnsignedCharArray::New();
    lineScalars->SetNumberOfComponents(numberOfScalarComponents);

    svtkDataArray* tryInputScalars = input->GetCellData()->GetScalars();
    // Get input scalars if they are RGB color scalars
    if (tryInputScalars && tryInputScalars->IsA("svtkUnsignedCharArray") &&
      numberOfScalarComponents == 3 && tryInputScalars->GetNumberOfComponents() == 3)
    {
      inputScalars = static_cast<svtkUnsignedCharArray*>(input->GetCellData()->GetScalars());

      svtkIdType numVerts = 0;
      svtkIdType numLines = 0;
      svtkIdType numPolys = 0;
      svtkCellArray* tmpCellArray = nullptr;
      if ((tmpCellArray = input->GetVerts()))
      {
        numVerts = tmpCellArray->GetNumberOfCells();
      }
      if ((tmpCellArray = input->GetLines()))
      {
        numLines = tmpCellArray->GetNumberOfCells();
      }
      if ((tmpCellArray = input->GetPolys()))
      {
        numPolys = tmpCellArray->GetNumberOfCells();
      }
      firstLineScalar = numVerts;
      firstPolyScalar = numVerts + numLines;
      firstStripScalar = numVerts + numLines + numPolys;
    }
  }

  // Break the input lines into segments, generate scalars for lines
  svtkCellArray* lines = svtkCellArray::New();
  if (input->GetLines() && input->GetLines()->GetNumberOfCells() > 0)
  {
    this->BreakPolylines(
      input->GetLines(), lines, inputScalars, firstLineScalar, lineScalars, colors[0]);
  }

  // Copy the polygons, convert strips to triangles
  svtkCellArray* polys = nullptr;
  int polyMax = 3;
  if ((input->GetPolys() && input->GetPolys()->GetNumberOfCells() > 0) ||
    (input->GetStrips() && input->GetStrips()->GetNumberOfCells() > 0))
  {
    // If there are line scalars, then poly scalars are needed too
    if (lineScalars)
    {
      polyScalars = svtkUnsignedCharArray::New();
      polyScalars->SetNumberOfComponents(numberOfScalarComponents);
    }

    polys = svtkCellArray::New();
    this->CopyPolygons(
      input->GetPolys(), polys, inputScalars, firstPolyScalar, polyScalars, colors[0]);
    this->BreakTriangleStrips(
      input->GetStrips(), polys, inputScalars, firstStripScalar, polyScalars, colors[0]);

    // Check if the input has polys and quads or just triangles
    svtkIdType npts = 0;
    const svtkIdType* pts = nullptr;
    svtkCellArray* inPolys = input->GetPolys();
    inPolys->InitTraversal();
    while (inPolys->GetNextCell(npts, pts))
    {
      if (npts > polyMax)
      {
        polyMax = npts;
      }
    }
  }

  // Get the clipping planes
  svtkPlaneCollection* planes = this->ClippingPlanes;

  // Arrays for storing the clipped lines and polys.
  svtkCellArray* newLines = svtkCellArray::New();
  svtkCellArray* newPolys = nullptr;
  if (polys)
  {
    newPolys = svtkCellArray::New();
  }

  // The point scalars, needed for clipping (not for the output!)
  svtkDoubleArray* pointScalars = svtkDoubleArray::New();

  // The line scalars, for coloring the outline
  svtkCellData* inLineData = svtkCellData::New();
  inLineData->CopyScalarsOn();
  inLineData->SetScalars(lineScalars);
  if (lineScalars)
  {
    lineScalars->Delete();
    lineScalars = nullptr;
  }

  // The poly scalars, for coloring the faces
  svtkCellData* inPolyData = svtkCellData::New();
  inPolyData->CopyScalarsOn();
  inPolyData->SetScalars(polyScalars);
  if (polyScalars)
  {
    polyScalars->Delete();
    polyScalars = nullptr;
  }

  // Also create output attribute data
  svtkCellData* outLineData = svtkCellData::New();
  outLineData->CopyScalarsOn();

  svtkCellData* outPolyData = svtkCellData::New();
  outPolyData->CopyScalarsOn();

  // Go through the clipping planes and clip the input with each plane
  svtkCollectionSimpleIterator iter;
  int numPlanes = 0;
  if (planes)
  {
    planes->InitTraversal(iter);
    numPlanes = planes->GetNumberOfItems();
  }

  svtkPlane* plane = nullptr;
  for (int planeId = 0; planes && (plane = planes->GetNextPlane(iter)); planeId++)
  {
    this->UpdateProgress((planeId + 1.0) / (numPlanes + 1.0));
    if (this->GetAbortExecute())
    {
      break;
    }

    // Is this the last cut plane?  If so, generate triangles.
    int triangulate = 5;
    if (planeId == numPlanes - 1)
    {
      triangulate = polyMax;
    }

    // Is this the active plane?
    int active = (planeId == this->ActivePlaneId);

    // Convert the plane into an easy-to-evaluate function
    double pc[4];
    plane->GetNormal(pc);
    pc[3] = -svtkMath::Dot(pc, plane->GetOrigin());

    // Create the clip scalars by evaluating the plane at each point
    svtkIdType numPoints = points->GetNumberOfPoints();
    pointScalars->SetNumberOfValues(numPoints);
    for (svtkIdType pointId = 0; pointId < numPoints; pointId++)
    {
      double p[3];
      points->GetPoint(pointId, p);
      double val = p[0] * pc[0] + p[1] * pc[1] + p[2] * pc[2] + pc[3];
      pointScalars->SetValue(pointId, val);
    }

    // Prepare the output scalars
    outLineData->CopyAllocate(inLineData, 0, 0);
    outPolyData->CopyAllocate(inPolyData, 0, 0);

    // Reset the locator
    edgeLocator->Initialize();

    // Clip the lines
    this->ClipLines(
      points, pointScalars, pointData, edgeLocator, lines, newLines, inLineData, outLineData);

    // Clip the polys
    if (polys)
    {
      // Get the number of lines remaining after the clipping
      svtkIdType numClipLines = newLines->GetNumberOfCells();

      // Cut the polys to generate more lines
      this->ClipAndContourPolys(points, pointScalars, pointData, edgeLocator, triangulate, polys,
        newPolys, newLines, inPolyData, outPolyData, outLineData);

      // Add scalars for the newly-created contour lines
      svtkUnsignedCharArray* scalars =
        svtkArrayDownCast<svtkUnsignedCharArray>(outLineData->GetScalars());

      if (scalars)
      {
        // Set the color to the active color if plane is active
        unsigned char* color = colors[1 + active];
        unsigned char* activeColor = colors[2];

        svtkIdType numLines = newLines->GetNumberOfCells();
        for (svtkIdType lineId = numClipLines; lineId < numLines; lineId++)
        {
          unsigned char oldColor[3];
          scalars->GetTypedTuple(lineId, oldColor);
          if (numberOfScalarComponents != 3 || oldColor[0] != activeColor[0] ||
            oldColor[1] != activeColor[1] || oldColor[2] != activeColor[2])
          {
            scalars->SetTypedTuple(lineId, color);
          }
        }
      }

      // Generate new polys from the cut lines
      svtkIdType cellId = newPolys->GetNumberOfCells();
      svtkIdType numClipAndContourLines = newLines->GetNumberOfCells();

      // Create a polydata for the lines
      tmpContourData->SetPoints(points);
      tmpContourData->SetLines(newLines);
      tmpContourData->BuildCells();

      this->TriangulateContours(
        tmpContourData, numClipLines, numClipAndContourLines - numClipLines, newPolys, pc);

      // Add scalars for the newly-created polys
      scalars = svtkArrayDownCast<svtkUnsignedCharArray>(outPolyData->GetScalars());

      if (scalars)
      {
        unsigned char* color = colors[1 + active];

        svtkIdType numCells = newPolys->GetNumberOfCells();
        if (numCells > cellId)
        {
          // The insert allocates space up to numCells-1
          scalars->InsertTypedTuple(numCells - 1, color);
          for (; cellId < numCells; cellId++)
          {
            scalars->SetTypedTuple(cellId, color);
          }
        }
      }

      // Add scalars to any diagnostic lines that added by
      // TriangulateContours().  In usual operation, no lines are added.
      scalars = svtkArrayDownCast<svtkUnsignedCharArray>(outLineData->GetScalars());

      if (scalars)
      {
        unsigned char color[3] = { 0, 255, 255 };

        svtkIdType numCells = newLines->GetNumberOfCells();
        if (numCells > numClipAndContourLines)
        {
          // The insert allocates space up to numCells-1
          scalars->InsertTypedTuple(numCells - 1, color);
          for (svtkIdType lineCellId = numClipAndContourLines; lineCellId < numCells; lineCellId++)
          {
            scalars->SetTypedTuple(lineCellId, color);
          }
        }
      }
    }

    // Swap the lines, points, etcetera: old output becomes new input
    svtkCellArray* tmp1 = lines;
    lines = newLines;
    newLines = tmp1;
    newLines->Initialize();

    if (polys)
    {
      svtkCellArray* tmp2 = polys;
      polys = newPolys;
      newPolys = tmp2;
      newPolys->Initialize();
    }

    svtkCellData* tmp4 = inLineData;
    inLineData = outLineData;
    outLineData = tmp4;
    outLineData->Initialize();

    svtkCellData* tmp5 = inPolyData;
    inPolyData = outPolyData;
    outPolyData = tmp5;
    outPolyData->Initialize();
  }

  // Delete the locator
  edgeLocator->Delete();

  // Delete the contour data container
  tmpContourData->Delete();

  // Delete the clip scalars
  pointScalars->Delete();

  // Get the line scalars
  svtkUnsignedCharArray* scalars = svtkArrayDownCast<svtkUnsignedCharArray>(inLineData->GetScalars());

  if (this->GenerateOutline)
  {
    output->SetLines(lines);
  }
  else if (scalars)
  {
    // If not adding lines to output, clear the line scalars
    scalars->Initialize();
  }

  if (this->GenerateFaces)
  {
    output->SetPolys(polys);

    if (polys && scalars)
    {
      svtkUnsignedCharArray* pScalars =
        svtkArrayDownCast<svtkUnsignedCharArray>(inPolyData->GetScalars());

      svtkIdType m = scalars->GetNumberOfTuples();
      svtkIdType n = pScalars->GetNumberOfTuples();

      if (n > 0)
      {
        unsigned char color[3];
        color[0] = color[1] = color[2] = 0;

        // This is just to expand the array
        scalars->InsertTypedTuple(n + m - 1, color);

        // Fill in the poly scalars
        for (svtkIdType i = 0; i < n; i++)
        {
          pScalars->GetTypedTuple(i, color);
          scalars->SetTypedTuple(i + m, color);
        }
      }
    }
  }

  lines->Delete();

  if (polys)
  {
    polys->Delete();
  }

  if (this->ScalarMode == SVTK_CCS_SCALAR_MODE_COLORS)
  {
    scalars->SetName("Colors");
    output->GetCellData()->SetScalars(scalars);
  }
  else if (this->ScalarMode == SVTK_CCS_SCALAR_MODE_LABELS)
  {
    // Don't use SVTK_UNSIGNED_CHAR or they will look like color scalars
    svtkSignedCharArray* categories = svtkSignedCharArray::New();
    categories->DeepCopy(scalars);
    categories->SetName("Labels");
    output->GetCellData()->SetScalars(categories);
    categories->Delete();
  }
  else
  {
    output->GetCellData()->SetScalars(nullptr);
  }

  newLines->Delete();
  if (newPolys)
  {
    newPolys->Delete();
  }

  inLineData->Delete();
  outLineData->Delete();
  inPolyData->Delete();
  outPolyData->Delete();

  // Finally, store the points in the output
  this->SqueezeOutputPoints(output, points, pointData, inputPointsType);
  output->Squeeze();

  points->Delete();
  pointData->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::SqueezeOutputPoints(
  svtkPolyData* output, svtkPoints* points, svtkPointData* pointData, int outputPointDataType)
{
  // Create a list of points used by cells
  svtkIdType n = points->GetNumberOfPoints();
  svtkIdType numNewPoints = 0;

  // The point data
  svtkPointData* outPointData = output->GetPointData();

  // A mapping from old pointIds to new pointIds
  svtkIdType* pointMap = new svtkIdType[n];
  for (svtkIdType i = 0; i < n; i++)
  {
    pointMap[i] = -1;
  }

  svtkIdType npts;
  const svtkIdType* pts;
  svtkCellArray* cellArrays[4];
  cellArrays[0] = output->GetVerts();
  cellArrays[1] = output->GetLines();
  cellArrays[2] = output->GetPolys();
  cellArrays[3] = output->GetStrips();
  int arrayId;

  // Find all the newPoints that are used by cells
  for (arrayId = 0; arrayId < 4; arrayId++)
  {
    svtkCellArray* cellArray = cellArrays[arrayId];
    if (cellArray)
    {
      cellArray->InitTraversal();
      while (cellArray->GetNextCell(npts, pts))
      {
        for (svtkIdType ii = 0; ii < npts; ii++)
        {
          svtkIdType pointId = pts[ii];
          if (pointMap[pointId] < 0)
          {
            pointMap[pointId] = numNewPoints++;
          }
        }
      }
    }
  }

  // Create exactly the number of points that are required
  svtkPoints* newPoints = svtkPoints::New();
  newPoints->SetDataType(outputPointDataType);
  newPoints->SetNumberOfPoints(numNewPoints);
  outPointData->CopyAllocate(pointData, numNewPoints, 0);

  for (svtkIdType pointId = 0; pointId < n; pointId++)
  {
    svtkIdType newPointId = pointMap[pointId];
    if (newPointId >= 0)
    {
      double p[3];
      points->GetPoint(pointId, p);
      newPoints->SetPoint(newPointId, p);
      outPointData->CopyData(pointData, pointId, newPointId);
    }
  }

  // Change the cell pointIds to reflect the new point array
  svtkNew<svtkIdList> repCell;
  for (arrayId = 0; arrayId < 4; arrayId++)
  {
    svtkCellArray* cellArray = cellArrays[arrayId];
    if (cellArray)
    {
      auto cellIter = svtkSmartPointer<svtkCellArrayIterator>::Take(cellArray->NewIterator());
      for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
      {
        cellIter->GetCurrentCell(repCell);
        for (svtkIdType ii = 0; ii < repCell->GetNumberOfIds(); ii++)
        {
          const svtkIdType pointId = repCell->GetId(ii);
          repCell->SetId(ii, pointMap[pointId]);
        }
        cellIter->ReplaceCurrentCell(repCell);
      }
    }
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  delete[] pointMap;
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::CreateColorValues(const double color1[3], const double color2[3],
  const double color3[3], unsigned char colors[3][3])
{
  // Convert colors from "double" to "unsigned char"

  const double* dcolors[3];
  dcolors[0] = color1;
  dcolors[1] = color2;
  dcolors[2] = color3;

  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      double val = dcolors[i][j];
      if (val < 0)
      {
        val = 0;
      }
      if (val > 1)
      {
        val = 1;
      }
      colors[i][j] = static_cast<unsigned char>(val * 255);
    }
  }
}

//----------------------------------------------------------------------------
// Point interpolation for clipping and contouring, given the scalar
// values (v0, v1) for the two endpoints (p0, p1).  The use of this
// function guarantees perfect consistency in the results.
int svtkClipClosedSurface::InterpolateEdge(svtkPoints* points, svtkPointData* pointData,
  svtkCCSEdgeLocator* locator, double tol, svtkIdType i0, svtkIdType i1, double v0, double v1,
  svtkIdType& i)
{
  // This swap guarantees that exactly the same point is computed
  // for both line directions, as long as the endpoints are the same.
  if (v1 > 0)
  {
    svtkIdType tmpi = i0;
    i0 = i1;
    i1 = tmpi;

    double tmp = v0;
    v0 = v1;
    v1 = tmp;
  }

  // After the above swap, i0 will be kept, and i1 will be clipped

  // Check to see if this point has already been computed
  svtkIdType* iptr = locator->InsertUniqueEdge(i0, i1, i);
  if (iptr == nullptr)
  {
    return 0;
  }

  // Get the edge and interpolate the new point
  double p0[3], p1[3], p[3];
  points->GetPoint(i0, p0);
  points->GetPoint(i1, p1);

  double f = v0 / (v0 - v1);
  double s = 1.0 - f;
  double t = 1.0 - s;

  p[0] = s * p0[0] + t * p1[0];
  p[1] = s * p0[1] + t * p1[1];
  p[2] = s * p0[2] + t * p1[2];

  double tol2 = tol * tol;

  // Make sure that new point is far enough from kept point
  if (svtkMath::Distance2BetweenPoints(p, p0) < tol2)
  {
    i = i0;
    *iptr = i0;
    return 0;
  }

  if (svtkMath::Distance2BetweenPoints(p, p1) < tol2)
  {
    i = i1;
    *iptr = i1;
    return 0;
  }

  i = points->InsertNextPoint(p);
  pointData->InterpolateEdge(pointData, i, i0, i1, t);

  // Store the new index in the locator
  *iptr = i;

  return 1;
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::ClipLines(svtkPoints* points, svtkDoubleArray* pointScalars,
  svtkPointData* pointData, svtkCCSEdgeLocator* edgeLocator, svtkCellArray* inputCells,
  svtkCellArray* outputLines, svtkCellData* inCellData, svtkCellData* outLineData)
{
  svtkIdType numCells = inputCells->GetNumberOfCells();

  inputCells->InitTraversal();
  for (svtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    svtkIdType numPts = 0;
    const svtkIdType* pts = nullptr;
    inputCells->GetNextCell(numPts, pts);

    svtkIdType i1 = pts[0];
    double v1 = pointScalars->GetValue(i1);
    int c1 = (v1 > 0);

    for (svtkIdType i = 1; i < numPts; i++)
    {
      svtkIdType i0 = i1;
      double v0 = v1;
      int c0 = c1;

      i1 = pts[i];
      v1 = pointScalars->GetValue(i1);
      c1 = (v1 > 0);

      // If at least one point wasn't clipped
      if ((c0 | c1))
      {
        svtkIdType linePts[2];
        linePts[0] = i0;
        linePts[1] = i1;

        // If only one end was clipped, interpolate new point
        if ((c0 ^ c1))
        {
          svtkClipClosedSurface::InterpolateEdge(
            points, pointData, edgeLocator, this->Tolerance, i0, i1, v0, v1, linePts[c0]);
        }

        // If endpoints are different, insert the line segment
        if (linePts[0] != linePts[1])
        {
          svtkIdType newCellId = outputLines->InsertNextCell(2, linePts);
          outLineData->CopyData(inCellData, cellId, newCellId);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::ClipAndContourPolys(svtkPoints* points, svtkDoubleArray* pointScalars,
  svtkPointData* pointData, svtkCCSEdgeLocator* edgeLocator, int triangulate,
  svtkCellArray* inputCells, svtkCellArray* outputPolys, svtkCellArray* outputLines,
  svtkCellData* inCellData, svtkCellData* outPolyData, svtkCellData* outLineData)
{
  svtkIdList* idList = this->IdList;

  // How many sides for output polygons?
  int polyMax = SVTK_INT_MAX;
  if (triangulate)
  {
    if (triangulate < 4)
    { // triangles only
      polyMax = 3;
    }
    else if (triangulate == 4)
    { // allow triangles and quads
      polyMax = 4;
    }
  }

  int triangulationFailure = 0;

  // Go through all cells and clip them.
  svtkIdType numCells = inputCells->GetNumberOfCells();

  inputCells->InitTraversal();
  for (svtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    svtkIdType numPts = 0;
    const svtkIdType* pts = nullptr;
    inputCells->GetNextCell(numPts, pts);
    idList->Reset();

    svtkIdType i1 = pts[numPts - 1];
    double v1 = pointScalars->GetValue(i1);
    int c1 = (v1 > 0);

    // The ids for the current edge: init j0 to -1 if i1 will be clipped
    svtkIdType j0 = (c1 ? i1 : -1);
    svtkIdType j1 = 0;

    // To store the ids of the contour line
    svtkIdType linePts[2];
    linePts[0] = 0;
    linePts[1] = 0;

    for (svtkIdType i = 0; i < numPts; i++)
    {
      // Save previous point info
      svtkIdType i0 = i1;
      double v0 = v1;
      int c0 = c1;

      // Generate new point info
      i1 = pts[i];
      v1 = pointScalars->GetValue(i1);
      c1 = (v1 > 0);

      // If at least one edge end point wasn't clipped
      if ((c0 | c1))
      {
        // If only one end was clipped, interpolate new point
        if ((c0 ^ c1))
        {
          svtkClipClosedSurface::InterpolateEdge(
            points, pointData, edgeLocator, this->Tolerance, i0, i1, v0, v1, j1);

          if (j1 != j0)
          {
            idList->InsertNextId(j1);
            j0 = j1;
          }

          // Save as one end of the contour line
          linePts[c0] = j1;
        }

        if (c1)
        {
          j1 = i1;

          if (j1 != j0)
          {
            idList->InsertNextId(j1);
            j0 = j1;
          }
        }
      }
    }

    // Insert the clipped poly
    svtkIdType numPoints = idList->GetNumberOfIds();

    if (numPoints > polyMax)
    {
      svtkIdType newCellId = outputPolys->GetNumberOfCells();

      // Triangulate the poly and insert triangles into output.
      if (!this->TriangulatePolygon(idList, points, outputPolys))
      {
        triangulationFailure = 1;
      }

      // Copy the attribute data to the triangle cells
      svtkIdType nCells = outputPolys->GetNumberOfCells();
      for (; newCellId < nCells; newCellId++)
      {
        outPolyData->CopyData(inCellData, cellId, newCellId);
      }
    }
    else if (numPoints > 2)
    {
      // Insert the polygon without triangulating it
      svtkIdType newCellId = outputPolys->InsertNextCell(idList);
      outPolyData->CopyData(inCellData, cellId, newCellId);
    }

    // Insert the contour line if one was created
    if (linePts[0] != linePts[1])
    {
      svtkIdType newCellId = outputLines->InsertNextCell(2, linePts);
      outLineData->CopyData(inCellData, cellId, newCellId);
    }
  }

  if (triangulationFailure && this->TriangulationErrorDisplay)
  {
    svtkErrorMacro("Triangulation failed, output may not be watertight");
  }

  // Free up the idList memory
  idList->Initialize();
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::BreakPolylines(svtkCellArray* inputLines, svtkCellArray* lines,
  svtkUnsignedCharArray* inputScalars, svtkIdType firstLineScalar, svtkUnsignedCharArray* scalars,
  const unsigned char color[3])
{
  // The color for the lines
  unsigned char cellColor[3];
  cellColor[0] = color[0];
  cellColor[1] = color[1];
  cellColor[2] = color[2];

  // Break the input lines into segments
  inputLines->InitTraversal();
  svtkIdType cellId = 0;
  svtkIdType npts;
  const svtkIdType* pts;
  while (inputLines->GetNextCell(npts, pts))
  {
    if (inputScalars)
    {
      inputScalars->GetTypedTuple(firstLineScalar + cellId++, cellColor);
    }

    for (svtkIdType i = 1; i < npts; i++)
    {
      lines->InsertNextCell(2);
      lines->InsertCellPoint(pts[i - 1]);
      lines->InsertCellPoint(pts[i]);

      if (scalars)
      {
        scalars->InsertNextTypedTuple(cellColor);
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::CopyPolygons(svtkCellArray* inputPolys, svtkCellArray* polys,
  svtkUnsignedCharArray* inputScalars, svtkIdType firstPolyScalar, svtkUnsignedCharArray* polyScalars,
  const unsigned char color[3])
{
  if (!inputPolys)
  {
    return;
  }

  polys->DeepCopy(inputPolys);

  if (polyScalars)
  {
    unsigned char scalarValue[3];
    scalarValue[0] = color[0];
    scalarValue[1] = color[1];
    scalarValue[2] = color[2];

    svtkIdType n = polys->GetNumberOfCells();
    polyScalars->SetNumberOfTuples(n);

    if (inputScalars)
    {
      for (svtkIdType i = 0; i < n; i++)
      {
        inputScalars->GetTypedTuple(i + firstPolyScalar, scalarValue);
        polyScalars->SetTypedTuple(i, scalarValue);
      }
    }
    else
    {
      for (svtkIdType i = 0; i < n; i++)
      {
        polyScalars->SetTypedTuple(i, scalarValue);
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::BreakTriangleStrips(svtkCellArray* inputStrips, svtkCellArray* polys,
  svtkUnsignedCharArray* inputScalars, svtkIdType firstStripScalar, svtkUnsignedCharArray* polyScalars,
  const unsigned char color[3])
{
  if (!inputStrips)
  {
    return;
  }

  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;

  inputStrips->InitTraversal();

  for (svtkIdType cellId = firstStripScalar; inputStrips->GetNextCell(npts, pts); cellId++)
  {
    svtkTriangleStrip::DecomposeStrip(npts, pts, polys);

    if (polyScalars)
    {
      unsigned char scalarValue[3];
      scalarValue[0] = color[0];
      scalarValue[1] = color[1];
      scalarValue[2] = color[2];

      if (inputScalars)
      {
        // If there are input scalars, use them instead of "color"
        inputScalars->GetTypedTuple(cellId, scalarValue);
      }

      svtkIdType n = npts - 3;
      svtkIdType m = polyScalars->GetNumberOfTuples();
      if (n >= 0)
      {
        // First insert is just to allocate space
        polyScalars->InsertTypedTuple(m + n, scalarValue);

        for (svtkIdType i = 0; i < n; i++)
        {
          polyScalars->SetTypedTuple(m + i, scalarValue);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkClipClosedSurface::TriangulateContours(svtkPolyData* data, svtkIdType firstLine,
  svtkIdType numLines, svtkCellArray* outputPolys, const double normal[3])
{
  // If no cut lines were generated, there's nothing to do
  if (numLines <= 0)
  {
    return;
  }

  double nnormal[3] = { -normal[0], -normal[1], -normal[2] };
  int rval =
    svtkContourTriangulator::TriangulateContours(data, firstLine, numLines, outputPolys, nnormal);

  if (rval == 0 && this->TriangulationErrorDisplay)
  {
    svtkErrorMacro("Triangulation failed, data may not be watertight.");
  }
}

// ---------------------------------------------------
int svtkClipClosedSurface::TriangulatePolygon(
  svtkIdList* polygon, svtkPoints* points, svtkCellArray* triangles)
{
  return svtkContourTriangulator::TriangulatePolygon(polygon, points, triangles);
}
