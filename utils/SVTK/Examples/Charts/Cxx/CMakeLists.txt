cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(Charts)

find_package(SVTK
  COMPONENTS
    ChartsCore
  OPTIONAL_COMPONENTS
    CommonSystem
    GUISupportQt
    InfovisCore
    RenderingOpenGL2
    ViewsInfovis
    ViewsContext2D
    ViewsQt)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

if (TARGET SVTK::InfovisCore AND TARGET SVTK::ViewsInfovis AND TARGET SVTK::ViewsContext2D)
  add_executable(GraphItem
    GraphItem.cxx
    svtkGraphItem.cxx
    svtkGraphItem.h)
  target_link_libraries(GraphItem
    PRIVATE
      SVTK::ChartsCore
      SVTK::InfovisCore
      SVTK::ViewsInfovis
      SVTK::ViewsContext2D)
  svtk_module_autoinit(
    TARGETS GraphItem
    MODULES SVTK::ChartsCore
            SVTK::InfovisCore
            SVTK::ViewsInfovis
            SVTK::ViewsContext2D)
endif ()

if (TARGET SVTK::ViewsContext2D)
  add_executable(PiecewiseItem
    PiecewiseItem.cxx)
  target_link_libraries(PiecewiseItem
    PRIVATE
      SVTK::ChartsCore
      SVTK::ViewsContext2D)
  svtk_module_autoinit(
    TARGETS PiecewiseItem
    MODULES SVTK::ChartsCore
            SVTK::ViewsContext2D)
endif ()

if (TARGET SVTK::CommonSystem AND TARGET SVTK::GUISupportQt AND TARGET SVTK::ViewsContext2D AND TARGET SVTK::ViewsQt)
  add_executable(QChartTable
    QChartTable.cxx)
  target_link_libraries(QChartTable
    PRIVATE
      SVTK::ChartsCore
      SVTK::CommonSystem
      SVTK::GUISupportQt
      SVTK::ViewsContext2D
      SVTK::ViewsQt)

  add_executable(QScalarsToColors
    QScalarsToColors.cxx)
  target_link_libraries(QScalarsToColors
    PRIVATE
      SVTK::ChartsCore
      SVTK::CommonSystem
      SVTK::GUISupportQt
      SVTK::ViewsContext2D
      SVTK::ViewsQt)

  svtk_module_autoinit(
    TARGETS QChartTable
            QScalarsToColors
    MODULES SVTK::ChartsCore
            SVTK::CommonSystem
            SVTK::GUISupportQt
            SVTK::ViewsContext2D
            SVTK::ViewsQt)
endif ()
