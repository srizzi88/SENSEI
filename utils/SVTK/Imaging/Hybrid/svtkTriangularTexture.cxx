/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTriangularTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTriangularTexture.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkTriangularTexture);

// Instantiate object with XSize and YSize = 64; the texture pattern =1
// (opaque at centroid); and the scale factor set to 1.0.

svtkTriangularTexture::svtkTriangularTexture()
{
  this->XSize = this->YSize = 64;
  this->TexturePattern = 1;
  this->ScaleFactor = 1.0;
  this->SetNumberOfInputPorts(0);
}

static void svtkOpaqueAtElementCentroid(
  int XSize, int YSize, double ScaleFactor, svtkUnsignedCharArray* newScalars)
{
  int i, j;
  double opacity;
  double point[3];
  double XScale = XSize + 1.0;
  double YScale = YSize + 1.0;
  unsigned char AGrayValue[2];
  double dist, distToV2, distToV3;
  double v1[3] = { 0.0, 0.0, 0.0 };
  double v2[3] = { 1.0, 0.0, 0.0 };
  double v3[3] = { 0.5, sqrt(3.0) / 2.0, 0.0 };

  point[2] = 0.0;
  AGrayValue[0] = AGrayValue[1] = 255;

  for (j = 0; j < YSize; j++)
  {
    for (i = 0; i < XSize; i++)
    {
      point[0] = i / XScale;
      point[1] = j / YScale;
      dist = svtkMath::Distance2BetweenPoints(point, v1);
      distToV2 = svtkMath::Distance2BetweenPoints(point, v2);
      if (distToV2 < dist)
      {
        dist = distToV2;
      }
      distToV3 = svtkMath::Distance2BetweenPoints(point, v3);
      if (distToV3 < dist)
      {
        dist = distToV3;
      }

      opacity = sqrt(dist) * ScaleFactor;
      if (opacity < .5)
      {
        opacity = 0.0;
      }
      if (opacity > .5)
      {
        opacity = 1.0;
      }
      AGrayValue[1] = static_cast<unsigned char>(opacity * 255);
      newScalars->SetValue((XSize * j + i) * 2, AGrayValue[0]);
      newScalars->SetValue((XSize * j + i) * 2 + 1, AGrayValue[1]);
    }
  }
}

static void svtkOpaqueAtVertices(
  int XSize, int YSize, double ScaleFactor, svtkUnsignedCharArray* newScalars)
{
  int i, j;
  double opacity;
  double point[3];
  double XScale = XSize + 1.0;
  double YScale = YSize + 1.0;
  unsigned char AGrayValue[2];
  double dist, distToV2, distToV3;
  double v1[3] = { 0.0, 0.0, 0.0 };
  double v2[3] = { 1.0, 0.0, 0.0 };
  double v3[3] = { 0.5, sqrt(3.0) / 2.0, 0.0 };

  point[2] = 0.0;
  AGrayValue[0] = AGrayValue[1] = 255;

  for (j = 0; j < YSize; j++)
  {
    for (i = 0; i < XSize; i++)
    {
      point[0] = i / XScale;
      point[1] = j / YScale;
      dist = svtkMath::Distance2BetweenPoints(point, v1);
      distToV2 = svtkMath::Distance2BetweenPoints(point, v2);
      if (distToV2 < dist)
      {
        dist = distToV2;
      }
      distToV3 = svtkMath::Distance2BetweenPoints(point, v3);
      if (distToV3 < dist)
      {
        dist = distToV3;
      }

      opacity = sqrt(dist) * ScaleFactor;
      if (opacity < .5)
      {
        opacity = 0.0;
      }
      if (opacity > .5)
      {
        opacity = 1.0;
      }
      opacity = 1.0 - opacity;
      AGrayValue[1] = static_cast<unsigned char>(opacity * 255);
      newScalars->SetValue((XSize * j + i) * 2, AGrayValue[0]);
      newScalars->SetValue((XSize * j + i) * 2 + 1, AGrayValue[1]);
    }
  }
}

//----------------------------------------------------------------------------
int svtkTriangularTexture::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int wExt[6] = { 0, this->XSize - 1, 0, this->YSize - 1, 0, 0 };

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_UNSIGNED_CHAR, 2);
  return 1;
}

void svtkTriangularTexture::ExecuteDataWithInformation(svtkDataObject* outp, svtkInformation* outInfo)
{
  svtkImageData* output = this->AllocateOutputData(outp, outInfo);
  svtkUnsignedCharArray* newScalars =
    svtkArrayDownCast<svtkUnsignedCharArray>(output->GetPointData()->GetScalars());

  if (this->XSize * this->YSize < 1)
  {
    svtkErrorMacro(<< "Bad texture (xsize,ysize) specification!");
    return;
  }

  switch (this->TexturePattern)
  {
    case 1: // opaque at element vertices
      svtkOpaqueAtVertices(this->XSize, this->YSize, this->ScaleFactor, newScalars);
      break;

    case 2: // opaque at element centroid
      svtkOpaqueAtElementCentroid(this->XSize, this->YSize, this->ScaleFactor, newScalars);
      break;

    case 3: // opaque in rings around vertices
      svtkErrorMacro(<< "Opaque vertex rings not implemented");
      break;
  }
}

void svtkTriangularTexture::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XSize:" << this->XSize << "\n";
  os << indent << "YSize:" << this->YSize << "\n";

  os << indent << "Texture Pattern:" << this->TexturePattern << "\n";

  os << indent << "Scale Factor:" << this->ScaleFactor << "\n";
}
