/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProbeSelectedLocations.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProbeSelectedLocations
 * @brief   similar to svtkExtractSelectedLocations
 * except that it interpolates the point attributes at the probe locations.
 *
 * svtkProbeSelectedLocations is similar to svtkExtractSelectedLocations except
 * that it interpolates the point attributes at the probe location. This is
 * equivalent to the svtkProbeFilter except that the probe locations are provided
 * by a svtkSelection. The FieldType of the input svtkSelection is immaterial and
 * is ignored. The ContentType of the input svtkSelection must be
 * svtkSelection::LOCATIONS.
 */

#ifndef svtkProbeSelectedLocations_h
#define svtkProbeSelectedLocations_h

#include "svtkExtractSelectionBase.h"
#include "svtkFiltersExtractionModule.h" // For export macro

class SVTKFILTERSEXTRACTION_EXPORT svtkProbeSelectedLocations : public svtkExtractSelectionBase
{
public:
  static svtkProbeSelectedLocations* New();
  svtkTypeMacro(svtkProbeSelectedLocations, svtkExtractSelectionBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkProbeSelectedLocations();
  ~svtkProbeSelectedLocations() override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkProbeSelectedLocations(const svtkProbeSelectedLocations&) = delete;
  void operator=(const svtkProbeSelectedLocations&) = delete;
};

#endif
