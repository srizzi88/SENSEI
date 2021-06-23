# Module Migration

SVTK 8.2 and older contained a module system which was based on variables and
informed CMake's migration to target-based properties and interactions. This
was incompatible with the way SVTK ended up doing it. With SVTK 9, its module
system has been reworked to use CMake's targets.

This document may be used as a guide to updating code using old SVTK modules into
code using new SVTK modules.

## Using modules

If your project is just using SVTK's modules and not declaring any of your own
modules, porting involves a few changes to the way SVTK is found and used.

The old module system made variables available for using SVTK.

~~~{.cmake}
find_package(SVTK
  REQUIRED
  COMPONENTS
    svtkCommonCore
    svtkRenderingOpenGL2)
include(${SVTK_USE_FILE})

add_library(usessvtk ...)
target_link_libraries(usessvtk ${visibility} ${SVTK_LIBRARIES})
target_include_directories(usessvtk ${visibility} ${SVTK_INCLUDE_DIRS})

# Pass any SVTK autoinit defines to the target.
target_compile_definitions(usessvtk PRIVATE ${SVTK_DEFINITIONS})
~~~

This causes problems if SVTK is found multiple times within a source tree with
different components. The new pattern is:

~~~{.cmake}
find_package(SVTK
  #9.0 # Compatibility support is not provided if 9.0 is requested.
  REQUIRED
  COMPONENTS
    # Old component names are OK, but deprecated.
    #svtkCommonCore
    #svtkRenderingOpenGL2
    # New names reflect the target names in use.
    CommonCore
    RenderingOpenGL2)
# No longer needed; warns or errors depending on the version requested when
# finding SVTK.
#include(${SVTK_USE_FILE})

add_library(usessvtk ...)
# SVTK_LIBRARIES is provided for compatibility, but not recommended.
#target_link_libraries(usessvtk ${visibility} ${SVTK_LIBRARIES})
target_link_libraries(usessvtk ${visibility} SVTK::CommonCore SVTK::RenderingOpenGL2)

# Rather than defining a single `SVTK_DEFINITIONS` for use by all relevant
# targets, the definitions are made as needed with the exact set needed for the
# listed modules.
svtk_module_autoinit(
  TARGETS usessvtk
  #MODULES ${SVTK_LIBRARIES} # Again, works, but is not recommended.
  MODULES SVTK::CommonCore SVTK::RenderingOpenGL2)
~~~

## Module declaration

The old module system had CMake code declare modules in `module.cmake` files.
This allowed logic and other things to happen within them which could cause
module dependencies to be hard to follow. The new module system now provides
facilities for disabling modules in certain configurations (using `CONDITION`)
and for optionally depending on modules (using `OPTIONAL_DEPENDS`).

~~~{.cmake}
if (NOT SOME_OPTION)
  set(depends)
  if (SOME_OTHER_OPTION)
    list(APPEND depends svtkSomeDep)
  endif ()
  svtk_module(svtkModuleName
    GROUPS
      # groups the module belongs to
    KIT
      # the kit the module belongs to
    IMPLEMENTS
      # modules containing svtkObjectFactory instances that are implemented here
    DEPENDS
      # public dependencies
      #${depends} # no analogy in the new system
    PRIVATE_DEPENDS
      # private dependencies
      ${depends}
    COMPILE_DEPENDS
      # modules which must be built before this one but which are not actually
      # linked.
    TEST_DEPENDS
      # test dependencies
    TEST_OPTIONAL_DEPENDS
      # optional test dependencies
      ${depends}
    #EXCLUDE_FROM_WRAPPING
      # present for modules which cannot be wrapped
  )
endif ()
~~~

This is now replaced with a declarative file named `svtk.module`. This file is
not CMake code and is instead parsed as an argument list in CMake (variable
expansions are also not allowed). The above example would translate into:

~~~{.cmake}
MODULE
  svtkModuleName
CONDITION
  SOME_OPTION
GROUPS
  # groups the module belongs to
KIT
  # the kit the module belongs to
#IMPLEMENTABLE # Implicit in the old build system. Now explicit.
IMPLEMENTS
  # modules containing svtkObjectFactory instances that are implemented here
DEPENDS
  # public dependencies
PRIVATE_DEPENDS
  # private dependencies
OPTIONAL_DEPENDS
  svtkSomeDep
ORDER_DEPENDS
  # modules which must be built before this one but which are not actually
  # linked.
TEST_DEPENDS
  # test dependencies
TEST_OPTIONAL_DEPENDS
  # optional test dependencies
  svtkSomeDep
#EXCLUDE_WRAP
  # present for modules which cannot be wrapped
~~~

Modules may also now be provided by the current project or by an external
project found by `find_package` as well.

## Declaring sources

Sources used to be listed just as `.cxx` files. The module system would then
search for a corresponding `.h` file, then add it to the list. Some source file
properties could be used to control header-only or private headers.

In this example, we have a module with the following sources:

  - `svtkPublicClass.cxx` and `svtkPublicClass.h`: Public SVTK class meant to be
    wrapped and its header installed.
  - `svtkPrivateClass.cxx` and `svtkPrivateClass.h`: Priavte SVTK class not meant
    for use outside of the module.
  - `helper.cpp` and `helper.h`: Private API, but not following SVTK's naming
    conventions.
  - `public_helper.cpp` and `public_helper.h`: Public API, but not following
    SVTK's naming conventions.
  - `svtkImplSource.cxx`: A source file without a header.
  - `public_header.h`: A public header without a source file.
  - `template.tcc` and `template.h`: Public API, but not following SVTK's naming
    conventions.
  - `private_template.tcc` and `private_template.h`: Private API, but not
    following SVTK's naming conventions.
  - `svtkPublicTemplate.txx` and `svtkPublicTemplate.h`: Public template sources.
    Wrapped and installed.
  - `svtkPrivateTemplate.txx` and `svtkPrivateTemplate.h`: Private template
    sources.
  - `svtkOptional.cxx` and `svtkOptional.h`: Private API which requires an
    optional dependency.

The old module's way of building these sources is:

~~~{.cmake}
set(Module_SRCS
  svtkPublicClass.cxx
  svtkPrivateClass.cxx
  helper.cpp
  helper.h
  public_helper.cpp
  public_helper.h
  public_header.h
  svtkImplSource.cxx
  svtkPublicTemplate.txx
  svtkPrivateTemplate.txx
  template.tcc # Not detected as a template, so not installed.
  template.h
  private_template.tcc
  private_template.h
)

# Mark some files as only being header files.
set_source_files_properties(
  public_header.h
  HEADER_FILE_ONLY
)

# Mark some headers as being private.
set_source_files_properties(
  helper.h
  private_template.h
  public_header.h
  template.h
  svtkImplSource.cxx # no header
  svtkPrivateTemplate.h
  PROPERTIES SKIP_HEADER_INSTALL 1
)

set(${svtk-module}_HDRS # Magic variable
  public_helper.h
  template.h
  #helper.h # private headers just go ignored.
)

# Optional dependencies are detected through variables.
if (Module_svtkSomeDep)
  list(APPEND Module_SRCS
    # Some optional file.
    svtkOptional.cxx)
endif ()

svtk_module_library(svtkModuleName ${Module_SRCS})
~~~

While with the new system, source files are explicitly declared using argument
parsing.

~~~{.cmake}
set(classes
  svtkPublicClass)
set(private_classes
  svtkPrivateClass)
set(sources
  helper.cpp
  public_helper.cpp
  svtkImplSource.cxx)
set(headers
  public_header.h
  public_helper.h
  template.h)
set(private_headers
  helper.h
  private_template.h)

set(template_classes
  svtkPublicTemplate)
set(private_template_classes
  svtkPrivateTemplate)
set(templates
  template.tcc)
set(private_templates
  private_template.tcc)

# Optional dependencies are detected as targets.
if (TARGET svtkSomeDep)
  # Optional classes may not be public (though there's no way to actually
  # enforce it, optional dependencies are always treated as private.
  list(APPEND private_classes
    svtkOptional)
endif ()

svtk_module_add_module(svtkModuleName
  # File pairs which follow SVTK's conventions. The headers will be wrapped and
  # installed.
  CLASSES ${classes}
  # File pairs which follow SVTK's conventions, but are not for use outside the
  # module.
  PRIVATE_CLASSES ${private_classes}
  # Standalone sources (those without headers or which do not follow SVTK's
  # conventions).
  SOURCES ${sources}
  # Standalone headers (those without sources or which do not follow SVTK's
  # conventions). These will be installed.
  HEADERS ${public_headers}
  # Standalone headers (those without sources or which do not follow SVTK's
  # conventions), but are not for use outside the module.
  PRIVATE_HEADERS ${private_headers}

  # Templates are also supported.

  # Template file pairs which follow SVTK's conventions. Both files will be
  # installed (only the headers will be wrapped).
  TEMPLATE_CLASSES ${template_classes}
  # Template file pairs which follow SVTK's conventions, but are not for use
  # outside the module.
  PRIVATE_TEMPLATE_CLASSES ${private_template_classes}
  # Standalone template files (those without headers or which do not follow
  # SVTK's conventions). These will be installed.
  TEMPLATES ${templates}
  # Standalone template files (those without headers or which do not follow
  # SVTK's conventions), but are not for use outside the module.
  PRIVATE_TEMPLATES ${private_templates}
)
~~~

Note that the arguments with `CLASSES` in their name expand to pairs of files
with the `.h` and either `.cxx` or `.txx` extension based on whether it is a
template or not. Projects not using this convention may use the `HEADERS`,
`SOURCES`, and `TEMPLATES` arguments instead.

## Object Factories

Previously, object factories were made using implicit variable declaration magic
behind the scenes. This is no longer the case and proper CMake APIs for them are
available.

~~~{.cmake}
set(sources
  svtkObjectFactoryImpl.cxx
  # This path is made by `svtk_object_factory_configure` later.
  "${CMAKE_CURRENT_BINARY_DIR}/${svtk-module}ObjectFactory.cxx")

# Make a list of base classes we will be overriding.
set(overrides svtkObjectFactoryBase)
# Make a variable declaring what the override for the class is.
set(svtk_module_svtkObjectFactoryBase_override "svtkObjectFactoryImpl")
# Generate a source using the list of base classes overridden.
svtk_object_factory_configure("${overrides}")

svtk_module_library("${svtk-module}" "${sources}")
~~~

This is now handled using proper APIs instead of variable lookups.

~~~{.cmake}
set(classes
  svtkObjectFactoryImpl)

# Explicitly declare the override relationship.
svtk_object_factory_declare(
  BASE      svtkObjectFactoryBase
  OVERRIDE  svtkObjectFactoryImpl)
# Collects the set of declared overrides and writes out a source file.
svtk_object_factory_declare(
  # The path to the source is returned as a variable.
  SOURCE_FILE factory_source
  # As is its header file.
  HEADER_FILE factory_header
  # The export macro is now explicitly passed (instead of assumed based on the
  # current module context).
  EXPORT_MACRO MODULE_EXPORT)

svtk_module_add_module(svtkModuleName
  CLASSES ${classes}
  SOURCES "${factory_source}"
  PRIVATE_HEADERS "${factory_header}")
~~~

## Building a group of modules

This was not well supported in the old module system. Basically, it involved
setting up the source tree like SVTK expects and then including the
`svtkModuleTop` file. This is best just rewritten using the following CMake APIs:

  - [svtk_module_find_modules]
  - [svtk_module_find_kits]
  - [svtk_module_scan]
  - [svtk_module_build]

[svtk_module_find_modules]: @ref svtk_module_find_modules
[svtk_module_find_kits]: @ref svtk_module_find_kits
[svtk_module_scan]: @ref svtk_module_scan
[svtk_module_build]: @ref svtk_module_build
