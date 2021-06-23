svtk_module_test_data(
  Data/CuspySurface.vtp
  Data/Delaunay/,REGEX:.*
  Data/beach.jpg
  Data/can.ex2
  Data/cow.vtp
  Data/delaunay3d.vtu
  Data/disk_out_ref_surface.vtp
  Data/explicitStructuredGrid.vtu
  Data/tex_debug.jpg
  Data/wavelet300Arrays.vti)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/3GQP.pdb
    Data/AMR/Enzo/DD0010/,REGEX:.*
    Data/B.pgm
    Data/RectGrid2.svtk
    Data/Viewpoint/cow.g
    Data/Viewpoint/cow.obj
    Data/beach.tif
    Data/blow.svtk
    Data/combq.bin
    Data/combxyz.bin
    Data/cth.vtr
    Data/cuttertest.vtu
    Data/disk_out_ref.ex2
    Data/financial.txt
    Data/financial.svtk
    Data/fran_cut.png
    Data/fran_cut.svtk
    Data/fullhead15.png
    Data/headsq/,REGEX:.*
    Data/masonry.bmp
    Data/plate.svtk
    Data/polyEx.svtk
    Data/quadTetEdgeTest.svtk
    Data/teapot.g
    Data/tensors.svtk)

  add_subdirectory(Python)
endif ()
