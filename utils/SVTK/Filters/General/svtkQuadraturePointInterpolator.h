/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQuadraturePointInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQuadraturePointInterpolator
 *
 *
 * Interpolates each scalar/vector field in a svtkUnstrcturedGrid
 * on its input to a specific set of quadrature points. The
 * set of quadrature points is specified per array via a
 * dictionary (ie an instance of svtkInformationQuadratureSchemeDefinitionVectorKey).
 * contained in the array. The interpolated fields are placed
 * in FieldData along with a set of per cell indexes, that allow
 * random access to a given cells quadrature points.
 *
 * @sa
 * svtkQuadratureSchemeDefinition, svtkQuadraturePointsGenerator,
 * svtkInformationQuadratureSchemeDefinitionVectorKey
 */

#ifndef svtkQuadraturePointInterpolator_h
#define svtkQuadraturePointInterpolator_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkUnstructuredGrid;
class svtkInformation;
class svtkInformationVector;

class SVTKFILTERSGENERAL_EXPORT svtkQuadraturePointInterpolator : public svtkDataSetAlgorithm
{
public:
  static svtkQuadraturePointInterpolator* New();
  svtkTypeMacro(svtkQuadraturePointInterpolator, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int RequestData(
    svtkInformation* req, svtkInformationVector** input, svtkInformationVector* output) override;
  svtkQuadraturePointInterpolator();
  ~svtkQuadraturePointInterpolator() override;

private:
  svtkQuadraturePointInterpolator(const svtkQuadraturePointInterpolator&) = delete;
  void operator=(const svtkQuadraturePointInterpolator&) = delete;
  //
  void Clear();
  //@{
  /**
   * Generate field data arrays that have all scalar/vector
   * fields interpolated to the quadrature points. The type
   * of quadrature used is found in the dictionary stored as
   * meta data in each data array.
   */
  int InterpolateFields(svtkUnstructuredGrid* usgOut);
  //@}
};

#endif
