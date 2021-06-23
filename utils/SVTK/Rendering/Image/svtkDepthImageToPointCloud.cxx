/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDepthImageToPointCloud.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDepthImageToPointCloud.h"

#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkDepthImageToPointCloud);
svtkCxxSetObjectMacro(svtkDepthImageToPointCloud, Camera, svtkCamera);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

// Map input point id to output point id. This map is needed because of the
// optionally capability to cull near and far points.
template <typename T>
void MapPoints(
  svtkIdType numPts, T* depths, bool cullNear, bool cullFar, svtkIdType* map, svtkIdType& numOutPts)
{
  numOutPts = 0;
  float d;
  for (svtkIdType ptId = 0; ptId < numPts; ++depths, ++map, ++ptId)
  {
    d = static_cast<float>(*depths);
    if ((cullNear && d <= 0.0) || (cullFar && d >= 1.0))
    {
      *map = (-1);
    }
    else
    {
      *map = numOutPts++;
    }
  }
}

// This class performs point by point transformation. The view matrix is
// used to transform each pixel. IMPORTANT NOTE: The transformation occurs
// by normalizing the image pixels into the (-1,1) view space (depth values
// are passed through). The process follows the svtkCoordinate class which is
// the standard for SVTK rendering transformations. Subtle differences in
// whether the lower left pixel origin are at the center of the pixel
// versus the lower-left corner of the pixel will make slight differences
// in how pixels are transformed. (Similarly for the upper right pixel as
// well). This half pixel difference can cause transformation issues. Here
// we've played around with the scaling below to produce the best results
// in the current version of SVTK.
template <typename TD, typename TP>
struct MapDepthImage
{
  const TD* Depths;
  TP* Pts;
  const int* Dims;
  const double* Matrix;
  const svtkIdType* PtMap;

  MapDepthImage(TD* depths, TP* pts, int dims[2], double* m, svtkIdType* ptMap)
    : Depths(depths)
    , Pts(pts)
    , Dims(dims)
    , Matrix(m)
    , PtMap(ptMap)
  {
  }

  void operator()(svtkIdType row, svtkIdType end)
  {
    double drow, result[4];
    svtkIdType offset = row * this->Dims[0];
    const TD* dptr = this->Depths + offset;
    const svtkIdType* mptr = this->PtMap + offset;
    TP* pptr;
    for (; row < end; ++row)
    {
      drow = -1.0 + (2.0 * static_cast<double>(row) / static_cast<double>(this->Dims[1] - 1));
      // if pixel origin is pixel center use the two lines below
      // drow = -1.0 + 2.0*((static_cast<double>(row)+0.5) /
      //                    static_cast<double>(this->Dims[1]));
      for (svtkIdType i = 0; i < this->Dims[0]; ++dptr, ++mptr, ++i)
      {
        if (*mptr > (-1)) // if not masked
        {
          pptr = this->Pts + *mptr * 3;
          result[0] = -1.0 + 2.0 * static_cast<double>(i) / static_cast<double>(this->Dims[0] - 1);
          // if pixel origin is pixel center use the two lines below
          // result[0] = -1.0 + 2.0*((static_cast<double>(i)+0.5) /
          //                         static_cast<double>(this->Dims[0]));
          result[1] = drow;
          result[2] = *dptr;
          result[3] = 1.0;
          svtkMatrix4x4::MultiplyPoint(this->Matrix, result, result);
          *pptr++ = result[0] / result[3]; // x
          *pptr++ = result[1] / result[3]; // y
          *pptr = result[2] / result[3];   // z
        }                                  // transform this point
      }
    }
  }
};

// Interface to svtkSMPTools. Threading over image rows. Also perform
// one time calculation/initialization for more efficient processing.
template <typename TD, typename TP>
void XFormPoints(TD* depths, svtkIdType* ptMap, TP* pts, int dims[2], svtkCamera* cam)
{
  double m[16], aspect = static_cast<double>(dims[0]) / static_cast<double>(dims[1]);
  svtkMatrix4x4* matrix = cam->GetCompositeProjectionTransformMatrix(aspect, 0, 1);
  svtkMatrix4x4::Invert(*matrix->Element, m);

  MapDepthImage<TD, TP> mapDepths(depths, pts, dims, m, ptMap);
  svtkSMPTools::For(0, dims[1], mapDepths);
}

// Process the color scalars. It would be pretty easy to process all
// attribute types if this was ever desired.
struct MapScalars
{
  svtkIdType NumColors;
  svtkDataArray* InColors;
  ArrayList Colors;
  const svtkIdType* PtMap;
  svtkDataArray* OutColors;

  MapScalars(svtkIdType num, svtkDataArray* colors, svtkIdType* ptMap)
    : NumColors(num)
    , InColors(colors)
    , PtMap(ptMap)
    , OutColors(nullptr)
  {
    svtkStdString outName = "DepthColors";
    this->OutColors = Colors.AddArrayPair(this->NumColors, this->InColors, outName, 0.0, false);
  }

  void operator()(svtkIdType id, svtkIdType end)
  {
    svtkIdType outId;
    for (; id < end; ++id)
    {
      if ((outId = this->PtMap[id]) > (-1))
      {
        this->Colors.Copy(id, outId);
      }
    }
  }
};

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkDepthImageToPointCloud::svtkDepthImageToPointCloud()
{
  this->Camera = nullptr;
  this->CullNearPoints = false;
  this->CullFarPoints = true;
  this->ProduceColorScalars = true;
  this->ProduceVertexCellArray = true;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkDepthImageToPointCloud::~svtkDepthImageToPointCloud()
{
  if (this->Camera)
  {
    this->Camera->UnRegister(this);
    this->Camera = nullptr;
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkDepthImageToPointCloud::GetMTime()
{
  svtkCamera* cam = this->GetCamera();
  svtkMTimeType t1 = this->MTime.GetMTime();
  svtkMTimeType t2;

  if (!cam)
  {
    return t1;
  }

  // Check the camera
  t2 = cam->GetMTime();
  if (t2 > t1)
  {
    t1 = t2;
  }

  return t1;
}

//----------------------------------------------------------------------------
int svtkDepthImageToPointCloud::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkDepthImageToPointCloud::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDepthImageToPointCloud::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkDepthImageToPointCloud::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  int inExt[6];
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the input extent
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    svtkInformation* in2Info = inputVector[1]->GetInformationObject(0);
    in2Info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkDepthImageToPointCloud::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the input, make sure that it is valid
  int numInputs = 0;
  svtkInformation* info = inputVector[0]->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  if (inData == nullptr)
  {
    svtkErrorMacro("At least one input image is required");
    return 0;
  }
  ++numInputs;

  svtkInformation* info2 = inputVector[1]->GetInformationObject(0);
  svtkImageData* inData2 = nullptr;
  if (info2)
  {
    inData2 = svtkImageData::SafeDownCast(info2->Get(svtkDataObject::DATA_OBJECT()));
    if (inData2 != nullptr)
    {
      ++numInputs;
    }
  }

  svtkCamera* cam = this->Camera;
  if (cam == nullptr)
  {
    svtkErrorMacro("Input camera required");
    return 0;
  }

  // At this point we have at least one input, possibly two. If one input, we
  // assume we either have 1) depth values or 2) color scalars + depth values
  // (if depth values are in an array called "ZBuffer".) If two inputs, then the
  // depth values are in input0 and the color scalars are in input1.
  svtkDataArray* depths = nullptr;
  svtkDataArray* colors = nullptr;
  if (numInputs == 2)
  {
    depths = inData->GetPointData()->GetScalars();
    colors = inData2->GetPointData()->GetScalars();
  }
  else if (numInputs == 1)
  {
    if ((depths = inData->GetPointData()->GetArray("ZBuffer")) != nullptr)
    {
      colors = inData->GetPointData()->GetScalars();
    }
    else
    {
      depths = inData->GetPointData()->GetScalars();
    }
  }
  else
  {
    svtkErrorMacro("Wrong number of inputs");
    return 0;
  }

  // Extract relevant information to generate output
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkPolyData* outData = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Determine the image extents
  const int* ext = inData->GetExtent();
  int dims[2];
  dims[0] = ext[1] - ext[0] + 1;
  dims[1] = ext[3] - ext[2] + 1;
  svtkIdType numPts = dims[0] * dims[1];

  // Estimate the total number of output points. Note that if we are culling
  // near and.or far points, then the number of output points is not known,
  // so a point mask is created.
  svtkIdType numOutPts = 0;
  svtkIdType* ptMap = new svtkIdType[numPts];
  void* depthPtr = depths->GetVoidPointer(0);
  switch (depths->GetDataType())
  {
    svtkTemplateMacro(MapPoints(
      numPts, (SVTK_TT*)depthPtr, this->CullNearPoints, this->CullFarPoints, ptMap, numOutPts));
  }

  // Manage the requested output point precision
  int pointsType = SVTK_DOUBLE;
  if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    pointsType = SVTK_FLOAT;
  }

  // Create the points array which represents the point cloud
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetDataType(pointsType);
  points->SetNumberOfPoints(numOutPts);
  outData->SetPoints(points);

  // Threaded over x-edges (rows). Each depth value is transformed into a
  // world point. Below there is a double allocation based on the depth type
  // and output point type.
  if (pointsType == SVTK_FLOAT)
  {
    float* ptsPtr = static_cast<float*>(points->GetVoidPointer(0));
    switch (depths->GetDataType())
    {
      svtkTemplateMacro(
        XFormPoints((SVTK_TT*)depthPtr, ptMap, static_cast<float*>(ptsPtr), dims, cam));
    }
  }
  else
  {
    double* ptsPtr = static_cast<double*>(points->GetVoidPointer(0));
    switch (depths->GetDataType())
    {
      svtkTemplateMacro(
        XFormPoints((SVTK_TT*)depthPtr, ptMap, static_cast<double*>(ptsPtr), dims, cam));
    }
  }

  // Produce the output colors if requested. Another templated, threaded loop.
  if (colors && this->ProduceColorScalars)
  {
    svtkPointData* outPD = outData->GetPointData();
    MapScalars mapScalars(numOutPts, colors, ptMap);
    svtkSMPTools::For(0, numPts, mapScalars);
    outPD->SetScalars(mapScalars.OutColors);
  }

  // Clean up
  delete[] ptMap;

  // If requested, create an output vertex array
  if (this->ProduceVertexCellArray)
  {
    svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();
    svtkIdType npts = points->GetNumberOfPoints();
    verts->InsertNextCell(npts);
    for (svtkIdType i = 0; i < npts; ++i)
    {
      verts->InsertCellPoint(i);
    }
    outData->SetVerts(verts);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkDepthImageToPointCloud::PrintSelf(ostream& os, svtkIndent indent)
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

  os << indent << "Cull Near Points: " << (this->CullNearPoints ? "On\n" : "Off\n");

  os << indent << "Cull Far Points: " << (this->CullFarPoints ? "On\n" : "Off\n");

  os << indent << "Produce Color Scalars: " << (this->ProduceColorScalars ? "On\n" : "Off\n");

  os << indent
     << "Produce Vertex Cell Array: " << (this->ProduceVertexCellArray ? "On\n" : "Off\n");

  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
}
