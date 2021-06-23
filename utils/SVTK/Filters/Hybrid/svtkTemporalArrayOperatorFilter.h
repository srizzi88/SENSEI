/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalArrayOperatorFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalArrayOperatorFilter
 * @brief   perform simple mathematical operation on a data array at different time
 *
 * This filter computes a simple operation between two time steps of one
 * data array.
 *
 * @sa
 * svtkArrayCalulator
 */

#ifndef svtkTemporalArrayOperatorFilter_h
#define svtkTemporalArrayOperatorFilter_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkMultiTimeStepAlgorithm.h"

class SVTKFILTERSHYBRID_EXPORT svtkTemporalArrayOperatorFilter : public svtkMultiTimeStepAlgorithm
{
public:
  static svtkTemporalArrayOperatorFilter* New();
  svtkTypeMacro(svtkTemporalArrayOperatorFilter, svtkMultiTimeStepAlgorithm);
  void PrintSelf(ostream& OS, svtkIndent indent) override;

  enum OperatorType
  {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    DIV = 3
  };

  //@{
  /**
   * @brief Set/Get the operator to apply. Default is ADD (0).
   */
  svtkSetMacro(Operator, int);
  svtkGetMacro(Operator, int);
  //@}

  //@{
  /**
   * @brief Set/Get the first time step.
   */
  svtkSetMacro(FirstTimeStepIndex, int);
  svtkGetMacro(FirstTimeStepIndex, int);
  //@}

  //@{
  /**
   * @brief Set/Get the second time step.
   */
  svtkSetMacro(SecondTimeStepIndex, int);
  svtkGetMacro(SecondTimeStepIndex, int);
  //@}

  //@{
  /**
   * @brief Set/Get the suffix to be append to the output array name.
   * If not specified, output will be suffixed with '_' and the operation
   * type (eg. myarrayname_add).
   */
  svtkSetStringMacro(OutputArrayNameSuffix);
  svtkGetStringMacro(OutputArrayNameSuffix);
  //@}

protected:
  svtkTemporalArrayOperatorFilter();
  ~svtkTemporalArrayOperatorFilter() override;

  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int GetInputArrayAssociation();
  virtual svtkDataObject* Process(svtkDataObject*, svtkDataObject*);
  virtual svtkDataObject* ProcessDataObject(svtkDataObject*, svtkDataObject*);
  virtual svtkDataArray* ProcessDataArray(svtkDataArray*, svtkDataArray*);

  int Operator;
  int FirstTimeStepIndex;
  int SecondTimeStepIndex;
  int NumberTimeSteps;
  char* OutputArrayNameSuffix;

private:
  svtkTemporalArrayOperatorFilter(const svtkTemporalArrayOperatorFilter&) = delete;
  void operator=(const svtkTemporalArrayOperatorFilter&) = delete;
};

#endif
