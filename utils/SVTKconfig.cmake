# PLEASE DO NOT ENABLE MORE SVTK FEATURES!!  The goal is to keep this small and
# tight, we are using SVTK only for the SENSEI data model. If you need
# additional VTK functionality bring in VTK propper.
set(SVTK_LEGACY_SILENT ON CACHE INTERNAL "")
set(SVTK_LEGACY_REMOVE OFF CACHE INTERNAL "")
set(SVTK_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(SVTK_BUILD_TESTING OFF CACHE INTERNAL "")
set(SVTK_BUILD_DOCUMENTATION OFF CACHE INTERNAL "")
set(SVTK_BUILD_SHARED_LIBS ON CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_AcceleratorsSVTKm NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ChartsCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonArchive NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonColor NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonComputationalGeometry NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonDataModel YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonExecutionModel NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonMath YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonMisc YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonSystem YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_CommonTransforms YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_DICOMParser NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_DomainsChemistry NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_DomainsChemistryOpenGL2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_DomainsMicroscopy NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_DomainsParallelChemistry NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersAMR NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersExtraction NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersFlowPaths NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersGeneral NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersGeneric NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersGeometry NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersHybrid NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersHyperTree NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersImaging NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersModeling NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersOpenTURNS NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallel NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallelDIY2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallelFlowPaths NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallelGeometry NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallelImaging NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallelMPI NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallelStatistics NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersParallelVerdict NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersPoints NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersProgrammable NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersReebGraph NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersSMP NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersSelection NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersSources NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersStatistics NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersTexture NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersTopology NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_FiltersVerdict NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_GUISupportQt NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_GUISupportQtSQL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_GeovisCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_GeovisGDAL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOADIOS2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOAMR NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOAsynchronous NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOCityGML NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOEnSight NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOExodus NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOExport NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOExportGL2PS NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOExportPDF NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOFFMPEG NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOGDAL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOGeoJSON NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOGeometry NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOH5part NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOImage NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOImport NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOInfovis NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOLAS NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOLSDyna NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOLegacy NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOMINC NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOMPIImage NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOMPIParallel NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOMotionFX NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOMovie NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOMySQL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IONetCDF NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOODBC NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOOggTheora NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOPDAL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOPIO NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOPLY NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOParallel NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOParallelExodus NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOParallelLSDyna NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOParallelNetCDF NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOParallelXML NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOParallelXdmf3 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOPostgreSQL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOSQL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOSegY NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOTRUCHAS NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOTecplotTable NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOVPIC NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOVeraOut NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOVideo NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOXML NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOXMLParser NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOXdmf2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_IOXdmf3 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingColor NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingFourier NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingGeneral NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingHybrid NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingMath NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingMorphological NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingOpenGL2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingSources NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingStatistics NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ImagingStencil NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_InfovisBoost NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_InfovisBoostGraphAlgorithms NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_InfovisCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_InfovisLayout NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_InteractionImage NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_InteractionStyle NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_InteractionWidgets NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_MomentInvariants NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ParallelCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ParallelDIY NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ParallelMPI NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_PoissonReconstruction NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_Powercrust NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_PythonInterpreter NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingAnnotation NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingContext2D NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingContextOpenGL2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingExternal NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingFreeType NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingFreeTypeFontConfig NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingGL2PSOpenGL2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingImage NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingLICOpenGL2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingLOD NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingLabel NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingMatplotlib NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingOpenGL2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingOpenVR NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingParallel NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingParallelLIC NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingQt NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingRayTracing NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingSceneGraph NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingUI NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingVolume NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingVolumeAMR NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingVolumeOpenGL2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_RenderingVtkJS NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_SignedTensor NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_SplineDrivenImageSlicer NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_TestingCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_TestingGenericBridge NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_TestingIOSQL NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_TestingRendering NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_UtilitiesBenchmarks NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ViewsContext2D NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ViewsCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ViewsInfovis NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ViewsQt NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_WebCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_WebGLExporter NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_WrappingPythonCore NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_WrappingTools NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_diy2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_doubleconversion NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_eigen NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_exodusII NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_expat NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_freetype NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_gl2ps NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_glew NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_h5part NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_hdf5 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_jpeg NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_jsoncpp NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_kissfft NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_kwiml YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_libharu NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_libproj NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_libxml2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_loguru NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_lz4 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_lzma NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_metaio NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_mpi NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_mpi4py NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_netcdf NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_octree NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_ogg NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_opengl NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_pegtl NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_png NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_pugixml NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_sqlite NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_svtkDICOM NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_svtkm NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_svtksys YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_theora NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_tiff NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_utf8 YES CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_verdict NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_vpic NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_xdmf2 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_xdmf3 NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_zfp NO CACHE INTERNAL "")
set(SVTK_MODULE_ENABLE_SVTK_zlib NO CACHE INTERNAL "")
set(SVTK_ENABLE_WRAPPING OFF CACHE INTERNAL "")
set(SVTK_WRAP_JAVA OFF CACHE INTERNAL "")
set(SVTK_WRAP_PYTHON OFF CACHE INTERNAL "")
set(SVTK_USE_MPI OFF CACHE INTERNAL "")
set(SVTK_ENABLE_LOGGING OFF CACHE INTERNAL "")