/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkNonOverlappingAMRAlgorithm.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkNonOverlappingAMRAlgorithm
 *  produce svtkNonOverlappingAMR as output.
 *
 *
 *
 */

#ifndef svtkNonOverlappingAMRAlgorithm_h
#define svtkNonOverlappingAMRAlgorithm_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkUniformGridAMRAlgorithm.h"

class svtkNonOverlappingAMR;
class svtkInformation;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkNonOverlappingAMRAlgorithm
  : public svtkUniformGridAMRAlgorithm
{
public:
  static svtkNonOverlappingAMRAlgorithm* New();
  svtkTypeMacro(svtkNonOverlappingAMRAlgorithm, svtkUniformGridAMRAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm
   */
  svtkNonOverlappingAMR* GetOutput();
  svtkNonOverlappingAMR* GetOutput(int);
  //@}

protected:
  svtkNonOverlappingAMRAlgorithm();
  ~svtkNonOverlappingAMRAlgorithm() override;

  //@{
  /**
   * See algorithm for more info.
   */
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  //@}

private:
  svtkNonOverlappingAMRAlgorithm(const svtkNonOverlappingAMRAlgorithm&) = delete;
  void operator=(const svtkNonOverlappingAMRAlgorithm&) = delete;
};

#endif /* SVTKNONOVERLAPPINGAMRALGORITHM_H_ */
