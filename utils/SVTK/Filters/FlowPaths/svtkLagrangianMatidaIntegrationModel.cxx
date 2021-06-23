/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianMatidaIntegrationModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLagrangianMatidaIntegrationModel.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIntArray.h"
#include "svtkLagrangianParticle.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStringArray.h"

#include <cstring>

svtkObjectFactoryNewMacro(svtkLagrangianMatidaIntegrationModel);

//---------------------------------------------------------------------------
svtkLagrangianMatidaIntegrationModel::svtkLagrangianMatidaIntegrationModel()
{
  // Fill the helper array
  this->SeedArrayNames->InsertNextValue("ParticleDiameter");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(SVTK_DOUBLE);
  this->SeedArrayNames->InsertNextValue("ParticleDensity");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(SVTK_DOUBLE);

  this->NumFuncs = 6;     // u, v, w, du/dt, dv/dt, dw/dt
  this->NumIndepVars = 7; // x, y, z, u, v, w, t
}

//---------------------------------------------------------------------------
svtkLagrangianMatidaIntegrationModel::~svtkLagrangianMatidaIntegrationModel() = default;

//---------------------------------------------------------------------------
void svtkLagrangianMatidaIntegrationModel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
int svtkLagrangianMatidaIntegrationModel::FunctionValues(svtkLagrangianParticle* particle,
  svtkDataSet* dataSet, svtkIdType cellId, double* weights, double* x, double* f)
{
  // Initialize output
  std::fill(f, f + 6, 0.0);

  if (!particle)
  {
    svtkErrorMacro(<< "No particle to integrate");
    return 0;
  }

  // Sanity Check
  if (!dataSet || cellId == -1)
  {
    svtkErrorMacro(<< "No cell or dataset to integrate the particle on. Dataset: " << dataSet
                  << " CellId:" << cellId);
    return 0;
  }

  // Fetch flowVelocity at index 3
  double flowVelocity[3];
  if (this->GetFlowOrSurfaceDataNumberOfComponents(3, dataSet) != 3 ||
    !this->GetFlowOrSurfaceData(particle, 3, dataSet, cellId, weights, flowVelocity))
  {
    svtkErrorMacro(<< "Flow velocity is not set in source flow dataset or "
                     "has incorrect number of components, cannot use Matida equations");
    return 0;
  }

  // Fetch flowDensity at index 4
  double flowDensity;
  if (this->GetFlowOrSurfaceDataNumberOfComponents(4, dataSet) != 1 ||
    !this->GetFlowOrSurfaceData(particle, 4, dataSet, cellId, weights, &flowDensity))
  {
    svtkErrorMacro(<< "Flow density is not set in source flow dataset or "
                     "has incorrect number of components, cannot use Matida equations");
    return 0;
  }

  // Fetch flowDynamicViscosity at index 5
  double flowDynamicViscosity;
  if (this->GetFlowOrSurfaceDataNumberOfComponents(5, dataSet) != 1 ||
    !this->GetFlowOrSurfaceData(particle, 5, dataSet, cellId, weights, &flowDynamicViscosity))
  {
    svtkErrorMacro(<< "Flow dynamic viscosity is not set in source flow dataset or "
                     "has incorrect number of components, cannot use Matida equations");
    return 0;
  }

  // Fetch Particle Diameter at index 6
  svtkDataArray* particleDiameters = svtkDataArray::SafeDownCast(this->GetSeedArray(6, particle));
  if (!particleDiameters)
  {
    svtkErrorMacro(<< "Particle diameter is not set in particle data, "
                     "cannot use Matida equations");
    return 0;
  }
  if (particleDiameters->GetNumberOfComponents() != 1)
  {
    svtkErrorMacro(<< "Particle diameter does not have the right number of components, "
                     "cannot use Matida equations");
    return 0;
  }
  double particleDiameter;
  particleDiameters->GetTuple(particle->GetSeedArrayTupleIndex(), &particleDiameter);

  // Fetch Particle Density at index 7
  svtkDataArray* particleDensities = svtkDataArray::SafeDownCast(this->GetSeedArray(7, particle));
  if (!particleDensities)
  {
    svtkErrorMacro(<< "Particle density is not set in particle data, "
                     "cannot use Matida equations");
    return 0;
  }
  if (particleDensities->GetNumberOfComponents() != 1)
  {
    svtkErrorMacro(<< "Particle densities does not have the right number of components, "
                     "cannot use Matida equations");
    return 0;
  }
  double particleDensity;
  particleDensities->GetTuple(particle->GetSeedArrayTupleIndex(), &particleDensity);

  // Compute function values
  for (int i = 0; i < 3; i++)
  {
    double drag = this->GetDragCoefficient(
      flowVelocity, particle->GetVelocity(), flowDynamicViscosity, particleDiameter, flowDensity);
    double relax = this->GetRelaxationTime(flowDynamicViscosity, particleDiameter, particleDensity);
    // Matida Equation
    f[i + 3] = (relax == 0) ? std::numeric_limits<double>::infinity()
                            : (flowVelocity[i] - x[i + 3]) * drag / relax;
    f[i] = x[i + 3];
  }

  const double G = 9.8; // Gravity
  f[5] -= G * (1 - (flowDensity / particleDensity));
  return 1;
}

//---------------------------------------------------------------------------
double svtkLagrangianMatidaIntegrationModel::GetRelaxationTime(
  double dynVisc, double diameter, double density)
{
  return (dynVisc == 0) ? std::numeric_limits<double>::infinity()
                        : (density * diameter * diameter) / (18.0 * dynVisc);
}

//---------------------------------------------------------------------------
double svtkLagrangianMatidaIntegrationModel::GetDragCoefficient(const double* flowVelocity,
  const double* particleVelocity, double dynVisc, double particleDiameter, double flowDensity)
{
  if (dynVisc == 0)
  {
    return -1.0 * std::numeric_limits<double>::infinity();
  }
  double relativeVelocity[3];
  for (int i = 0; i < 3; i++)
  {
    relativeVelocity[i] = particleVelocity[i] - flowVelocity[i];
  }
  double relativeSpeed = svtkMath::Norm(relativeVelocity);
  double reynolds = flowDensity * relativeSpeed * particleDiameter / dynVisc;
  return (1.0 + 0.15 * pow(reynolds, 0.687));
}
