/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSectorSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

     =========================================================================*/
#include "svtkSectorSource.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLineSource.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRotationalExtrusionFilter.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkSectorSource);

svtkSectorSource::svtkSectorSource()
{
  this->InnerRadius = 1.0;
  this->OuterRadius = 2.0;
  this->ZCoord = 0.0;
  this->StartAngle = 0.0;
  this->EndAngle = 90.0;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;

  this->SetNumberOfInputPorts(0);
}

int svtkSectorSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int piece, numPieces;
  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  //   if( (this->StartAngle == 0. && this->EndAngle == 360.) ||
  //       (this->StartAngle == 360. && this->EndAngle == 0. ) )
  //   {
  //       //use svtkDiskSource
  //     SVTK_CREATE(svtkDiskSource, diskSource );
  //     diskSource->SetCircumferentialResolution( this->CircumferentialResolution );
  //     diskSource->SetRadialResolution( this->RadialResolution );
  //     diskSource->SetInnerRadius( this->InnerRadius );
  //     diskSource->SetOuterRadius( this->OuterRadius );

  //     if (output->GetUpdatePiece() == 0 && numPieces > 0)
  //     {
  //       diskSource->Update();
  //       output->ShallowCopy(diskSource->GetOutput());
  //     }
  //     output->SetUpdatePiece(piece);
  //     output->SetUpdateNumberOfPieces(numPieces);
  //     output->SetUpdateGhostLevel(ghostLevel);
  //   }
  //   else
  //   {
  SVTK_CREATE(svtkLineSource, lineSource);
  lineSource->SetResolution(this->RadialResolution);

  // set vertex 1, adjust for start angle
  // set vertex 2, adjust for start angle
  double x1[3], x2[3];
  x1[0] = this->InnerRadius * cos(svtkMath::RadiansFromDegrees(this->StartAngle));
  x1[1] = this->InnerRadius * sin(svtkMath::RadiansFromDegrees(this->StartAngle));
  x1[2] = this->ZCoord;

  x2[0] = this->OuterRadius * cos(svtkMath::RadiansFromDegrees(this->StartAngle));
  x2[1] = this->OuterRadius * sin(svtkMath::RadiansFromDegrees(this->StartAngle));
  x2[2] = this->ZCoord;

  lineSource->SetPoint1(x1);
  lineSource->SetPoint2(x2);
  lineSource->Update();

  SVTK_CREATE(svtkRotationalExtrusionFilter, rotateFilter);
  rotateFilter->SetResolution(this->CircumferentialResolution);
  rotateFilter->SetInputConnection(lineSource->GetOutputPort());
  rotateFilter->SetAngle(this->EndAngle - this->StartAngle);

  if (piece == 0 && numPieces > 0)
  {
    rotateFilter->Update();
    output->ShallowCopy(rotateFilter->GetOutput());
  }
  //  }

  return 1;
}

void svtkSectorSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "ZCoord: " << this->ZCoord << "\n";
  os << indent << "StartAngle: " << this->StartAngle << "\n";
  os << indent << "EndAngle: " << this->EndAngle << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
}
