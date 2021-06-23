"""
svtkImageImportFromArray: a NumPy front-end to svtkImageImport

Load a python array into a svtk image.
To use this class, you must have NumPy installed (http://numpy.scipy.org/)

Methods:

  SetArray()  -- set the numpy array to load
  Update()    -- generate the output
  GetOutput() -- get the image as svtkImageData
  GetOutputPort() -- connect to SVTK pipeline

Methods from svtkImageImport:
(if you don't set these, sensible defaults will be used)

  SetDataExtent()
  SetDataSpacing()
  SetDataOrigin()
"""

from svtkmodules.svtkIOImage import svtkImageImport
from svtkmodules.svtkCommonCore import SVTK_SIGNED_CHAR
from svtkmodules.svtkCommonCore import SVTK_UNSIGNED_CHAR
from svtkmodules.svtkCommonCore import SVTK_SHORT
from svtkmodules.svtkCommonCore import SVTK_UNSIGNED_SHORT
from svtkmodules.svtkCommonCore import SVTK_INT
from svtkmodules.svtkCommonCore import SVTK_UNSIGNED_INT
from svtkmodules.svtkCommonCore import SVTK_LONG
from svtkmodules.svtkCommonCore import SVTK_UNSIGNED_LONG
from svtkmodules.svtkCommonCore import SVTK_FLOAT
from svtkmodules.svtkCommonCore import SVTK_DOUBLE

class svtkImageImportFromArray:
    def __init__(self):
        self.__import = svtkImageImport()
        self.__ConvertIntToUnsignedShort = False
        self.__Array = None

    # type dictionary: note that python doesn't support
    # unsigned integers properly!
    __typeDict = {'b':SVTK_SIGNED_CHAR,     # int8
                  'B':SVTK_UNSIGNED_CHAR,   # uint8
                  'h':SVTK_SHORT,           # int16
                  'H':SVTK_UNSIGNED_SHORT,  # uint16
                  'i':SVTK_INT,             # int32
                  'I':SVTK_UNSIGNED_INT,    # uint32
                  'f':SVTK_FLOAT,           # float32
                  'd':SVTK_DOUBLE,          # float64
                  'F':SVTK_FLOAT,           # float32
                  'D':SVTK_DOUBLE,          # float64
                  }

    __sizeDict = { SVTK_SIGNED_CHAR:1,
                   SVTK_UNSIGNED_CHAR:1,
                   SVTK_SHORT:2,
                   SVTK_UNSIGNED_SHORT:2,
                   SVTK_INT:4,
                   SVTK_UNSIGNED_INT:4,
                   SVTK_FLOAT:4,
                   SVTK_DOUBLE:8 }

    # convert 'Int32' to 'unsigned short'
    def SetConvertIntToUnsignedShort(self,yesno):
        self.__ConvertIntToUnsignedShort = yesno

    def GetConvertIntToUnsignedShort(self):
        return self.__ConvertIntToUnsignedShort

    def ConvertIntToUnsignedShortOn(self):
        self.__ConvertIntToUnsignedShort = True

    def ConvertIntToUnsignedShortOff(self):
        self.__ConvertIntToUnsignedShort = False

    def Update(self):
        self.__import.Update()

    # get the output
    def GetOutputPort(self):
        return self.__import.GetOutputPort()

    # get the output
    def GetOutput(self):
        return self.__import.GetOutput()

    # import an array
    def SetArray(self,imArray):
        self.__Array = imArray
        numComponents = 1
        dim = imArray.shape
        if len(dim) == 0:
            dim = (1,1,1)
        elif len(dim) == 1:
            dim = (1, 1, dim[0])
        elif len(dim) == 2:
            dim = (1, dim[0], dim[1])
        elif len(dim) == 4:
            numComponents = dim[3]
            dim = (dim[0],dim[1],dim[2])

        typecode = imArray.dtype.char

        ar_type = self.__typeDict[typecode]

        complexComponents = 1
        if (typecode == 'F' or typecode == 'D'):
            numComponents = numComponents * 2
            complexComponents = 2

        if (self.__ConvertIntToUnsignedShort and typecode == 'i'):
            imArray = imArray.astype('h')
            ar_type = SVTK_UNSIGNED_SHORT

        size = len(imArray.flat)*self.__sizeDict[ar_type]*complexComponents
        self.__import.CopyImportVoidPointer(imArray, size)
        self.__import.SetDataScalarType(ar_type)
        self.__import.SetNumberOfScalarComponents(numComponents)
        extent = self.__import.GetDataExtent()
        self.__import.SetDataExtent(extent[0],extent[0]+dim[2]-1,
                                    extent[2],extent[2]+dim[1]-1,
                                    extent[4],extent[4]+dim[0]-1)
        self.__import.SetWholeExtent(extent[0],extent[0]+dim[2]-1,
                                     extent[2],extent[2]+dim[1]-1,
                                     extent[4],extent[4]+dim[0]-1)

    def GetArray(self):
        return self.__Array

    # a whole bunch of methods copied from svtkImageImport

    def SetDataExtent(self,extent):
        self.__import.SetDataExtent(extent)

    def GetDataExtent(self):
        return self.__import.GetDataExtent()

    def SetDataSpacing(self,spacing):
        self.__import.SetDataSpacing(spacing)

    def GetDataSpacing(self):
        return self.__import.GetDataSpacing()

    def SetDataOrigin(self,origin):
        self.__import.SetDataOrigin(origin)

    def GetDataOrigin(self):
        return self.__import.GetDataOrigin()
