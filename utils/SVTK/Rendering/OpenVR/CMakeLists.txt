svtk_module_find_package(PACKAGE OpenVR)

set(classes
  svtkOpenGLAvatar
  svtkOpenVRCamera
  svtkOpenVRControlsHelper
  svtkOpenVRDefaultOverlay
  svtkOpenVRFollower
  svtkOpenVRHardwarePicker
  svtkOpenVRInteractorStyle
  svtkOpenVRMenuRepresentation
  svtkOpenVRMenuWidget
  svtkOpenVRModel
  svtkOpenVROverlay
  svtkOpenVRPanelRepresentation
  svtkOpenVRPanelWidget
  svtkOpenVRPropPicker
  svtkOpenVRRay
  svtkOpenVRRenderWindow
  svtkOpenVRRenderWindowInteractor
  svtkOpenVRRenderer)

svtk_encode_string(
  INPUT         "OpenVRDashboard.jpg"
  HEADER_OUTPUT dashboard_header
  SOURCE_OUTPUT dashboard_source
  BINARY)

set(geometry_sources)
set(geometry_headers)

# Generates headers with char arrays that contain the model file contents.
set(geometry_files
  data/svtkAvatarHead.vtp
  data/svtkAvatarLeftForeArm.vtp
  data/svtkAvatarLeftHand.vtp
  data/svtkAvatarLeftUpperArm.vtp
  data/svtkAvatarRightForeArm.vtp
  data/svtkAvatarRightHand.vtp
  data/svtkAvatarRightUpperArm.vtp
  data/svtkAvatarTorso.vtp)
foreach (geometry_file IN LISTS geometry_files)
  svtk_encode_string(
    INPUT         "${geometry_file}"
    EXPORT_SYMBOL "SVTKRENDERINGOPENVR_EXPORT"
    EXPORT_HEADER "svtkRenderingOpenVRModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
    BINARY
    NUL_TERMINATE)
  list(APPEND geometry_sources "${source}")
  list(APPEND geometry_headers "${header}")
endforeach ()

svtk_object_factory_declare(
  BASE      svtkAvatar
  OVERRIDE  svtkOpenGLAvatar)
svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGOPENVR_EXPORT")

svtk_module_add_module(SVTK::RenderingOpenVR
  CLASSES ${classes}
  SOURCES "${dashboard_source}" ${svtk_object_factory_source} ${geometry_sources}
  PRIVATE_HEADERS "${dashboard_header}" ${svtk_object_factory_header} ${geometry_headers})
svtk_module_link(SVTK::RenderingOpenVR
  PUBLIC
    OpenVR::OpenVR)
