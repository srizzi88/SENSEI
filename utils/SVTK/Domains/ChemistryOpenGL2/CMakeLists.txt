set(classes
  svtkOpenGLMoleculeMapper
  )

# Now for the object factory.
set(opengl_overrides
  MoleculeMapper
  )

foreach(_override ${opengl_overrides})
  svtk_object_factory_declare(
    BASE svtk${_override}
    OVERRIDE svtkOpenGL${_override})
endforeach()

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKDOMAINSCHEMISTRYOPENGL2_EXPORT")

svtk_module_add_module(SVTK::DomainsChemistryOpenGL2
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
