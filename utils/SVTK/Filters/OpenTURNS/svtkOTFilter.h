/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOTFilter
 * @brief
 * A generic SVTK Filter to process svtkTable
 * using openturns algorithm.
 * It expects a svtkTable as first input,
 * converts it to a openturns structure and then process it
 * The inherited classes are responsible of filling up the output
 * table in the Process() method.
 */

#ifndef svtkOTFilter_h
#define svtkOTFilter_h

#include "svtkFiltersOpenTURNSModule.h" // For export macro
#include "svtkTableAlgorithm.h"

namespace OT
{
class Sample;
}

class SVTKFILTERSOPENTURNS_EXPORT svtkOTFilter : public svtkTableAlgorithm
{
public:
  svtkTypeMacro(svtkOTFilter, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkOTFilter();
  ~svtkOTFilter() override;

  /**
   * Set the input of this filter, a svtkTable
   */
  virtual int FillInputPortInformation(int port, svtkInformation* info) override;

  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Abstract method to process openturns data
   */
  virtual int Process(OT::Sample* input) = 0;

  /**
   * Method to add a openturns data to a table as a named column
   */
  virtual void AddToOutput(OT::Sample* ns, const std::string& name);

  svtkTable* Output;

private:
  void operator=(const svtkOTFilter&) = delete;
  svtkOTFilter(const svtkOTFilter&) = delete;
};

#endif
