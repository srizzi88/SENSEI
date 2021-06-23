"""Miscellaneous functions and classes that don't fit into specific
categories."""

import sys, os

def calldata_type(type):
    """set_call_data_type(type) -- convenience decorator to easily set the CallDataType attribute
    for python function used as observer callback.
    For example:

    import svtkmodules.util.calldata_type
    import svtkmodules.util.svtkConstants
    import svtkmodules.svtkCommonCore import svtkCommand, svtkLookupTable

    @calldata_type(svtkConstants.SVTK_STRING)
    def onError(caller, event, calldata):
        print("caller: %s - event: %s - msg: %s" % (caller.GetClassName(), event, calldata))

    lt = svtkLookupTable()
    lt.AddObserver(svtkCommand.ErrorEvent, onError)
    lt.SetTableRange(2,1)
    """
    from svtkmodules import svtkCommonCore
    supported_call_data_types = ['string0', svtkCommonCore.SVTK_STRING,
            svtkCommonCore.SVTK_OBJECT, svtkCommonCore.SVTK_INT,
            svtkCommonCore.SVTK_LONG, svtkCommonCore.SVTK_DOUBLE, svtkCommonCore.SVTK_FLOAT]

    if type not in supported_call_data_types:
        raise TypeError("'%s' is not a supported SVTK call data type. Supported types are: %s" % (type, supported_call_data_types))

    def wrap(f):
        f.CallDataType = type
        return f

    return wrap

#----------------------------------------------------------------------
# the following functions are for the svtk regression testing and examples

def svtkGetDataRoot():
    """svtkGetDataRoot() -- return svtk example data directory"""
    dataRoot = None
    for i, argv in enumerate(sys.argv):
        if argv == '-D' and i+1 < len(sys.argv):
            dataRoot = sys.argv[i+1]

    if dataRoot is None:
        dataRoot = os.environ.get('SVTK_DATA_ROOT', '../../../../SVTKData')

    return dataRoot

def svtkGetTempDir():
    """svtkGetTempDir() -- return svtk testing temp dir"""
    tempDir = None
    for i, argv in enumerate(sys.argv):
        if argv == '-T' and i+1 < len(sys.argv):
            tempDir = sys.argv[i+1]

    if tempDir is None:
        tempDir = '.'

    return tempDir

def svtkRegressionTestImage(renWin):
    """svtkRegressionTestImage(renWin) -- produce regression image for window

    This function writes out a regression .png file for a svtkWindow.
    Does anyone involved in testing care to elaborate?
    """
    from svtkmodules.svtkRenderingCore import svtkWindowToImageFilter
    from svtkmodules.svtkIOImage import svtkPNGReader
    from svtkmodules.svtkImagingCore import svtkImageDifference

    fname = None
    for i, argv in enumerate(sys.argv):
        if argv == '-V' and i+1 < len(sys.argv):
            fname = os.path.join(svtkGetDataRoot(), sys.argv[i+1])

    if fname is None:
        return 2

    else:
        rt_w2if = svtkWindowToImageFilter()
        rt_w2if.SetInput(renWin)

        if not os.path.isfile(fname):
            rt_pngw = svtkPNGWriter()
            rt_pngw.SetFileName(fname)
            rt_pngw.SetInputConnection(rt_w2if.GetOutputPort())
            rt_pngw.Write()
            rt_pngw = None

        rt_png = svtkPNGReader()
        rt_png.SetFileName(fname)

        rt_id = svtkImageDifference()
        rt_id.SetInputConnection(rt_w2if.GetOutputPort())
        rt_id.SetImageConnection(rt_png.GetOutputPort())
        rt_id.Update()

        if rt_id.GetThresholdedError() <= 10:
            return 1
        else:
            sys.stderr.write('Failed image test: %f\n'
                             % rt_id.GetThresholdedError())
            return 0
