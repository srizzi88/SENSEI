/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEllipseArcSource.cxx

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEllipseArcSource.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMathUtilities.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkEllipseArcSource);

// --------------------------------------------------------------------------
svtkEllipseArcSource::svtkEllipseArcSource()
{
  // Default center is origin
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  // Default normal vector is unit in Oz direction
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->MajorRadiusVector[0] = 1.0;
  this->MajorRadiusVector[1] = 0.0;
  this->MajorRadiusVector[2] = 0.0;

  // Default arc is a quarter-circle
  this->StartAngle = 0.0;
  this->SegmentAngle = 90.;

  // Default resolution
  this->Resolution = 100;

  this->Close = false;

  this->OutputPointsPrecision = SINGLE_PRECISION;

  // Default Ratio
  this->Ratio = 1.0;

  // This is a source
  this->SetNumberOfInputPorts(0);
}

// --------------------------------------------------------------------------
int svtkEllipseArcSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  int numLines = this->Resolution;
  int numPts = this->Resolution + 1;
  double tc[3] = { 0.0, 0.0, 0.0 };

  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  double a = 1.0, b = 1.0;
  double majorRadiusVect[3];
  double orthogonalVect[3];
  double startAngleRad = 0.0, segmentAngleRad, angleIncRad;

  // make sure the normal vector is normalized
  svtkMath::Normalize(this->Normal);

  // get orthonal vector between user-defined major radius and this->Normal
  svtkMath::Cross(this->Normal, this->MajorRadiusVector, orthogonalVect);
  if (svtkMathUtilities::FuzzyCompare(svtkMath::Norm(orthogonalVect), 0.0))
  {
    svtkErrorMacro(<< "Ellipse normal vector and major radius axis are collinear");
    return 0;
  }

  svtkMath::Normalize(orthogonalVect);

  // get major Radius Vector adjusted to be on the plane defined by this->Normal
  svtkMath::Cross(orthogonalVect, this->Normal, majorRadiusVect);

  svtkMath::Normalize(majorRadiusVect);

  // set the major and minor Radius values
  a = svtkMath::Norm(this->MajorRadiusVector);
  b = a * this->Ratio;

  // user-defined angles (positive only)
  startAngleRad = svtkMath::RadiansFromDegrees(this->StartAngle);
  if (startAngleRad < 0)
  {
    startAngleRad += 2.0 * svtkMath::Pi();
  }

  segmentAngleRad = svtkMath::RadiansFromDegrees(this->SegmentAngle);

  // Calcute angle increment
  angleIncRad = segmentAngleRad / this->Resolution;

  // Now create arc points and segments
  svtkNew<svtkPoints> newPoints;

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }

  newPoints->Allocate(numPts);
  svtkNew<svtkFloatArray> newTCoords;
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2 * numPts);
  newTCoords->SetName("Texture Coordinates");
  svtkNew<svtkCellArray> newLines;
  newLines->AllocateEstimate(numLines, 2);

  // Should we skip adding the last point in the loop? Yes if the segment angle is a full
  // 360 degrees and we want to close the loop because the last point will be coincident
  // with the first.
  bool skipLastPoint = this->Close && fabs(this->SegmentAngle - 360.0) < 1e-5;

  double theta = startAngleRad;
  double thetaEllipse;
  // Iterate over angle increments
  for (int i = 0; i <= this->Resolution; ++i, theta += angleIncRad)
  {
    // convert section angle to an angle applied to ellipse equation.
    // the result point with the ellipse angle, will be located on section angle
    int quotient = (int)(theta / (2.0 * svtkMath::Pi()));
    theta = theta - quotient * 2.0 * svtkMath::Pi();

    // result range: -pi/2, pi/2
    thetaEllipse = atan(tan(theta) * this->Ratio);

    // theta range: 0, 2 * pi
    if (theta > svtkMath::Pi() / 2 && theta <= svtkMath::Pi())
    {
      thetaEllipse += svtkMath::Pi();
    }
    else if (theta > svtkMath::Pi() && theta <= 1.5 * svtkMath::Pi())
    {
      thetaEllipse -= svtkMath::Pi();
    }

    const double cosTheta = cos(thetaEllipse);
    const double sinTheta = sin(thetaEllipse);
    double p[3] = { this->Center[0] + a * cosTheta * majorRadiusVect[0] +
        b * sinTheta * orthogonalVect[0],
      this->Center[1] + a * cosTheta * majorRadiusVect[1] + b * sinTheta * orthogonalVect[1],
      this->Center[2] + a * cosTheta * majorRadiusVect[2] + b * sinTheta * orthogonalVect[2] };

    tc[0] = static_cast<double>(i) / this->Resolution;

    // Skip adding a point at the end if it is going to be coincident with the first
    if (i != this->Resolution || !skipLastPoint)
    {
      newPoints->InsertPoint(i, p);
      newTCoords->InsertTuple(i, tc);
    }
  }

  newLines->InsertNextCell(numPts);
  for (int k = 0; k < numPts - 1; ++k)
  {
    newLines->InsertCellPoint(k);
  }

  if (this->Close)
  {
    newLines->InsertCellPoint(0);
  }
  else
  {
    newLines->InsertCellPoint(newPoints->GetNumberOfPoints() - 1);
  }

  output->SetPoints(newPoints);
  output->GetPointData()->SetTCoords(newTCoords);
  output->SetLines(newLines);
  return 1;
}

// --------------------------------------------------------------------------
void svtkEllipseArcSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")\n";

  os << indent << "Major Radius Vector: (" << this->MajorRadiusVector[0] << ", "
     << this->MajorRadiusVector[1] << ", " << this->MajorRadiusVector[2] << ")\n";

  os << indent << "StartAngle: " << this->StartAngle << "\n";
  os << indent << "SegmentAngle: " << this->SegmentAngle << "\n";

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Ratio: " << this->Ratio << "\n";

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
