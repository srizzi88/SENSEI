svtk_add_test_python(
  VToXLoop.py,NO_VALID,NO_RT,NO_DATA
  ReadXdmfPolyhedron.py,NO_VALID,NO_RT
  )

if (SVTK_USE_LARGE_DATA)
  set(_ManyFiles_ARGS "--do_big_files")
  set(_BigFiles
    "Data/XDMF/Big/,REGEX:.*"
    "Data/XDMF/Iron/,REGEX:.*")
endif()
svtk_module_test_data(
  Data/XDMF/,REGEX:.*
  ${_BigFiles}
  )
svtk_add_test_python(
  ManyFiles.py NO_OUTPUT NO_VALID NO_RT ${_ManyFiles_ARGS}
)
