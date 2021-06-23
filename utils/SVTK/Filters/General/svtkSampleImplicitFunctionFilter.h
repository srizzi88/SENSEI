/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSampleImplicitFunctionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSampleImplicitFunctionFilter
 * @brief   sample an implicit function over a dataset,
 * generating scalar values and optional gradient vectors
 *
 *
 * svtkSampleImplicitFunctionFilter is a filter that evaluates an implicit function and
 * (optional) gradients at each point in an input svtkDataSet. The output
 * of the filter are new scalar values (the function values) and the
 * optional vector (function gradient) array.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkSampleFunction svtkImplicitModeller
 */

#ifndef svtkSampleImplicitFunctionFilter_h
#define svtkSampleImplicitFunctionFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkImplicitFunction;
class svtkDataArray;

class SVTKFILTERSGENERAL_EXPORT svtkSampleImplicitFunctionFilter : public svtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard instantiation, type information, and print methods.
   */
  static svtkSampleImplicitFunctionFilter* New();
  svtkTypeMacro(svtkSampleImplicitFunctionFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the implicit function to use to generate data.
   */
  virtual void SetImplicitFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(ImplicitFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Turn on/off the computation of gradients.
   */
  svtkSetMacro(ComputeGradients, svtkTypeBool);
  svtkGetMacro(ComputeGradients, svtkTypeBool);
  svtkBooleanMacro(ComputeGradients, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the scalar array name for this data set. The initial value is
   * "Implicit scalars".
   */
  svtkSetStringMacro(ScalarArrayName);
  svtkGetStringMacro(ScalarArrayName);
  //@}

  //@{
  /**
   * Set/get the gradient array name for this data set. The initial value is
   * "Implicit gradients".
   */
  svtkSetStringMacro(GradientArrayName);
  svtkGetStringMacro(GradientArrayName);
  //@}

  /**
   * Return the MTime also taking into account the implicit function.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkSampleImplicitFunctionFilter();
  ~svtkSampleImplicitFunctionFilter() override;

  svtkImplicitFunction* ImplicitFunction;
  svtkTypeBool ComputeGradients;
  char* ScalarArrayName;
  char* GradientArrayName;

  void ReportReferences(svtkGarbageCollector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkSampleImplicitFunctionFilter(const svtkSampleImplicitFunctionFilter&) = delete;
  void operator=(const svtkSampleImplicitFunctionFilter&) = delete;
};

#endif
