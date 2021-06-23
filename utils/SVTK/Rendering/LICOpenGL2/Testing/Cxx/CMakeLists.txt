# basic tests on the legacy filters
# exercises line integral convolution 2d
svtk_add_test_cxx(svtkRenderingLICOpenGL2CxxTests tests
  TestImageDataLIC2D.cxx
  TestStructuredGridLIC2DXSlice.cxx
  TestStructuredGridLIC2DYSlice.cxx
  TestStructuredGridLIC2DZSlice.cxx
  )

# surface lic painter tests
# get decent coverage by calling
# varying arguments to the following
# test exec. to get unique test names
# avoid svtk_add_test_cxx and add exec
# and it's test cases manually
list(APPEND tests
  TestSurfaceLIC
  )
svtk_test_cxx_executable(svtkRenderingLICOpenGL2CxxTests tests
  RENDERING_FACTORY
  svtkStructuredGridLIC2DTestDriver.cxx
  svtkSurfaceLICTestDriver.cxx
  )

# surface lic on a curved surface
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedDefaults
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedDefaults.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedBlended
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedBlended.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --enhance-contrast=1
    --lic-intensity=0.8
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedMapped
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedMapped.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --enhance-contrast=4
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedEnhancedVectorNormalizeOff
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedEnhancedVectorNormalizeOff.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --camera-config=1
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=800
    --enhance-contrast=1
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedBlendedSmallGrain
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedBlendedSmallGrain.png,:}"
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=1
    --lic-intensity=0.8
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedMappedSmallGrain
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedMappedSmallGrain.png,:}"
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=4
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedMappedSmallVectorNormalizeOff
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedMappedSmallVectorNormalizeOff.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --camera-config=1
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=800
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=1
    --low-lic-contrast-enhancement-factor=0.05
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedDefaultsColor
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedDefaultsColor.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --color-by-mag=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedColorBlendedSmallGrain
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorBlendedSmallGrain.png,:}"
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=1
    --color-by-mag=1
    --lic-intensity=0.6
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedColorMappedSmallGrain
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorMappedSmallGrain.png,:}"
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=4
    --color-by-mag=1
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedColorBlendedSmallGrainMask
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorBlendedSmallGrainMask.png,:}"
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=1
    --color-by-mag=1
    --lic-intensity=0.6
    --mask-intensity=0.2
    --mask-color 1.0 1.0 1.0
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedColorMappedSmallGrainMask
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorMappedSmallGrainMask.png,:}"
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=1
    --high-lic-contrast-enhancement-factor=0.1
    --color-by-mag=1
    --color-mode=1
    --map-mode-bias=0.05
    --mask-intensity=0.2
    --mask-color 1.0 1.0 1.0
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICCurvedContrastEnhancedSmallGrainMask
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICCurvedContrastEnhancedSmallGrainMask.png,:}"
    --camera-config=1
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/disk_out_ref_surface.vtp}
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=800
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=1
    --low-lic-contrast-enhancement-factor=0.05
    --color-mode=1
    --map-mode-bias=0.05
    --mask-intensity=0.2
    )
# surface lic on a slice
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICPlanarDefaults
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICPlanarDefaults.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/SurfaceVectors.svtk}
    --camera-config=3
    --vectors=V
    --step-size=1
    --num-steps=40
    --lic-intensity=0.8
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICPlanarContrastEnhanced
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICPlanarContrastEnhanced.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/SurfaceVectors.svtk}
    --camera-config=3
    --vectors=V
    --step-size=1
    --num-steps=40
    --enhance-contrast=1
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICPlanarVectorNormalizeOff
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICPlanarVectorNormalizeOff.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/SurfaceVectors.svtk}
    --camera-config=3
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=2000
    --generate-noise-texture=1
    --noise-texture-size=128
    --noise-grain-size=2
    --number-of-noise-levels=256
    --noise-gen-seed=12367
    --enhance-contrast=1
    --color-by-mag=1
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICPlanarVectorNormalizeOffMediumGrainUniform
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICPlanarVectorNormalizeOffMediumGrainUniform.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/SurfaceVectors.svtk}
    --camera-config=3
    --vectors=V
    --enhanced-lic=0
    --step-size=0.25
    --num-steps=40
    --generate-noise-texture=1
    --noise-type=1
    --noise-texture-size=128
    --noise-grain-size=2
    --impulse-noise-prob=0.04
    --enhance-contrast=4
    --high-color-contrast-enhancement-factor=0.2
    --color-by-mag=1
    --color-mode=1
    )
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICPlanarVectorNormalizeOffMediumGrainPerlin
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICPlanarVectorNormalizeOffMediumGrainPerlin.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/SurfaceVectors.svtk}
    --camera-config=3
    --vectors=V
    --normalize-vectors=0
    --step-size=2
    --num-steps=400
    --generate-noise-texture=1
    --noise-type=2
    --noise-texture-size=256
    --noise-grain-size=8
    --max-noise-value=0.8
    --enhance-contrast=4
    --color-by-mag=1
    --color-mode=1
    --anti-alias=1
    )

svtk_module_test_data(
  Data/ex-blow_5/,REGEX:.*)

ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::RenderingLICOpenGL2Cxx-SurfaceLICMultiBlockContrastEnhancedPerlin
  COMMAND svtkRenderingLICOpenGL2CxxTests TestSurfaceLIC
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V "DATA{../Data/Baseline/TestSurfaceLICMultiBlockContrastEnhancedPerlin.png,:}"
    --data=DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/ex-blow_5.vtm}
    --camera-config=4
    --vectors=DISPLACEMENT_
    --step-size=0.1
    --num-steps=50
    --generate-noise-texture=1
    --noise-type=2
    --noise-texture-size=128
    --noise-grain-size=2
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=4
    --color-by-mag=1
    --color-mode=1
    --map-mode-bias=0.1
    --mask-on-surface=1
    --mask-intensity=0.41
    --mask-color 0 0 0
    )
