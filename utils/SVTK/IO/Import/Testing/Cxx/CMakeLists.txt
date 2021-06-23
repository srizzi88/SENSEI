# This is an alternative  way to add a named test that uses a common executable
# Use this if it does not require MPI

set(TEMP ${SVTK_BINARY_DIR}/Testing/Temporary)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  OBJImporter-TwoTrianglesWithNegativeIndices,TestOBJImporter.cxx
    DATA{../Data/Input/TwoTrianglesWithNegativeIndices.obj}
    DATA{../Data/Input/TwoTrianglesWithNegativeIndices.mtl}
    DATA{../Data/Input/noise.png}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  OBJImport-SolidAndTextured,TestOBJImporter.cxx
    DATA{../Data/Input/cube-scene.obj}
    DATA{../Data/Input/cube-scene.mtl}
    DATA{../Data/Input/noise.png}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  OBJImport-MixedOrder1,TestOBJImporter.cxx
    DATA{../Data/Input/boxes_2.obj}
    DATA{../Data/Input/boxes_2.obj.mtl}
    DATA{../Data/Input/flare.jpg}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  OBJImport-NoMTL,TestOBJImporter.cxx
    DATA{../Data/Input/boxes_3_no_mtl.obj}
    DATA{../Data/Input/boxes_2.obj.mtl}
    DATA{../Data/Input/flare.jpg}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  OBJImport-MTLwithoutTextureFile,TestOBJImporter.cxx
    DATA{../Data/Input/boxes_4_mtl_no_texture.obj}
    DATA{../Data/Input/boxes_4_mtl_no_texture.obj.mtl}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestOBJImporter-DuplicateMaps,TestOBJImporter.cxx
    DATA{../Data/Input/TestOBJImporter.obj}
    DATA{../Data/Input/TestOBJImporter.mtl}
    DATA{../Data/Input/TestOBJImporter.png}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestOBJImporter-SharedMtl,TestOBJImporter.cxx
    DATA{../Data/Input/TestOBJImporter-SharedMtl.obj}
    DATA{../Data/Input/TestOBJImporter-SharedMtl.mtl}
)


svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestOBJImporter-VtNoIndices,TestOBJImporter.cxx
    DATA{../Data/Input/TestOBJImporter-VtNoIndices.obj}
    DATA{../Data/Input/TestOBJImporter-VtNoIndices.mtl}
    DATA{../Data/Input/TestOBJImporter-VtNoIndices.png}
)


svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestVRMLImporter.cxx,NO_VALID
  TestVRMLNormals.cxx
  )

svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestGLTFImporter,TestGLTFImporter.cxx DATA{../Data/glTF/Avocado/Avocado.glb}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestGLTFImporterPBR,TestGLTFImporter.cxx DATA{../Data/glTF/WaterBottle/WaterBottle.glb}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestGLTFImporterKHRLightsPunctual,TestGLTFImporter.cxx DATA{../Data/glTF/Lights/lights.gltf}
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  NO_VALID
  TestImportGLFTExportOBJ,TestImportExportOBJ.cxx
  DATA{../Data/glTF/Avocado/Avocado.glb}
  ${TEMP}/Avocado
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  NO_VALID
  TestImport3DSExportOBJ,TestImportExportOBJ.cxx
  DATA{../../../../Testing/Data/iflamigm.3ds}
  ${TEMP}/iflamigm
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestImport3DS,TestImportOBJ.cxx
  ${TEMP}/iflamigm.obj
  ${TEMP}/iflamigm.mtl
  ${TEMP}
)

set_tests_properties(SVTK::IOImportCxx-TestImport3DS
      PROPERTIES
      DEPENDS
      "SVTK::IOImportCxx-TestImport3DSExportOBJ"
)

svtk_add_test_cxx(svtkIOImportCxxTests tests
  TestImportGLFT,TestImportOBJ.cxx
  ${TEMP}/Avocado.obj
  ${TEMP}/Avocado.mtl
  ${TEMP}
)

set_tests_properties(SVTK::IOImportCxx-TestImportGLFT
      PROPERTIES
      DEPENDS
      "SVTK::IOImportCxx-TestImportGLFTExportOBJ"
)

svtk_test_cxx_executable(svtkIOImportCxxTests tests
  RENDERING_FACTORY)
