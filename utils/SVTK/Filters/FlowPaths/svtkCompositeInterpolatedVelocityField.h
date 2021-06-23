/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeInterpolatedVelocityField
 * @brief   An abstract class for
 *  obtaining the interpolated velocity values at a point
 *
 *  svtkCompositeInterpolatedVelocityField acts as a continuous velocity field
 *  by performing cell interpolation on one or more underlying svtkDataSets.
 *
 * @warning
 *  svtkCompositeInterpolatedVelocityField is not thread safe. A new instance
 *  should be created by each thread.
 *
 * @sa
 *  svtkInterpolatedVelocityField svtkCellLocatorInterpolatedVelocityField
 *  svtkGenericInterpolatedVelocityField svtkCachingInterpolatedVelocityField
 *  svtkTemporalInterpolatedVelocityField svtkFunctionSet svtkStreamTracer
 */

#ifndef svtkCompositeInterpolatedVelocityField_h
#define svtkCompositeInterpolatedVelocityField_h

#include "svtkAbstractInterpolatedVelocityField.h"
#include "svtkFiltersFlowPathsModule.h" // For export macro

#include <vector> // STL Header; Required for vector

class svtkDataSet;
class svtkDataArray;
class svtkPointData;
class svtkGenericCell;
class svtkCompositeInterpolatedVelocityFieldDataSetsType;

class SVTKFILTERSFLOWPATHS_EXPORT svtkCompositeInterpolatedVelocityField
  : public svtkAbstractInterpolatedVelocityField
{
public:
  svtkTypeMacro(svtkCompositeInterpolatedVelocityField, svtkAbstractInterpolatedVelocityField);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a dataset for implicit velocity function evaluation. If more than
   * one dataset is added, the evaluation point is searched in all until a
   * match is found. THIS FUNCTION DOES NOT CHANGE THE REFERENCE COUNT OF
   * dataset FOR THREAD SAFETY REASONS.
   */
  virtual void AddDataSet(svtkDataSet* dataset) = 0;

  //@{
  /**
   * Get the most recently visited dataset and its id. The dataset is used
   * for a guess regarding where the next point will be, without searching
   * through all datasets. When setting the last dataset, care is needed as
   * no reference counting or checks are performed. This feature is intended
   * for custom interpolators only that cache datasets independently.
   */
  svtkGetMacro(LastDataSetIndex, int);
  //@}

protected:
  svtkCompositeInterpolatedVelocityField();
  ~svtkCompositeInterpolatedVelocityField() override;

  int LastDataSetIndex;
  svtkCompositeInterpolatedVelocityFieldDataSetsType* DataSets;

private:
  svtkCompositeInterpolatedVelocityField(const svtkCompositeInterpolatedVelocityField&) = delete;
  void operator=(const svtkCompositeInterpolatedVelocityField&) = delete;
};

typedef std::vector<svtkDataSet*> DataSetsTypeBase;
class svtkCompositeInterpolatedVelocityFieldDataSetsType : public DataSetsTypeBase
{
};

#endif
