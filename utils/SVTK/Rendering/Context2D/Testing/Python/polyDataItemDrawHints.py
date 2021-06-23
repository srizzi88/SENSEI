#!/usr/bin/env python
import sys
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


def affine(val, inmin, inmax, outmin, outmax):
    return (((val - inmin) / (inmax - inmin)) * (outmax - outmin)) + outmin


def buildSpiralPolyDataItem(drawOpts):
    xmin, xmax, ymin, ymax = drawOpts['vp']
    color = drawOpts['color']
    lineWidth = drawOpts['lineWidth']
    stippleType = drawOpts['stippleType']

    spiralPoints = [
        [0.1, 0.1],
        [0.1, 0.9],
        [0.9, 0.9],
        [0.9, 0.2],

        [0.2, 0.2],
        [0.2, 0.8],
        [0.8, 0.8],
        [0.8, 0.3],

        [0.3, 0.3],
        [0.3, 0.7],
        [0.7, 0.7],
        [0.7, 0.4],

        [0.4, 0.4],
        [0.4, 0.6],
        [0.6, 0.6],
        [0.6, 0.5]
    ]

    xformedPoints = [[
        affine(p[0], 0.0, 1.0, xmin, xmax),
        affine(p[1], 0.0, 1.0, ymin, ymax),
        0.0
    ] for p in spiralPoints]

    pd = svtk.svtkPolyData()
    pd.Allocate(50)

    pts = svtk.svtkPoints()

    for pt in xformedPoints:
        pts.InsertNextPoint(pt)

    pd.SetPoints(pts)

    numpts = len(xformedPoints)
    pd.InsertNextCell(svtk.SVTK_POLY_LINE, numpts, [idx for idx in range(numpts)])

    colors = svtk.svtkUnsignedCharArray()
    colors.SetNumberOfComponents(4)
    colors.SetNumberOfTuples(1)

    colors.SetTuple4(0, color[0], color[1], color[2], 255)

    # Specify line width
    floatValue = svtk.svtkFloatArray()
    floatValue.SetNumberOfComponents(1)
    floatValue.SetName("LineWidth")
    floatValue.InsertNextValue(lineWidth)
    pd.GetFieldData().AddArray(floatValue)

    # Specify stipple pattern
    intValue = svtk.svtkIntArray()
    intValue.SetNumberOfComponents(1)
    intValue.SetName("StippleType")
    intValue.InsertNextValue(stippleType)
    pd.GetFieldData().AddArray(intValue)

    item = svtk.svtkPolyDataItem()
    item.SetPolyData(pd)
    item.SetMappedColors(colors)
    item.SetScalarMode(svtk.SVTK_SCALAR_MODE_USE_CELL_DATA)
    item.SetVisible(True)

    return item


polyDataItemList = [
    buildSpiralPolyDataItem({
        'vp': [0.0, 0.5, 0.0, 0.5],
        'color': [255, 0, 0],
        'lineWidth': 3.5,
        'stippleType': svtk.svtkPen.DASH_DOT_DOT_LINE
    }),
    buildSpiralPolyDataItem({
        'vp': [0.0, 0.5, 0.5, 1.0],
        'color': [0, 255, 0],
        'lineWidth': 2.5,
        'stippleType': svtk.svtkPen.DASH_LINE
    }),
    buildSpiralPolyDataItem({
        'vp': [0.5, 1.0, 0.5, 1.0],
        'color': [0, 0, 255],
        'lineWidth': 1.5,
        'stippleType': svtk.svtkPen.DASH_DOT_LINE
    }),
    buildSpiralPolyDataItem({
        'vp': [0.5, 1.0, 0.0, 0.5],
        'color': [255, 0, 255],
        'lineWidth': 0.5,
        'stippleType': svtk.svtkPen.SOLID_LINE
    })
]

width = 400
height = 400

view = svtk.svtkContextView()
renWin = view.GetRenderWindow()
renWin.SetSize(width, height)

area = svtk.svtkInteractiveArea()
view.GetScene().AddItem(area)

drawAreaBounds = svtk.svtkRectd(0.0, 0.0, 1.0, 1.0)

vp = [0.0, 1.0, 0.0, 1.0]
screenGeometry = svtk.svtkRecti(int(vp[0] * width),
                              int(vp[2] * height),
                              int((vp[1] - vp[0]) * width),
                              int((vp[3] - vp[2]) * height))

for item in polyDataItemList:
    area.GetDrawAreaItem().AddItem(item)

area.SetDrawAreaBounds(drawAreaBounds)
area.SetGeometry(screenGeometry)
area.SetFillViewport(False)
area.SetShowGrid(False)

axisLeft = area.GetAxis(svtk.svtkAxis.LEFT)
axisRight = area.GetAxis(svtk.svtkAxis.RIGHT)
axisBottom = area.GetAxis(svtk.svtkAxis.BOTTOM)
axisTop = area.GetAxis(svtk.svtkAxis.TOP)
axisTop.SetVisible(False)
axisRight.SetVisible(False)
axisLeft.SetVisible(False)
axisBottom.SetVisible(False)
axisTop.SetMargins(0, 0)
axisRight.SetMargins(0, 0)
axisLeft.SetMargins(0, 0)
axisBottom.SetMargins(0, 0)

renWin.Render()

# iren = view.GetInteractor()
# iren.Initialize()
# iren.GetInteractorStyle().SetCurrentRenderer(view.GetRenderer());
# iren.Start()
