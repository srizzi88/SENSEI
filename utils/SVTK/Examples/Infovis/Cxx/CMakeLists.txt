cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(Infovis)

set(svtk_modules
find_package(SVTK
  COMPONENTS
    FiltersSources
    IOInfovis
    InfovisCore
    InfovisLayout
    InteractionStyle
    RenderingCore
    RenderingLabel
    RenderingOpenGL2)
  OPTIONAL_COMPONENTS
    FiltersStatistics
    GUISupportQt
    IOSQL
    ViewsInfovis
    ViewsQt)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(TreeLayout
  TreeLayout.cxx)
target_link_libraries(TreeLayout
  PRIVATE
    ${svtk_modules})

svtk_module_autoinit(
  TARGETS TreeLayout
  MODULES ${svtk_modules})

if (TARGET SVTK::ViewsInfovis)
  add_executable(HelloWorld
    HelloWorld.cxx)
  target_link_libraries(HelloWorld
    PRIVATE
      ${svtk_modules}
      svtkViewsInfovis)

  add_executable(Theme
    Theme.cxx)
  target_link_libraries(Theme
    PRIVATE
      ${svtk_modules}
      svtkViewsInfovis)

  add_executable(CreateTree
    CreateTree.cxx)
  target_link_libraries(CreateTree
    PRIVATE
      ${svtk_modules}
      svtkViewsInfovis)

  add_executable(MultiView
    MultiView.cxx)
  target_link_libraries(MultiView
    PRIVATE
      ${svtk_modules}
      svtkViewsInfovis)

  svtk_module_autoinit(
    TARGETS HelloWorld
            Theme
            CreateTree
            MultiView
    MODULES ${svtk_modules}
            svtkViewsInfovis)
endif ()

# If Qt is enabled include the Qt based examples
if (TARGET SVTK::GUISupportQt AND TARGET SVTK::ViewsQt)
  add_subdirectory(EasyView)
  add_subdirectory(CustomLinkView)
  if (TARGET SVTK::IOSQL AND TARGET SVTK::FiltersStatistics)
    add_subdirectory(StatsView)
  endif ()
endif ()
