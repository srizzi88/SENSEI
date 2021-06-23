# The TDx support has not been moved, this header is here to define nothing,
# and so allow classes in other modules to safely include it and keep going.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkTDxConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkTDxConfigure.h")

set(classes
  svtkAbstractMapper
  svtkAbstractMapper3D
  svtkAbstractPicker
  svtkAbstractVolumeMapper
  svtkActor
  svtkActor2D
  svtkActor2DCollection
  svtkActorCollection
  svtkAssembly
  svtkAvatar
  svtkBackgroundColorMonitor
  svtkBillboardTextActor3D
  svtkCIEDE2000
  svtkCamera
  svtkCameraActor
  svtkCameraInterpolator
  svtkCellCenterDepthSort
  svtkColorTransferFunction
  svtkCompositeDataDisplayAttributes
  svtkCompositeDataDisplayAttributesLegacy
  svtkCompositePolyDataMapper
  svtkCoordinate
  svtkCuller
  svtkCullerCollection
  svtkDataSetMapper
  svtkDiscretizableColorTransferFunction
  svtkDistanceToCamera
  svtkFXAAOptions
  svtkFlagpoleLabel
  svtkFollower
  svtkFrameBufferObjectBase
  svtkFrustumCoverageCuller
  svtkGPUInfo
  svtkGPUInfoList
  svtkGenericVertexAttributeMapping
  svtkGlyph3DMapper
  svtkGraphMapper
  svtkGraphToGlyphs
  svtkGraphicsFactory
  svtkHardwareSelector
  svtkHardwareWindow
  svtkHierarchicalPolyDataMapper
  svtkImageActor
  svtkImageMapper
  svtkImageMapper3D
  svtkImageProperty
  svtkImageSlice
  svtkImageSliceMapper
  svtkInteractorEventRecorder
  svtkInteractorObserver
  svtkLabeledContourMapper
  svtkLight
  svtkLightActor
  svtkLightCollection
  svtkLightKit
  svtkLogLookupTable
  svtkLookupTableWithEnabling
  svtkMapArrayValues
  svtkMapper
  svtkMapper2D
  svtkMapperCollection
  svtkObserverMediator
  svtkPointGaussianMapper
  svtkPolyDataMapper
  svtkPolyDataMapper2D
  svtkProp
  svtkProp3D
  svtkProp3DCollection
  svtkProp3DFollower
  svtkPropAssembly
  svtkPropCollection
  svtkProperty
  svtkProperty2D
  svtkRenderPass
  svtkRenderState
  svtkRenderTimerLog
  svtkRenderWindow
  svtkRenderWindowCollection
  svtkRenderWindowInteractor
  svtkRenderWindowInteractor3D
  svtkRenderer
  svtkRendererCollection
  svtkRendererDelegate
  svtkRendererSource
  svtkSelectVisiblePoints
  svtkShaderProperty
  svtkSkybox
  svtkStereoCompositor
  svtkTextActor
  svtkTextActor3D
  svtkTexture
  svtkTexturedActor2D
  svtkTransformCoordinateSystems
  svtkTransformInterpolator
  svtkTupleInterpolator
  svtkUniforms
  svtkViewDependentErrorMetric
  svtkViewport
  svtkVisibilitySort
  svtkVolume
  svtkVolumeCollection
  svtkVolumeProperty
  svtkWindowLevelLookupTable
  svtkWindowToImageFilter

  # Needed by svtkAssembly
  svtkAssemblyNode
  svtkAssemblyPath
  svtkAssemblyPaths

  # Needed by svtkRenderer
  svtkAreaPicker
  svtkPicker

  # Needed by svtkAreaPicker
  svtkAbstractPropPicker
  svtkLODProp3D

  # Needed by svtkRenderWindowInteractor
  svtkPropPicker
  svtkPickingManager

  # Needed by svtkPropPicker
  svtkWorldPointPicker

  # Needed for the InteractorStyleSwitch...
  svtkCellPicker
  svtkPointPicker
  svtkRenderedAreaPicker
  svtkScenePicker

  # Needed by svtkRenderWindowInteractor, for the default interactor.
  svtkInteractorStyle
  svtkInteractorStyle3D
  svtkInteractorStyleSwitchBase

  # svtkTDx stuff dragged in by svtkInteractorStyle
  svtkTDxInteractorStyle
  svtkTDxInteractorStyleCamera
  svtkTDxInteractorStyleSettings

  svtkStringToImage
  svtkTextMapper
  svtkTextProperty
  svtkTextPropertyCollection
  svtkTextRenderer

  # Some of the new rendering code - may move around in the future.
  svtkAbstractInteractionDevice
  svtkAbstractRenderDevice
  svtkRenderWidget)

# needed as we do not have svtkRenderingOpenGLConfigure.h here
set_source_files_properties(
  svtkRenderWindow.cxx  # dummy out painterdevice adapter
  svtkHardwareSelector.cxx # dummy out painter device adapter
  PROPERTIES
    COMPILE_DEFINITIONS SVTK_OPENGL2)

set(headers
  svtkGPUInfoListArray.h
  svtkNoise200x200.h
  svtkPythagoreanQuadruples.h
  svtkRayCastStructures.h
  svtkRenderingCoreEnums.h
  "${CMAKE_CURRENT_BINARY_DIR}/svtkTDxConfigure.h"
  svtkTDxMotionEventInfo.h)

svtk_module_add_module(SVTK::RenderingCore
  CLASSES ${classes}
  HEADERS ${headers}
  )

if (SVTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  svtk_module_definitions(SVTK::RenderingCore
    PRIVATE
      SVTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
endif()
