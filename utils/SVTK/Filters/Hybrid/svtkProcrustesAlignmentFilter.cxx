/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcrustesAlignmentFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkProcrustesAlignmentFilter.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLandmarkTransform.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkTransformPolyDataFilter.h"

svtkStandardNewMacro(svtkProcrustesAlignmentFilter);

//----------------------------------------------------------------------------
// protected
svtkProcrustesAlignmentFilter::svtkProcrustesAlignmentFilter()
{
  this->LandmarkTransform = svtkLandmarkTransform::New();
  this->StartFromCentroid = false;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // The precision of the mean points is set in RequestData().
  this->MeanPoints = svtkPoints::New();
}

//----------------------------------------------------------------------------
// protected
svtkProcrustesAlignmentFilter::~svtkProcrustesAlignmentFilter()
{
  if (this->LandmarkTransform)
  {
    this->LandmarkTransform->Delete();
  }
  if (this->MeanPoints)
  {
    this->MeanPoints->Delete();
  }
}

//----------------------------------------------------------------------------
// Calculate the centroid of a point cloud
static inline void Centroid(svtkPoints* pd, double* cp)
{
  // Center point
  cp[0] = 0;
  cp[1] = 0;
  cp[2] = 0;

  int np = pd->GetNumberOfPoints();

  // Calculate center of shape
  for (int i = 0; i < np; i++)
  {
    double p[3];
    pd->GetPoint(i, p);
    cp[0] += p[0];
    cp[1] += p[1];
    cp[2] += p[2];
  }
  cp[0] /= np;
  cp[1] /= np;
  cp[2] /= np;
}

//----------------------------------------------------------------------------
// Calculate the centroid size of a point cloud
static inline double CentroidSize(svtkPoints* pd, double* cp)
{
  Centroid(pd, cp);

  double S = 0;
  for (int i = 0; i < pd->GetNumberOfPoints(); i++)
  {
    double p[3];
    pd->GetPoint(i, p);
    S += svtkMath::Distance2BetweenPoints(p, cp);
  }

  return sqrt(S);
}

//----------------------------------------------------------------------------
// Translation of point cloud. Could be done using transformations
static inline void TranslateShape(svtkPoints* pd, double* tp)
{
  for (int i = 0; i < pd->GetNumberOfPoints(); i++)
  {
    double p[3];
    pd->GetPoint(i, p);
    pd->SetPoint(i, p[0] + tp[0], p[1] + tp[1], p[2] + tp[2]);
  }
}

//----------------------------------------------------------------------------
// Scaling of point cloud. Could be done using transformations
static inline void ScaleShape(svtkPoints* pd, double S)
{
  for (int i = 0; i < pd->GetNumberOfPoints(); i++)
  {
    double p[3];
    pd->GetPoint(i, p);
    pd->SetPoint(i, p[0] * S, p[1] * S, p[2] * S);
  }
}

//----------------------------------------------------------------------------
// Normalise a point cloud to have centroid (0,0,0) and centroid size 1
static inline int NormaliseShape(svtkPoints* pd)
{
  double cp[3];
  double S = CentroidSize(pd, cp);
  if (S == 0)
    return 0;

  double tp[3];
  tp[0] = -cp[0];
  tp[1] = -cp[1];
  tp[2] = -cp[2];
  TranslateShape(pd, tp);
  ScaleShape(pd, 1 / S);
  return 1;
}

//----------------------------------------------------------------------------
// protected
int svtkProcrustesAlignmentFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkMultiBlockDataSet* mbInput =
    svtkMultiBlockDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  const int N_SETS = mbInput->GetNumberOfBlocks();
  if (N_SETS == 0)
  {
    return 1;
  }

  int i, v;
  svtkPointSet* input = nullptr;
  for (i = 0; i < N_SETS; i++)
  {
    input = svtkPointSet::SafeDownCast(mbInput->GetBlock(i));
    if (input)
    {
      break;
    }
  }

  if (!input)
  {
    return 1;
  }

  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Execute()");

  svtkPointSet* tmpInput;

  // copy the inputs across
  // (really actually only the points need to be deep copied since the rest stays the same)
  for (i = 0; i < N_SETS; i++)
  {
    tmpInput = svtkPointSet::SafeDownCast(mbInput->GetBlock(i));
    svtkPointSet* outputBlock = nullptr;
    if (tmpInput)
    {
      outputBlock = tmpInput->NewInstance();
      outputBlock->DeepCopy(tmpInput);

      // Set the desired precision for the points in the output. If
      // this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION
      // then the precision of the points in the output is correctly
      // set during the deep copy of tmpInput.
      if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
      {
        // Only create another new svtkPoints if the output precision is
        // different from the input.
        if (tmpInput->GetPoints() && tmpInput->GetPoints()->GetDataType() != SVTK_FLOAT)
        {
          svtkPoints* newPoints = svtkPoints::New();
          newPoints->SetDataType(SVTK_FLOAT);
          newPoints->DeepCopy(tmpInput->GetPoints());
          outputBlock->SetPoints(newPoints);
          newPoints->Delete();
        }
      }
      else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
      {
        // Only create another new svtkPoints if the output precision is
        // different from the input.
        if (tmpInput->GetPoints() && tmpInput->GetPoints()->GetDataType() != SVTK_DOUBLE)
        {
          svtkPoints* newPoints = svtkPoints::New();
          newPoints->SetDataType(SVTK_DOUBLE);
          newPoints->DeepCopy(tmpInput->GetPoints());
          outputBlock->SetPoints(newPoints);
          newPoints->Delete();
        }
      }
    }
    output->SetBlock(i, outputBlock);
    if (outputBlock)
    {
      outputBlock->Delete();
    }
  }

  // the number of points is determined by the first input (they must all be the same)
  const int N_POINTS = input->GetNumberOfPoints();

  svtkDebugMacro(<< "N_POINTS is " << N_POINTS);

  if (N_POINTS == 0)
  {
    svtkErrorMacro(<< "No points!");
    return 1;
  }

  // all the inputs must have the same number of points to consider executing

  for (i = 1; i < N_SETS; i++)
  {
    tmpInput = svtkPointSet::SafeDownCast(mbInput->GetBlock(i));
    if (!tmpInput)
    {
      continue;
    }
    if (tmpInput->GetNumberOfPoints() != N_POINTS)
    {
      svtkErrorMacro(<< "The inputs have different numbers of points!");
      return 1;
    }
  }

  // Set the desired precision for the mean points.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    // The points in distinct blocks may be of differing precisions.
    this->MeanPoints->SetDataType(SVTK_FLOAT);
    for (i = 0; i < N_SETS; i++)
    {
      tmpInput = svtkPointSet::SafeDownCast(mbInput->GetBlock(i));

      // Set the desired precision to SVTK_DOUBLE if the precision of the
      // mean points in any of the blocks is SVTK_DOUBLE.
      if (tmpInput && tmpInput->GetPoints()->GetDataType() == SVTK_DOUBLE)
      {
        this->MeanPoints->SetDataType(SVTK_DOUBLE);
        break;
      }
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    this->MeanPoints->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    this->MeanPoints->SetDataType(SVTK_DOUBLE);
  }

  this->MeanPoints->DeepCopy(input->GetPoints());
  // our initial estimate of the mean comes from the first example in the set

  // Move to the mutual centroid of the data if requested.
  if (this->GetStartFromCentroid())
  {
    double meanCentroid[3];
    double firstCentroid[3];
    Centroid(MeanPoints, firstCentroid);
    meanCentroid[0] = firstCentroid[0];
    meanCentroid[1] = firstCentroid[1];
    meanCentroid[2] = firstCentroid[2];

    for (i = 1; i < N_SETS; i++)
    {
      tmpInput = svtkPointSet::SafeDownCast(mbInput->GetBlock(i));
      if (!tmpInput)
      {
        continue;
      }
      double localCentroid[3];
      Centroid(tmpInput->GetPoints(), localCentroid);
      meanCentroid[0] += localCentroid[0];
      meanCentroid[1] += localCentroid[1];
      meanCentroid[2] += localCentroid[2];
    }
    meanCentroid[0] /= N_SETS;
    meanCentroid[1] /= N_SETS;
    meanCentroid[2] /= N_SETS;

    double translate[3];
    translate[0] = meanCentroid[0] - firstCentroid[0];
    translate[1] = meanCentroid[1] - firstCentroid[1];
    translate[2] = meanCentroid[2] - firstCentroid[2];

    TranslateShape(MeanPoints, translate);
  }

  // we keep a record of the first mean to fix the orientation and scale
  // (which are otherwise undefined and the loop will not converge)
  svtkPoints* first_mean = svtkPoints::New();
  first_mean->SetDataType(this->MeanPoints->GetDataType());
  first_mean->DeepCopy(MeanPoints);

  // If the similarity transform is used, the mean shape must be normalised
  // to avoid shrinking
  if (this->LandmarkTransform->GetMode() == SVTK_LANDMARK_SIMILARITY)
  {
    if (!NormaliseShape(MeanPoints))
    {
      svtkErrorMacro(<< "Centroid size zero");
      return 1;
    }
    if (!NormaliseShape(first_mean))
    {
      svtkErrorMacro(<< "Centroid size zero");
      return 1;
    }
  }

  // storage for the new mean that is being calculated
  svtkPoints* new_mean = svtkPoints::New();
  new_mean->SetDataType(this->MeanPoints->GetDataType());
  new_mean->SetNumberOfPoints(N_POINTS);

  // compute mean and align all the shapes to it, until convergence
  int converged = 0; // bool converged=false
  int iterations = 0;
  const int MAX_ITERATIONS = 5;
  double difference;
  double point[3], p[3], p2[3];
  double outPoint[3];
  do
  { // (while not converged)

    // align each pointset with the mean
    for (i = 0; i < N_SETS; i++)
    {
      svtkPointSet* block = svtkPointSet::SafeDownCast(output->GetBlock(i));
      if (!block)
      {
        continue;
      }
      this->LandmarkTransform->SetSourceLandmarks(block->GetPoints());
      this->LandmarkTransform->SetTargetLandmarks(this->MeanPoints);
      this->LandmarkTransform->Update();
      for (v = 0; v < N_POINTS; v++)
      {
        this->LandmarkTransform->InternalTransformPoint(block->GetPoint(v), outPoint);
        block->GetPoints()->SetPoint(v, outPoint);
      }
    }

    // compute the new mean (just average the point locations)
    for (v = 0; v < N_POINTS; v++)
    {
      point[0] = 0.0F;
      point[1] = 0.0F;
      point[2] = 0.0F;
      for (i = 0; i < N_SETS; i++)
      {
        svtkPointSet* block = svtkPointSet::SafeDownCast(output->GetBlock(i));
        block->GetPoint(v, p);
        point[0] += p[0];
        point[1] += p[1];
        point[2] += p[2];
      }
      p[0] = point[0] / (double)N_SETS;
      p[1] = point[1] / (double)N_SETS;
      p[2] = point[2] / (double)N_SETS;
      new_mean->SetPoint(v, p);
    }

    // align the new mean with the fixed mean if the transform
    // is similarity or rigidbody. It is not yet decided what to do with affine
    if (this->LandmarkTransform->GetMode() == SVTK_LANDMARK_SIMILARITY ||
      this->LandmarkTransform->GetMode() == SVTK_LANDMARK_RIGIDBODY)
    {
      this->LandmarkTransform->SetSourceLandmarks(new_mean);
      this->LandmarkTransform->SetTargetLandmarks(first_mean);
      this->LandmarkTransform->Update();
      for (v = 0; v < N_POINTS; v++)
      {
        this->LandmarkTransform->InternalTransformPoint(new_mean->GetPoint(v), outPoint);
        new_mean->SetPoint(v, outPoint);
      }
    }

    // If the similarity transform is used, the mean shape must be normalised
    // to avoid shrinking
    if (this->LandmarkTransform->GetMode() == SVTK_LANDMARK_SIMILARITY)
    {
      if (!NormaliseShape(new_mean))
      {
        svtkErrorMacro(<< "Centroid size zero");
        return 1;
      }
    }

    // the new mean becomes our mean
    // compute the difference between the two
    difference = 0.0F;
    for (v = 0; v < N_POINTS; v++)
    {
      new_mean->GetPoint(v, p);
      MeanPoints->GetPoint(v, p2);
      difference += svtkMath::Distance2BetweenPoints(p, p2);
      MeanPoints->SetPoint(v, p);
    }

    // test for convergence
    iterations++;
    svtkDebugMacro(<< "Difference after " << iterations << " iteration(s) is: " << difference);
    if (difference < 1e-6 || iterations >= MAX_ITERATIONS)
    {
      converged = 1; // true
    }

    // The convergence test is that the sum of the distances between the
    // points on mean(t) and mean(t-1) is less than a very small number.
    // Procrustes shouldn't need more than 2 or 3 iterations but things could go wrong
    // so we impose an iteration limit to avoid getting stuck in an infinite loop.

  } while (!converged);

  if (iterations >= MAX_ITERATIONS)
  {
    svtkDebugMacro(<< "Procrustes did not converge in  " << MAX_ITERATIONS
                  << " iterations! Objects may not be aligned. Difference = " << difference);
    // we don't throw an Error here since the shapes most probably *are* aligned, but the
    // numerical precision is worse than our convergence test anticipated.
  }
  else
  {
    svtkDebugMacro(<< "Procrustes required " << iterations << " iterations to converge to "
                  << difference);
  }

  // clean up
  first_mean->Delete();
  new_mean->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// public
void svtkProcrustesAlignmentFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->LandmarkTransform->PrintSelf(os, indent.GetNextIndent());
  this->MeanPoints->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Start From Centroid: " << (this->StartFromCentroid ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
