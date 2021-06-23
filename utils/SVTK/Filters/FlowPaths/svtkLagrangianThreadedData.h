/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @struct   svtkLagrangianThreadedData
 * @brief   struct to hold a user data
 *
 * Struct to hold threaded data used by the Lagrangian Particle Tracker
 */

#ifndef svtkLagrangianThreadedData_h
#define svtkLagrangianThreadedData_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkPolyData.h"

class svtkBilinearQuadIntersection;
class svtkDataObject;
class svtkInitialValueProblemSolver;

struct SVTKFILTERSFLOWPATHS_EXPORT svtkLagrangianThreadedData
{
  svtkNew<svtkGenericCell> GenericCell;
  svtkNew<svtkIdList> IdList;
  svtkNew<svtkPolyData> ParticlePathsOutput;

  svtkBilinearQuadIntersection* BilinearQuadIntersection;
  svtkDataObject* InteractionOutput;
  svtkInitialValueProblemSolver* Integrator;
  void* UserData;
};
#endif // svtkLagrangianThreadedData_h
// SVTK-HeaderTest-Exclude: svtkLagrangianThreadedData.h
