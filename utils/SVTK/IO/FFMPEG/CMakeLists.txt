svtk_module_find_package(
  PACKAGE FFMPEG
  VERSION 2.3.3
  FORWARD_VERSION_REQ MINOR
  COMPONENTS
    avformat
    avcodec
    avutil
    swscale)

set(classes
  svtkFFMPEGWriter)

set_source_files_properties(svtkFFMPEGWriter.cxx
  PROPERTIES
    COMPILE_DEFINITIONS __STDC_CONSTANT_MACROS)

# svtkFFMPEGVideoSource requires ffmpeg 3.1 or later.
if (NOT FFMPEG_VERSION VERSION_LESS "3.1")
  list(APPEND classes
    svtkFFMPEGVideoSource)
endif ()

svtk_module_add_module(SVTK::IOFFMPEG
  CLASSES ${classes})
svtk_module_link(SVTK::IOFFMPEG
  PRIVATE
    FFMPEG::avformat
    FFMPEG::avcodec
    FFMPEG::avutil
    FFMPEG::swscale)
