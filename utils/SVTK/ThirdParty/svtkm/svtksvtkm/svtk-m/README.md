# SVTK-m #

SVTK-m is a toolkit of scientific visualization algorithms for emerging
processor architectures. SVTK-m supports the fine-grained concurrency for
data analysis and visualization algorithms required to drive extreme scale
computing by providing abstract models for data and execution that can be
applied to a variety of algorithms across many different processor
architectures.

You can find out more about the design of SVTK-m on the [SVTK-m Wiki].


## Learning Resources ##

  + A high-level overview is given in the IEEE Vis talk "[SVTK-m:
    Accelerating the Visualization Toolkit for Massively Threaded
    Architectures][SVTK-m Overview]."

  + The [SVTK-m Users Guide] provides extensive documentation. It is broken
    into multiple parts for learning and references at multiple different
    levels.
      + "Part 1: Getting Started" provides the introductory instruction for
        building SVTK-m and using its high-level features.
      + "Part 2: Using SVTK-m" covers the core fundamental components of
        SVTK-m including data model, worklets, and filters.
      + "Part 3: Developing with SVTK-m" covers how to develop new worklets
        and filters.
      + "Part 4: Advanced Development" covers topics such as new worklet
        types and custom device adapters.

  + Community discussion takes place on the [SVTK-m users email list].

  + Doxygen-generated nightly reference documentation is available
    [online][SVTK-m Doxygen].


## Contributing ##

There are many ways to contribute to [SVTK-m], with varying levels of
effort.

  + Ask a question on the [SVTK-m users email list].

  + Submit new or add to discussions of a feature requests or bugs on the
    [SVTK-m Issue Tracker].

  + Submit a Pull Request to improve [SVTK-m]
      + See [CONTRIBUTING.md] for detailed instructions on how to create a
        Pull Request.
      + See the [SVTK-m Coding Conventions] that must be followed for
        contributed code.

  + Submit an Issue or Pull Request for the [SVTK-m Users Guide]


## Dependencies ##

SVTK-m Requires:

  + C++11 Compiler. SVTK-m has been confirmed to work with the following
      + GCC 4.8+
      + Clang 5.0+
      + XCode 5.0+
      + MSVC 2015+
      + Intel 17.0.4+
  + [CMake](http://www.cmake.org/download/)
      + CMake 3.8+
      + CMake 3.11+ (for Visual Studio generator)
      + CMake 3.12+ (for OpenMP support)
      + CMake 3.13+ (for CUDA support)

Optional dependencies are:

  + CUDA Device Adapter
      + [Cuda Toolkit 9.2+](https://developer.nvidia.com/cuda-toolkit)
      + Note CUDA >= 10.1 is required on Windows
  + TBB Device Adapter
      + [TBB](https://www.threadingbuildingblocks.org/)
  + OpenMP Device Adapter
      + Requires a compiler that supports OpenMP >= 4.0.
  + OpenGL Rendering
      + The rendering module contains multiple rendering implementations
        including standalone rendering code. The rendering module also
        includes (optionally built) OpenGL rendering classes.
      + The OpenGL rendering classes require that you have a extension
        binding library and one rendering library. A windowing library is
        not needed except for some optional tests.
  + Extension Binding
      + [GLEW](http://glew.sourceforge.net/)
  + On Screen Rendering
      + OpenGL Driver
      + Mesa Driver
  + On Screen Rendering Tests
      + [GLFW](http://www.glfw.org/)
      + [GLUT](http://freeglut.sourceforge.net/)
  + Headless Rendering
      + [OS Mesa](https://www.mesa3d.org/osmesa.html)
      + EGL Driver

SVTK-m has been tested on the following configurations:c
  + On Linux
      + GCC 4.8.5, 5.4.0, 6.4.0, 7.3.0, Clang 5.0, 6.0, 7.0, Intel 17.0.4, Intel 19.0.0
      + CMake 3.13.3, 3.14.1
      + CUDA 9.2.148, 10.0.130, 10.1.105
      + TBB 4.4 U2, 2017 U7
  + On Windows
      + Visual Studio 2015, 2017
      + CMake 3.8.2, 3.11.1, 3.12.4
      + CUDA 10.1
      + TBB 2017 U3, 2018 U2
  + On MacOS
      + AppleClang 9.1
      + CMake 3.12.3
      + TBB 2018


## Building ##

SVTK-m supports all majors platforms (Windows, Linux, OSX), and uses CMake
to generate all the build rules for the project. The SVTK-m source code is
available from the [SVTK-m download page] or by directly cloning the [SVTK-m
git repository].

The basic procedure for building SVTK-m is to unpack the source, create a
build directory, run CMake in that build directory (pointing to the source)
and then build. Here are some example *nix commands for the process
(individual commands may vary).

```sh
$ tar xvzf ~/Downloads/svtk-m-v1.4.0.tar.gz
$ mkdir svtkm-build
$ cd svtkm-build
$ cmake-gui ../svtk-m-v1.4.0
$ cmake --build -j .              # Runs make (or other build program)
```

A more detailed description of building SVTK-m is available in the [SVTK-m
Users Guide].


## Example ##

The SVTK-m source distribution includes a number of examples. The goal of the
SVTK-m examples is to illustrate specific SVTK-m concepts in a consistent and
simple format. However, these examples only cover a small part of the
capabilities of SVTK-m.

Below is a simple example of using SVTK-m to load a SVTK image file, run the
Marching Cubes algorithm on it, and render the results to an image:

```cpp
svtkm::io::reader::SVTKDataSetReader reader("path/to/svtk_image_file");
svtkm::cont::DataSet inputData = reader.ReadDataSet();
std::string fieldName = "scalars";

svtkm::Range range;
inputData.GetPointField(fieldName).GetRange(&range);
svtkm::Float64 isovalue = range.Center();

// Create an isosurface filter
svtkm::filter::Contour filter;
filter.SetIsoValue(0, isovalue);
filter.SetActiveField(fieldName);
svtkm::cont::DataSet outputData = filter.Execute(inputData);

// compute the bounds and extends of the input data
svtkm::Bounds coordsBounds = inputData.GetCoordinateSystem().GetBounds();
svtkm::Vec<svtkm::Float64,3> totalExtent( coordsBounds.X.Length(),
                                        coordsBounds.Y.Length(),
                                        coordsBounds.Z.Length() );
svtkm::Float64 mag = svtkm::Magnitude(totalExtent);
svtkm::Normalize(totalExtent);

// setup a camera and point it to towards the center of the input data
svtkm::rendering::Camera camera;
camera.ResetToBounds(coordsBounds);

camera.SetLookAt(totalExtent*(mag * .5f));
camera.SetViewUp(svtkm::make_Vec(0.f, 1.f, 0.f));
camera.SetClippingRange(1.f, 100.f);
camera.SetFieldOfView(60.f);
camera.SetPosition(totalExtent*(mag * 2.f));
svtkm::cont::ColorTable colorTable("inferno");

// Create a mapper, canvas and view that will be used to render the scene
svtkm::rendering::Scene scene;
svtkm::rendering::MapperRayTracer mapper;
svtkm::rendering::CanvasRayTracer canvas(512, 512);
svtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);

// Render an image of the output isosurface
scene.AddActor(svtkm::rendering::Actor(outputData.GetCellSet(),
                                      outputData.GetCoordinateSystem(),
                                      outputData.GetField(fieldName),
                                      colorTable));
svtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);
view.Initialize();
view.Paint();
view.SaveAs("demo_output.pnm");
```


## License ##

SVTK-m is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt](LICENSE.txt) for details.


[SVTK-m]:                    https://gitlab.kitware.com/svtk/svtk-m/
[SVTK-m Coding Conventions]: docs/CodingConventions.md
[SVTK-m Doxygen]:            http://m.svtk.org/documentation/
[SVTK-m download page]:      http://m.svtk.org/index.php/SVTK-m_Releases
[SVTK-m git repository]:     https://gitlab.kitware.com/svtk/svtk-m/
[SVTK-m Issue Tracker]:      https://gitlab.kitware.com/svtk/svtk-m/issues
[SVTK-m Overview]:           http://m.svtk.org/images/2/29/SVTKmVis2016.pptx
[SVTK-m Users Guide]:        http://m.svtk.org/images/c/c8/SVTKmUsersGuide.pdf
[SVTK-m users email list]:   http://svtk.org/mailman/listinfo/svtkm
[SVTK-m Wiki]:               http://m.svtk.org/
[CONTRIBUTING.md]:          CONTRIBUTING.md
