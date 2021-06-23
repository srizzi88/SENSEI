set(classes
  svtkExporter
  svtkGLTFExporter
  svtkIVExporter
  svtkJSONDataSetWriter
  svtkJSONRenderWindowExporter
  svtkJSONSceneExporter
  svtkOBJExporter
  svtkOOGLExporter
  svtkPOVExporter
  svtkRIBExporter
  svtkRIBLight
  svtkRIBProperty
  svtkSVGContextDevice2D
  svtkSVGExporter
  svtkSingleVTPExporter
  svtkVRMLExporter
  svtkX3D
  svtkX3DExporter
  svtkX3DExporterFIWriter
  svtkX3DExporterWriter
  svtkX3DExporterXMLWriter)

svtk_module_add_module(SVTK::IOExport
  CLASSES ${classes})
