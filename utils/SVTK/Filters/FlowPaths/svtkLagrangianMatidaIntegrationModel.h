/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianMatidaIntegrationModel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLagrangianMatidaIntegrationModel
 * svtkLagrangianBasicIntegrationModel implementation
 *
 *
 * svtkLagrangianBasicIntegrationModel implementation using
 * article :
 * "Matida, E. A., et al. "Improved numerical simulation of aerosol deposition in
 * an idealized mouth-throat." Journal of Aerosol Science 35.1 (2004): 1-19."
 * Input Array to process are expected as follow :
 * Index 1 is the "FlowVelocity" from flow input in the tracker
 * Index 2 is the "FlowDensity" from flow input in the tracker
 * Index 3 is the "FlowDynamicViscosity" from flow input in the tracker
 * Index 4 is the "ParticleDiameter" from seed (source) input in the tracker
 * Index 5 is the "ParticleDensity" from seed (source) input in the tracker
 *
 * @sa
 * svtkLagrangianParticleTracker svtkLagrangianParticle
 * svtkLagrangianBasicIntegrationModel
 */

#ifndef svtkLagrangianMatidaIntegrationModel_h
#define svtkLagrangianMatidaIntegrationModel_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkLagrangianBasicIntegrationModel.h"

class SVTKFILTERSFLOWPATHS_EXPORT svtkLagrangianMatidaIntegrationModel
  : public svtkLagrangianBasicIntegrationModel
{
public:
  svtkTypeMacro(svtkLagrangianMatidaIntegrationModel, svtkLagrangianBasicIntegrationModel);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkLagrangianMatidaIntegrationModel* New();

  // Needed for multiple signatures polymorphism
  using Superclass::FunctionValues;

  /**
   * Evaluate the integration model velocity field
   * f at position x, using data from cell in dataSet with index cellId
   */
  int FunctionValues(svtkLagrangianParticle* particle, svtkDataSet* dataSet, svtkIdType cellId,
    double* weights, double* x, double* f) override;

protected:
  svtkLagrangianMatidaIntegrationModel();
  ~svtkLagrangianMatidaIntegrationModel() override;

  static double GetRelaxationTime(double dynVisc, double diameter, double density);

  static double GetDragCoefficient(const double* flowVelocity, const double* particleVelocity,
    double dynVisc, double particleDiameter, double flowDensity);

private:
  svtkLagrangianMatidaIntegrationModel(const svtkLagrangianMatidaIntegrationModel&) = delete;
  void operator=(const svtkLagrangianMatidaIntegrationModel&) = delete;
};

#endif
