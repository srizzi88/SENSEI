svtk_add_test_cxx(svtkCommonArchiveCxxTests tests
  TestBufferedRenderWindowExporter.cxx,NO_DATA,NO_VALID
  TestPartitionedRenderWindowExporter.cxx,NO_DATA,NO_VALID
  )
svtk_test_cxx_executable(svtkCommonArchiveCxxTests tests
  RENDERING_FACTORY
  )

find_package(LibArchive REQUIRED)

target_link_libraries(svtkCommonArchiveCxxTests
  PRIVATE
  LibArchive::LibArchive)
