/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQuadratureSchemeDictionaryGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQuadratureSchemeDictionaryGenerator
 *
 *
 * Given an unstructured grid on its input this filter generates
 * for each data array in point data dictionary (ie an instance of
 * svtkInformationQuadratureSchemeDefinitionVectorKey). This filter
 * has been introduced to facilitate testing of the svtkQuadrature*
 * classes as these cannot operate with the dictionary. This class
 * is for testing and should not be used for application development.
 *
 * @sa
 * svtkQuadraturePointInterpolator, svtkQuadraturePointsGenerator, svtkQuadratureSchemeDefinition
 */

#ifndef svtkQuadratureSchemeDictionaryGenerator_h
#define svtkQuadratureSchemeDictionaryGenerator_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkPolyData;
class svtkUnstructuredGrid;
class svtkInformation;
class svtkInformationVector;

class SVTKFILTERSGENERAL_EXPORT svtkQuadratureSchemeDictionaryGenerator : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkQuadratureSchemeDictionaryGenerator, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkQuadratureSchemeDictionaryGenerator* New();

protected:
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int RequestData(
    svtkInformation* req, svtkInformationVector** input, svtkInformationVector* output) override;
  svtkQuadratureSchemeDictionaryGenerator();
  ~svtkQuadratureSchemeDictionaryGenerator() override;

private:
  svtkQuadratureSchemeDictionaryGenerator(const svtkQuadratureSchemeDictionaryGenerator&) = delete;
  void operator=(const svtkQuadratureSchemeDictionaryGenerator&) = delete;

  //@{
  /**
   * Generate definitions for each cell type found on the
   * input data set. The same definition will be used
   * for all point data arrays.
   */
  int Generate(svtkUnstructuredGrid* usgOut);
  //@}
};

#endif
