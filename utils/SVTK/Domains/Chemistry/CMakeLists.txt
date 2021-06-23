set(classes
  svtkBlueObeliskData
  svtkBlueObeliskDataParser
  svtkCMLMoleculeReader
  svtkGaussianCubeReader2
  svtkMoleculeMapper
  svtkMoleculeToAtomBallFilter
  svtkMoleculeToBondStickFilter
  svtkMoleculeToLinesFilter
  svtkMoleculeToPolyDataFilter
  svtkPeriodicTable
  svtkPointSetToMoleculeFilter
  svtkProgrammableElectronicData
  svtkProteinRibbonFilter
  svtkSimpleBondPerceiver
  svtkVASPAnimationReader
  svtkVASPTessellationReader
  svtkXYZMolReader2)

# Generate configured header file
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkChemistryConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkChemistryConfigure.h"
  @ONLY)

svtk_module_add_module(SVTK::DomainsChemistry
  CLASSES ${classes}
  HEADERS "${CMAKE_CURRENT_BINARY_DIR}/svtkChemistryConfigure.h"
  PRIVATE_HEADERS svtkBlueObeliskDataInternal.h)
