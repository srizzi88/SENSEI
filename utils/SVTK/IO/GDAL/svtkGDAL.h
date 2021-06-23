/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGDAL.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGDAL
 * @brief   Shared data for GDAL classes
 *
 */

#ifndef svtkGDAL_h
#define svtkGDAL_h

#include "svtkObject.h"
#include <svtkIOGDALModule.h> // For export macro

class svtkInformationStringKey;
class svtkInformationIntegerVectorKey;

class SVTKIOGDAL_EXPORT svtkGDAL : public svtkObject
{
public:
  svtkTypeMacro(svtkGDAL, svtkObject);
  // Key used to put GDAL map projection string in the output information
  // by readers.
  static svtkInformationStringKey* MAP_PROJECTION();
  static svtkInformationIntegerVectorKey* FLIP_AXIS();

protected:
private:
  svtkGDAL(); // Static class
  ~svtkGDAL() override;
  svtkGDAL(const svtkGDAL&) = delete;
  void operator=(const svtkGDAL&) = delete;
};

#endif // svtkGDAL_h
// SVTK-HeaderTest-Exclude: svtkGDAL.h
