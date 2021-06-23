set(classes
  svtkPLagrangianParticleTracker
  svtkPParticlePathFilter
  svtkPParticleTracer
  svtkPParticleTracerBase
  svtkPStreaklineFilter
  svtkPStreamTracer)

# Overrides for object factory.
set(parallel_overrides
  LagrangianParticleTracker
  StreamTracer
  ParticleTracer
  ParticlePathFilter
  StreaklineFilter)

if (NOT SVTK_LEGACY_REMOVE)
  list(APPEND classes svtkPTemporalStreamTracer)
  list(APPEND parallel_overrides TemporalStreamTracer)
endif()

foreach (parallel_override IN LISTS parallel_overrides)
  svtk_object_factory_declare(
    BASE      svtk${parallel_override}
    OVERRIDE  svtkP${parallel_override})
endforeach ()

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKFILTERSPARALLELFLOWPATHS_EXPORT")

svtk_module_add_module(SVTK::FiltersParallelFlowPaths
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  HEADERS ${svtk_object_factory_header})
