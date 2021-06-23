/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlatonicSolidSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPlatonicSolidSource
 * @brief   produce polygonal Platonic solids
 *
 * svtkPlatonicSolidSource can generate each of the five Platonic solids:
 * tetrahedron, cube, octahedron, icosahedron, and dodecahedron. Each of the
 * solids is placed inside a sphere centered at the origin with radius 1.0.
 * To use this class, simply specify the solid to create. Note that this
 * source object creates cell scalars that are (integral value) face numbers.
 */

#ifndef svtkPlatonicSolidSource_h
#define svtkPlatonicSolidSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_SOLID_TETRAHEDRON 0
#define SVTK_SOLID_CUBE 1
#define SVTK_SOLID_OCTAHEDRON 2
#define SVTK_SOLID_ICOSAHEDRON 3
#define SVTK_SOLID_DODECAHEDRON 4

class SVTKFILTERSSOURCES_EXPORT svtkPlatonicSolidSource : public svtkPolyDataAlgorithm
{
public:
  static svtkPlatonicSolidSource* New();
  svtkTypeMacro(svtkPlatonicSolidSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the type of PlatonicSolid solid to create.
   */
  svtkSetClampMacro(SolidType, int, SVTK_SOLID_TETRAHEDRON, SVTK_SOLID_DODECAHEDRON);
  svtkGetMacro(SolidType, int);
  void SetSolidTypeToTetrahedron() { this->SetSolidType(SVTK_SOLID_TETRAHEDRON); }
  void SetSolidTypeToCube() { this->SetSolidType(SVTK_SOLID_CUBE); }
  void SetSolidTypeToOctahedron() { this->SetSolidType(SVTK_SOLID_OCTAHEDRON); }
  void SetSolidTypeToIcosahedron() { this->SetSolidType(SVTK_SOLID_ICOSAHEDRON); }
  void SetSolidTypeToDodecahedron() { this->SetSolidType(SVTK_SOLID_DODECAHEDRON); }
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkPlatonicSolidSource();
  ~svtkPlatonicSolidSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int SolidType;
  int OutputPointsPrecision;

private:
  svtkPlatonicSolidSource(const svtkPlatonicSolidSource&) = delete;
  void operator=(const svtkPlatonicSolidSource&) = delete;
};

#endif
