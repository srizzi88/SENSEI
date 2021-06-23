/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkComputeQuartiles.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkComputeQuartiles
 * @brief   Extract quartiles and extremum values
 * of all columns of a table or all fields of a dataset.
 *
 *
 * svtkComputeQuartiles accepts any svtkDataObject as input and produces a
 * svtkTable data as output.
 * This filter can be used to generate a table to create box plots
 * using a svtkPlotBox instance.
 * The filter internally uses svtkOrderStatistics to compute quartiles.
 *
 * @sa
 * svtkTableAlgorithm svtkOrderStatistics svtkPlotBox svtkChartBox
 *
 * @par Thanks:
 * This class was written by Kitware SAS and supported by EDF - www.edf.fr
 */

#ifndef svtkComputeQuartiles_h
#define svtkComputeQuartiles_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class svtkDataSet;
class svtkDoubleArray;
class svtkFieldData;
class svtkTable;

class SVTKFILTERSSTATISTICS_EXPORT svtkComputeQuartiles : public svtkTableAlgorithm
{
public:
  static svtkComputeQuartiles* New();
  svtkTypeMacro(svtkComputeQuartiles, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkComputeQuartiles();
  ~svtkComputeQuartiles() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  void ComputeTable(svtkDataObject*, svtkTable*, svtkIdType);

  int FieldAssociation;

private:
  void operator=(const svtkComputeQuartiles&) = delete;
  svtkComputeQuartiles(const svtkComputeQuartiles&) = delete;

  int GetInputFieldAssociation();
  svtkFieldData* GetInputFieldData(svtkDataObject* input);
};

#endif
