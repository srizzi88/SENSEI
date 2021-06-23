set(classes
  QFilterTreeProxyModel
  QSVTKApplication
  QSVTKInteractor
  QSVTKInteractorAdapter
  QSVTKOpenGLNativeWidget
  QSVTKOpenGLStereoWidget
  QSVTKOpenGLWindow
  QSVTKRenderWindowAdapter
  svtkEventQtSlotConnect
  svtkQWidgetRepresentation
  svtkQWidgetTexture
  svtkQWidgetWidget
  svtkQtAbstractModelAdapter
  svtkQtAnnotationLayersModelAdapter
  svtkQtConnection
  svtkQtDebugLeaksModel
  svtkQtDebugLeaksView
  svtkQtTableModelAdapter
  svtkQtTreeModelAdapter)

if (SVTK_USE_X AND SVTK_USE_TDX)
  list(APPEND classes
    svtkTDxQtUnixDevices)
endif ()

set(headers
  QSVTKWin32Header.h)

set(private_headers
  QSVTKInteractorInternal.h)

set(qt5_components
  Widgets)

svtk_module_find_package(
  PACKAGE     Qt5
  VERSION     5.9
  COMPONENTS  ${qt5_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)

set(CMAKE_AUTOMOC 1)

svtk_module_add_module(SVTK::GUISupportQt
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})

svtk_module_link(SVTK::GUISupportQt PUBLIC Qt5::Widgets)
