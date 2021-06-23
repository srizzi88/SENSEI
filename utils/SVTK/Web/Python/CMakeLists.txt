set(files
  svtkmodules/web/__init__.py
  svtkmodules/web/camera.py
  svtkmodules/web/dataset_builder.py
  svtkmodules/web/protocols.py
  svtkmodules/web/query_data_model.py
  svtkmodules/web/render_window_serializer.py
  svtkmodules/web/testing.py
  svtkmodules/web/svtkjs_helper.py
  svtkmodules/web/wslink.py)

svtk_module_add_python_package(SVTK::WebPython
  FILES               ${files}
  PACKAGE             "svtkmodules.web"
  MODULE_DESTINATION  "${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}")

svtk_module_add_python_module(SVTK::WebPython
  PACKAGES "svtkmodules.web")

set_property(GLOBAL APPEND
  PROPERTY
    svtk_required_python_modules "wslink>=0.1.3")
set_property(GLOBAL APPEND
  PROPERTY
    svtk_required_python_modules "Twisted>=17.5.0")
set_property(GLOBAL APPEND
  PROPERTY
    svtk_required_python_modules "autobahn>=17.7.1")
