set(classes
  svtkAbstractInterpolatedVelocityField
  svtkAMRInterpolatedVelocityField
  svtkCachingInterpolatedVelocityField
  svtkCellLocatorInterpolatedVelocityField
  svtkCompositeInterpolatedVelocityField
  svtkEvenlySpacedStreamlines2D
  svtkInterpolatedVelocityField
  svtkLagrangianBasicIntegrationModel
  svtkLagrangianMatidaIntegrationModel
  svtkLagrangianParticle
  svtkLagrangianParticleTracker
  svtkModifiedBSPTree
  svtkParticlePathFilter
  svtkParticleTracer
  svtkParticleTracerBase
  svtkStreaklineFilter
  svtkStreamTracer
  svtkTemporalInterpolatedVelocityField)

if (NOT SVTK_LEGACY_REMOVE)
  list(APPEND classes svtkTemporalStreamTracer)
endif()
svtk_module_add_module(SVTK::FiltersFlowPaths
  CLASSES ${classes})
