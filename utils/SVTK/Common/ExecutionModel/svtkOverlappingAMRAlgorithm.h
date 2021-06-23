/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkOverlappingAMRAlgorithm.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkOverlappingAMRAlgorithm
 *
 *
 *  A base class for all algorithms that take as input svtkOverlappingAMR and
 *  produce svtkOverlappingAMR.
 */

#ifndef svtkOverlappingAMRAlgorithm_h
#define svtkOverlappingAMRAlgorithm_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkUniformGridAMRAlgorithm.h"

class svtkOverlappingAMR;
class svtkInformation;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkOverlappingAMRAlgorithm : public svtkUniformGridAMRAlgorithm
{
public:
  static svtkOverlappingAMRAlgorithm* New();
  svtkTypeMacro(svtkOverlappingAMRAlgorithm, svtkUniformGridAMRAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm
   */
  svtkOverlappingAMR* GetOutput();
  svtkOverlappingAMR* GetOutput(int);
  //@}

protected:
  svtkOverlappingAMRAlgorithm();
  ~svtkOverlappingAMRAlgorithm() override;

  //@{
  /**
   * See algorithm for more info.
   */
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  //@}

private:
  svtkOverlappingAMRAlgorithm(const svtkOverlappingAMRAlgorithm&) = delete;
  void operator=(const svtkOverlappingAMRAlgorithm&) = delete;
};

#endif /* SVTKOVERLAPPINGAMRALGORITHM_H_ */
