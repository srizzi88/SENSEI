/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOutlineCornerFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPOutlineCornerFilter
 * @brief   create wireframe outline corners for arbitrary data set
 *
 * svtkPOutlineCornerFilter works like svtkOutlineCornerFilter,
 * but it looks for data
 * partitions in other processes.  It assumes the filter is operated
 * in a data parallel pipeline.
 */

#ifndef svtkPOutlineCornerFilter_h
#define svtkPOutlineCornerFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"
class svtkOutlineCornerSource;
class svtkMultiProcessController;
class svtkAppendPolyData;
class svtkPOutlineFilterInternals;

class SVTKFILTERSPARALLEL_EXPORT svtkPOutlineCornerFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkPOutlineCornerFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct outline corner filter with default corner factor = 0.2
   */
  static svtkPOutlineCornerFilter* New();

  /**
   * Set/Get the factor that controls the relative size of the corners
   * to the length of the corresponding bounds
   * Typically svtkSetClampMacro(CornerFactor, double, 0.001, 0.5) would
   * used but since we are chaining this to an internal method we rewrite
   * the code in the macro
   */
  virtual void SetCornerFactor(double cornerFactor);
  virtual double GetCornerFactorMinValue() { return 0.001; }
  virtual double GetCornerFactorMaxValue() { return 0.5; }

  svtkGetMacro(CornerFactor, double);

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPOutlineCornerFilter();
  ~svtkPOutlineCornerFilter() override;

  svtkMultiProcessController* Controller;
  svtkOutlineCornerSource* OutlineCornerSource;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  double CornerFactor;

private:
  svtkPOutlineCornerFilter(const svtkPOutlineCornerFilter&) = delete;
  void operator=(const svtkPOutlineCornerFilter&) = delete;

  svtkPOutlineFilterInternals* Internals;
};

#endif
