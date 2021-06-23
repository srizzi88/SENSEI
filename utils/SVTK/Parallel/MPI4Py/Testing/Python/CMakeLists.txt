set(SVTK_PYTHON_EXE $<TARGET_FILE:psvtkpython>)
svtk_add_test_python_mpi(
  TestParallelNumpy.py,NO_VALID,NO_DATA,NO_OUTPUT
  )
