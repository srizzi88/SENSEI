/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSynchronizedTemplatesCutter3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSynchronizedTemplatesCutter3D.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdListCollection.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolygonBuilder.h"
#include "svtkShortArray.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredPoints.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedShortArray.h"

#include <cmath>

svtkStandardNewMacro(svtkSynchronizedTemplatesCutter3D);
svtkCxxSetObjectMacro(svtkSynchronizedTemplatesCutter3D, CutFunction, svtkImplicitFunction);

//----------------------------------------------------------------------------
// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
svtkSynchronizedTemplatesCutter3D::svtkSynchronizedTemplatesCutter3D()
{
  this->CutFunction = nullptr;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
svtkSynchronizedTemplatesCutter3D::~svtkSynchronizedTemplatesCutter3D()
{
  this->SetCutFunction(nullptr);
}

//----------------------------------------------------------------------------
static void svtkSynchronizedTemplatesCutter3DInitializeOutput(
  int* ext, int precision, svtkImageData* input, svtkPolyData* o)
{
  svtkPoints* newPts;
  svtkCellArray* newPolys;
  long estimatedSize;

  estimatedSize =
    (int)pow((double)((ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1)), .75);
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  newPts = svtkPoints::New();

  // set precision for the points in the output
  if (precision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      newPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPts->SetDataType(SVTK_FLOAT);
    }
  }
  else if (precision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (precision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(estimatedSize, estimatedSize);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(estimatedSize, 3);

  o->GetPointData()->CopyAllOn();

  o->GetPointData()->InterpolateAllocate(input->GetPointData(), estimatedSize, estimatedSize / 2);
  o->GetCellData()->CopyAllocate(input->GetCellData(), estimatedSize, estimatedSize / 2);

  o->SetPoints(newPts);
  newPts->Delete();

  o->SetPolys(newPolys);
  newPolys->Delete();
}

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images
//
template <class T>
void ContourImage(svtkSynchronizedTemplatesCutter3D* self, int* exExt, svtkImageData* data,
  svtkPolyData* output, T* ptr, bool outputTriangles)
{
  int* inExt = data->GetExtent();
  svtkIdType xdim = exExt[1] - exExt[0] + 1;
  svtkIdType ydim = exExt[3] - exExt[2] + 1;
  double* values = self->GetValues();
  svtkIdType numContours = self->GetNumberOfContours();
  T *inPtrX, *inPtrY, *inPtrZ;
  T *s0, *s1, *s2, *s3;
  int xMin, xMax, yMin, yMax, zMin, zMax;
  svtkIdType xInc, yInc, zInc;
  int xIncFunc, yIncFunc, zIncFunc, scalarZIncFunc;
  double* origin = data->GetOrigin();
  double* spacing = data->GetSpacing();
  svtkIdType *isect1Ptr, *isect2Ptr;
  double y, z, t;
  int i, j, k;
  svtkIdType zstep, yisectstep;
  svtkIdType offsets[12];
  int* tablePtr;
  svtkIdType idx;
  int vidx;
  double x[3], xz[3];
  svtkIdType v0, v1, v2, v3;
  svtkIdType ptIds[3];
  double value;
  // We need to know the edgePointId's for interpolating attributes.
  svtkIdType edgePtId, inCellId, outCellId;
  svtkPointData* inPD = data->GetPointData();
  svtkCellData* inCD = data->GetCellData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD = output->GetCellData();
  // Use to be arguments
  svtkPoints* newPts;
  svtkCellArray* newPolys;
  ptr += self->GetArrayComponent();
  svtkPolygonBuilder polyBuilder;
  svtkSmartPointer<svtkIdListCollection> polys = svtkSmartPointer<svtkIdListCollection>::New();

  svtkSynchronizedTemplatesCutter3DInitializeOutput(
    exExt, self->GetOutputPointsPrecision(), data, output);
  newPts = output->GetPoints();
  newPolys = output->GetPolys();

  xMin = exExt[0];
  xMax = exExt[1];
  yMin = exExt[2];
  yMax = exExt[3];
  zMin = exExt[4];
  zMax = exExt[5];

  svtkImplicitFunction* func = self->GetCutFunction();
  if (!func)
  {
    return;
  }

  xInc = 1;
  yInc = xInc * (inExt[1] - inExt[0] + 1);
  zInc = yInc * (inExt[3] - inExt[2] + 1);

  // Note that the implicit functions are specified
  // over exExt so we need to compute the steps differently
  xIncFunc = 1;
  yIncFunc = xIncFunc * xdim;
  zIncFunc = yIncFunc * ydim;
  scalarZIncFunc = zIncFunc;

  // Kens increments, probably to do with edge array
  zstep = xdim * ydim;
  yisectstep = xdim * 3;
  // compute offsets probably how to get to the edges in the edge array.
  offsets[0] = -xdim * 3;
  offsets[1] = -xdim * 3 + 1;
  offsets[2] = -xdim * 3 + 2;
  offsets[3] = -xdim * 3 + 4;
  offsets[4] = -xdim * 3 + 5;
  offsets[5] = 0;
  offsets[6] = 2;
  offsets[7] = 5;
  offsets[8] = (zstep - xdim) * 3;
  offsets[9] = (zstep - xdim) * 3 + 1;
  offsets[10] = (zstep - xdim) * 3 + 4;
  offsets[11] = zstep * 3;

  // allocate storage array
  svtkIdType* isect1 = new svtkIdType[xdim * ydim * 3 * 2];
  // set impossible edges to -1
  for (i = 0; i < ydim; i++)
  {
    isect1[(i + 1) * xdim * 3 - 3] = -1;
    isect1[(i + 1) * xdim * 3 * 2 - 3] = -1;
  }
  for (i = 0; i < xdim; i++)
  {
    isect1[((ydim - 1) * xdim + i) * 3 + 1] = -1;
    isect1[((ydim - 1) * xdim + i) * 3 * 2 + 1] = -1;
  }

  // allocate scalar storage for two slices
  T* scalars = new T[xdim * ydim * 2];
  T* scalars1 = scalars;
  T* scalars2 = scalars + xdim * ydim;

  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
  {
    value = values[vidx];
    inPtrZ = ptr;

    // fill the first slice
    z = origin[2] + spacing[2] * zMin;
    x[2] = z;
    T* scalarsTmp = scalars1;
    scalars1 = scalars2;
    scalars2 = scalarsTmp;
    for (j = yMin; j <= yMax; j++)
    {
      x[1] = origin[1] + spacing[1] * j;
      for (i = xMin; i <= xMax; i++)
      {
        x[0] = origin[0] + spacing[0] * i;
        *scalarsTmp = func->FunctionValue(x);
        scalarsTmp++;
      }
    }
    scalarZIncFunc = -scalarZIncFunc;

    //==================================================================
    for (k = zMin; k <= zMax; k++)
    {
      self->UpdateProgress(
        (double)vidx / numContours + (k - zMin) / ((zMax - zMin + 1.0) * numContours));
      inPtrY = inPtrZ;

      // for each slice compute the scalars
      z = origin[2] + spacing[2] * (k + 1);
      x[2] = z;
      scalarsTmp = scalars1;
      scalars1 = scalars2;
      scalars2 = scalarsTmp;
      // if not the last slice then get more scalars
      if (k < zMax)
      {
        for (j = yMin; j <= yMax; j++)
        {
          x[1] = origin[1] + spacing[1] * j;
          for (i = xMin; i <= xMax; i++)
          {
            x[0] = origin[0] + spacing[0] * i;
            *scalarsTmp = func->FunctionValue(x);
            scalarsTmp++;
          }
        }
      }
      inPtrY = scalars1;
      scalarZIncFunc = -scalarZIncFunc;

      z = origin[2] + spacing[2] * k;
      x[2] = z;

      // swap the buffers
      if (k % 2)
      {
        offsets[8] = (zstep - xdim) * 3;
        offsets[9] = (zstep - xdim) * 3 + 1;
        offsets[10] = (zstep - xdim) * 3 + 4;
        offsets[11] = zstep * 3;
        isect1Ptr = isect1;
        isect2Ptr = isect1 + xdim * ydim * 3;
      }
      else
      {
        offsets[8] = (-zstep - xdim) * 3;
        offsets[9] = (-zstep - xdim) * 3 + 1;
        offsets[10] = (-zstep - xdim) * 3 + 4;
        offsets[11] = -zstep * 3;
        isect1Ptr = isect1 + xdim * ydim * 3;
        isect2Ptr = isect1;
      }

      for (j = yMin; j <= yMax; j++)
      {
        // Should not impact performance here/
        edgePtId = (xMin - inExt[0]) * xInc + (j - inExt[2]) * yInc + (k - inExt[4]) * zInc;

        // Increments are different for cells.  Since the cells are not
        // contoured until the second row of templates, subtract 1 from
        // i,j,and k.  Note: first cube is formed when i=0, j=1, and k=1.
        inCellId = (xMin - inExt[0]) +
          (inExt[1] - inExt[0]) * ((j - inExt[2] - 1) + (k - inExt[4] - 1) * (inExt[3] - inExt[2]));

        y = origin[1] + j * spacing[1];
        xz[1] = y;

        s1 = inPtrY;
        v1 = (*s1 < value ? 0 : 1);

        inPtrX = inPtrY;
        for (i = xMin; i <= xMax; i++)
        {
          s0 = s1;
          v0 = v1;
          *isect2Ptr = -1;
          *(isect2Ptr + 1) = -1;
          *(isect2Ptr + 2) = -1;
          if (i < xMax)
          {
            s1 = (inPtrX + xIncFunc);
            v1 = (*s1 < value ? 0 : 1);
            if (v0 ^ v1)
            {
              // watch for degenerate points
              if (*s0 == value)
              {
                if (i > xMin && *(isect2Ptr - 3) > -1)
                {
                  *isect2Ptr = *(isect2Ptr - 3);
                }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 1);
                }
                else if (k > zMin && *(isect1Ptr + 2) > -1)
                {
                  *isect2Ptr = *(isect1Ptr + 2);
                }
              }
              else if (*s1 == value)
              {
                if (j > yMin && *(isect2Ptr - yisectstep + 4) > -1)
                {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 4);
                }
                else if (k > zMin && i<xMax&&*(isect1Ptr + 5)> - 1)
                {
                  *isect2Ptr = *(isect1Ptr + 5);
                }
              }
              // if the edge has not been set yet then it is a new point
              if (*isect2Ptr == -1)
              {
                t = (value - (double)(*s0)) / ((double)(*s1) - (double)(*s0));
                x[0] = origin[0] + spacing[0] * (i + t);
                x[1] = y;
                *isect2Ptr = newPts->InsertNextPoint(x);
                outPD->InterpolateEdge(inPD, *isect2Ptr, edgePtId, edgePtId + 1, t);
              }
            }
          }
          if (j < yMax)
          {
            s2 = (inPtrX + yIncFunc);
            v2 = (*s2 < value ? 0 : 1);
            if (v0 ^ v2)
            {
              if (*s0 == value)
              {
                if (*isect2Ptr > -1)
                {
                  *(isect2Ptr + 1) = *isect2Ptr;
                }
                else if (i > xMin && *(isect2Ptr - 3) > -1)
                {
                  *(isect2Ptr + 1) = *(isect2Ptr - 3);
                }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                {
                  *(isect2Ptr + 1) = *(isect2Ptr - yisectstep + 1);
                }
                else if (k > zMin && *(isect1Ptr + 2) > -1)
                {
                  *(isect2Ptr + 1) = *(isect1Ptr + 2);
                }
              }
              else if (*s2 == value && k > zMin && *(isect1Ptr + yisectstep + 2) > -1)
              {
                *(isect2Ptr + 1) = *(isect1Ptr + yisectstep + 2);
              }
              // if the edge has not been set yet then it is a new point
              if (*(isect2Ptr + 1) == -1)
              {
                t = (value - (double)(*s0)) / ((double)(*s2) - (double)(*s0));
                x[0] = origin[0] + spacing[0] * i;
                x[1] = y + spacing[1] * t;
                *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
                outPD->InterpolateEdge(inPD, *(isect2Ptr + 1), edgePtId, edgePtId + yInc, t);
              }
            }
          }
          if (k < zMax)
          {
            s3 = (inPtrX + scalarZIncFunc);
            v3 = (*s3 < value ? 0 : 1);
            if (v0 ^ v3)
            {
              if (*s0 == value)
              {
                if (*isect2Ptr > -1)
                {
                  *(isect2Ptr + 2) = *isect2Ptr;
                }
                else if (*(isect2Ptr + 1) > -1)
                {
                  *(isect2Ptr + 2) = *(isect2Ptr + 1);
                }
                else if (i > xMin && *(isect2Ptr - 3) > -1)
                {
                  *(isect2Ptr + 2) = *(isect2Ptr - 3);
                }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                {
                  *(isect2Ptr + 2) = *(isect2Ptr - yisectstep + 1);
                }
                else if (k > zMin && *(isect1Ptr + 2) > -1)
                {
                  *(isect2Ptr + 2) = *(isect1Ptr + 2);
                }
              }
              if (*(isect2Ptr + 2) == -1)
              {
                t = (value - (double)(*s0)) / ((double)(*s3) - (double)(*s0));
                xz[0] = origin[0] + spacing[0] * i;
                xz[2] = z + spacing[2] * t;
                *(isect2Ptr + 2) = newPts->InsertNextPoint(xz);
                outPD->InterpolateEdge(inPD, *(isect2Ptr + 2), edgePtId, edgePtId + zInc, t);
              }
            }
          }
          // To keep track of ids for interpolating attributes.
          ++edgePtId;

          // now add any polys that need to be added
          // basically look at the isect values,
          // form an index and lookup the polys
          if (j > yMin && i < xMax && k > zMin)
          {
            idx = (v0 ? 4096 : 0);
            idx = idx + (*(isect1Ptr - yisectstep) > -1 ? 2048 : 0);
            idx = idx + (*(isect1Ptr - yisectstep + 1) > -1 ? 1024 : 0);
            idx = idx + (*(isect1Ptr - yisectstep + 2) > -1 ? 512 : 0);
            idx = idx + (*(isect1Ptr - yisectstep + 4) > -1 ? 256 : 0);
            idx = idx + (*(isect1Ptr - yisectstep + 5) > -1 ? 128 : 0);
            idx = idx + (*(isect1Ptr) > -1 ? 64 : 0);
            idx = idx + (*(isect1Ptr + 2) > -1 ? 32 : 0);
            idx = idx + (*(isect1Ptr + 5) > -1 ? 16 : 0);
            idx = idx + (*(isect2Ptr - yisectstep) > -1 ? 8 : 0);
            idx = idx + (*(isect2Ptr - yisectstep + 1) > -1 ? 4 : 0);
            idx = idx + (*(isect2Ptr - yisectstep + 4) > -1 ? 2 : 0);
            idx = idx + (*(isect2Ptr) > -1 ? 1 : 0);

            tablePtr =
              SVTK_SYNCHRONIZED_TEMPLATES_3D_TABLE_2 + SVTK_SYNCHRONIZED_TEMPLATES_3D_TABLE_1[idx];

            if (!outputTriangles)
            {
              polyBuilder.Reset();
            }
            while (*tablePtr != -1)
            {
              ptIds[0] = *(isect1Ptr + offsets[*tablePtr]);
              tablePtr++;
              ptIds[1] = *(isect1Ptr + offsets[*tablePtr]);
              tablePtr++;
              ptIds[2] = *(isect1Ptr + offsets[*tablePtr]);
              tablePtr++;
              if (ptIds[0] != ptIds[1] && ptIds[0] != ptIds[2] && ptIds[1] != ptIds[2])
              {
                if (outputTriangles)
                {
                  outCellId = newPolys->InsertNextCell(3, ptIds);
                  outCD->CopyData(inCD, inCellId, outCellId);
                }
                else
                {
                  polyBuilder.InsertTriangle(ptIds);
                }
              }
            }
            if (!outputTriangles)
            {
              polyBuilder.GetPolygons(polys);
              int nPolys = polys->GetNumberOfItems();
              for (int polyId = 0; polyId < nPolys; ++polyId)
              {
                svtkIdList* poly = polys->GetItem(polyId);
                if (poly->GetNumberOfIds() != 0)
                {
                  outCellId = newPolys->InsertNextCell(poly);
                  outCD->CopyData(inCD, inCellId, outCellId);
                }
                poly->Delete();
              }
              polys->RemoveAllItems();
            }
          }
          inPtrX += xIncFunc;
          isect2Ptr += 3;
          isect1Ptr += 3;
          // To keep track of ids for copying cell attributes..
          ++inCellId;
        }
        inPtrY += yIncFunc;
      }
      inPtrZ += zIncFunc;
    }
  }
  delete[] isect1;

  delete[] scalars;
}

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images (or slices from images)
//
void svtkSynchronizedTemplatesCutter3D::ThreadedExecute(
  svtkImageData* data, svtkInformation* outInfo, int)
{
  svtkPolyData* output;

  svtkDebugMacro(<< "Executing Cutter3D structured contour");

  output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int* exExt = data->GetExtent();
  if (exExt[0] >= exExt[1] || exExt[2] >= exExt[3] || exExt[4] >= exExt[5])
  {
    svtkDebugMacro(<< "Cutter3D structured contours requires Cutter3D data");
    return;
  }

  // Check data type and execute appropriate function
  ContourImage(this, exExt, data, output, (double*)nullptr, this->GenerateTriangles != 0);
}

//----------------------------------------------------------------------------
int svtkSynchronizedTemplatesCutter3D::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Just call the threaded execute directly.
  this->ThreadedExecute(input, outInfo, 0);

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkSynchronizedTemplatesCutter3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Cut Function: " << this->CutFunction << "\n";
  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
}
