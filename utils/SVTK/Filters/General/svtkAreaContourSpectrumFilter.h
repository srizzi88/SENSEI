/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAreaContourSpectrumFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAreaContourSpectrumFilter
 * @brief   compute an approximation of the area
 * contour signature (evolution of the area of the input surface along an arc of
 * the Reeb graph).
 *
 * The filter takes a svtkPolyData as an input (port 0), along with a
 * svtkReebGraph (port 1).
 * The Reeb graph arc to consider can be specified with SetArcId() (default: 0).
 * The number of (evenly distributed) samples of the signature can be defined
 * with SetNumberOfSamples() (default value: 100).
 * The filter will first try to pull as a scalar field the svtkDataArray with Id
 * 'FieldId' of the svtkPolyData, see SetFieldId (default: 0). The filter will
 * abort if this field does not exist.
 *
 * The filter outputs a svtkTable with the area contour signature
 * approximation, each sample being evenly distributed in the function span of
 * the arc.
 *
 * This filter is a typical example for designing your own contour signature
 * filter (with customized metrics). It also shows typical svtkReebGraph
 * traversals.
 *
 * Reference:
 * C. Bajaj, V. Pascucci, D. Schikore,
 * "The contour spectrum",
 * IEEE Visualization, 167-174, 1997.
 */

#ifndef svtkAreaContourSpectrumFilter_h
#define svtkAreaContourSpectrumFilter_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkTable;

class SVTKFILTERSGENERAL_EXPORT svtkAreaContourSpectrumFilter : public svtkDataObjectAlgorithm
{
public:
  static svtkAreaContourSpectrumFilter* New();
  svtkTypeMacro(svtkAreaContourSpectrumFilter, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the arc Id for which the contour signature has to be computed.
   * Default value: 0
   */
  svtkSetMacro(ArcId, svtkIdType);
  svtkGetMacro(ArcId, svtkIdType);
  //@}

  //@{
  /**
   * Set the number of samples in the output signature
   * Default value: 100
   */
  svtkSetMacro(NumberOfSamples, int);
  svtkGetMacro(NumberOfSamples, int);
  //@}

  //@{
  /**
   * Set the scalar field Id
   * Default value: 0
   */
  svtkSetMacro(FieldId, svtkIdType);
  svtkGetMacro(FieldId, svtkIdType);
  //@}

  svtkTable* GetOutput();

protected:
  svtkAreaContourSpectrumFilter();
  ~svtkAreaContourSpectrumFilter() override;

  svtkIdType ArcId, FieldId;
  int NumberOfSamples;

  int FillInputPortInformation(int portNumber, svtkInformation*) override;
  int FillOutputPortInformation(int portNumber, svtkInformation* info) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkAreaContourSpectrumFilter(const svtkAreaContourSpectrumFilter&) = delete;
  void operator=(const svtkAreaContourSpectrumFilter&) = delete;
};

#endif
