svtk_add_test_python(RectOutline.py)
svtk_add_test_python(TestCutMaterial.py)
svtk_add_test_python(TestExtractCTHPart.py)
svtk_add_test_python(TestExtractCTHPartSolid.py)
svtk_add_test_python(TestExtrudePiece.py)
svtk_add_test_python(TestImageStreamer.py)
svtk_add_test_python(TestIntegrateAttributes.py NO_OUTPUT NO_VALID NO_RT)
svtk_add_test_python(TestRemoveGhosts.py NO_DATA NO_OUTPUT NO_VALID NO_RT)
svtk_add_test_python(TestResamplingInterpolator.py)

if(TARGET psvtkpython)
  add_test(NAME svtkFiltersParallelPython-testTransmit
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
      $<TARGET_FILE:psvtkpython> ${CMAKE_CURRENT_SOURCE_DIR}/testTransmit.py
      ${SVTK_MPI_POSTFLAGS})
endif()
