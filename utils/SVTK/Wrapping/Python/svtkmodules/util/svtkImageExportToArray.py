"""
svtkImageExportToArray - a NumPy front-end to svtkImageExport

This class converts a SVTK image to a numpy array.  The output
array will always have 3 dimensions (or 4, if the image had
multiple scalar components).

To use this class, you must have numpy installed (http://numpy.scipy.org)

Methods

  SetInputConnection(svtkAlgorithmOutput) -- connect to SVTK image pipeline
  SetInputData(svtkImageData) -- set an svtkImageData to export
  GetArray() -- execute pipeline and return a numpy array

Methods from svtkImageExport

  GetDataExtent()
  GetDataSpacing()
  GetDataOrigin()
"""

import numpy
import numpy.core.umath as umath

from svtkmodules.svtkIOImage import svtkImageExport
from svtkmodules.svtkCommonExecutionModel import svtkStreamingDemandDrivenPipeline
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


class svtkImageExportToArray:
    def __init__(self):
        self.__export = svtkImageExport()
        self.__ConvertUnsignedShortToInt = False

    # type dictionary

    __typeDict = { SVTK_SIGNED_CHAR:'b',
                   SVTK_UNSIGNED_CHAR:'B',
                   SVTK_SHORT:'h',
                   SVTK_UNSIGNED_SHORT:'H',
                   SVTK_INT:'i',
                   SVTK_UNSIGNED_INT:'I',
                   SVTK_FLOAT:'f',
                   SVTK_DOUBLE:'d'}

    __sizeDict = { SVTK_SIGNED_CHAR:1,
                   SVTK_UNSIGNED_CHAR:1,
                   SVTK_SHORT:2,
                   SVTK_UNSIGNED_SHORT:2,
                   SVTK_INT:4,
                   SVTK_UNSIGNED_INT:4,
                   SVTK_FLOAT:4,
                   SVTK_DOUBLE:8 }

    # convert unsigned shorts to ints, to avoid sign problems
    def SetConvertUnsignedShortToInt(self,yesno):
        self.__ConvertUnsignedShortToInt = yesno

    def GetConvertUnsignedShortToInt(self):
        return self.__ConvertUnsignedShortToInt

    def ConvertUnsignedShortToIntOn(self):
        self.__ConvertUnsignedShortToInt = True

    def ConvertUnsignedShortToIntOff(self):
        self.__ConvertUnsignedShortToInt = False

    # set the input
    def SetInputConnection(self,input):
        return self.__export.SetInputConnection(input)

    def SetInputData(self,input):
        return self.__export.SetInputData(input)

    def GetInput(self):
        return self.__export.GetInput()

    def GetArray(self):
        self.__export.Update()
        input = self.__export.GetInput()
        extent = input.GetExtent()
        type = input.GetScalarType()
        numComponents = input.GetNumberOfScalarComponents()
        dim = (extent[5]-extent[4]+1,
               extent[3]-extent[2]+1,
               extent[1]-extent[0]+1)
        if (numComponents > 1):
            dim = dim + (numComponents,)

        imArray = numpy.zeros(dim, self.__typeDict[type])
        self.__export.Export(imArray)

        # convert unsigned short to int to avoid sign issues
        if (type == SVTK_UNSIGNED_SHORT and self.__ConvertUnsignedShortToInt):
            imArray = umath.bitwise_and(imArray.astype('i'),0xffff)

        return imArray

    def GetDataExtent(self):
        return self.__export.GetDataExtent()

    def GetDataSpacing(self):
        return self.__export.GetDataSpacing()

    def GetDataOrigin(self):
        return self.__export.GetDataOrigin()
