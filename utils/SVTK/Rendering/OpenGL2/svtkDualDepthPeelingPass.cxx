/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDualDepthPeelingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDualDepthPeelingPass.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkInformation.h"
#include "svtkInformationKey.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLBufferObject.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLQuadHelper.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkRenderState.h"
#include "svtkRenderTimerLog.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include "svtkTypeTraits.h"

#include <algorithm>

// Define to output details about each peel:
//#define DEBUG_PEEL

// Define to output details about each frame:
//#define DEBUG_FRAME

// Define to render the categorization of the initial volume-prepass pixel:
// - Pixels with no opaque or translucent geometry will be red.
// - Pixels with only opaque geometry will be green.
// - Pixels with only translucent geometry will be blue.
// - Pixels with both opaque and translucent geometry will be purple.
//#define DEBUG_VOLUME_PREPASS_PIXELS

// Recent OSX/ATI drivers perform some out-of-order execution that's causing
// the dFdx/dFdy calls to be conditionally executed. Specifically, it looks
// like the early returns when the depth is not on a current peel layer
// (Peeling pass, SVTK::PreColor::Impl hook) are moved before the dFdx/dFdy
// calls used to compute normals. Disable the early returns on apple for now, I
// don't think most GPUs really benefit from them anyway at this point.
#ifdef __APPLE__
#define NO_PRECOLOR_EARLY_RETURN
#endif

using RenderEvent = svtkRenderTimerLog::ScopedEventLogger;

#define TIME_FUNCTION(functionName) SVTK_SCOPED_RENDER_EVENT(#functionName, this->Timer);

svtkStandardNewMacro(svtkDualDepthPeelingPass);
svtkCxxSetObjectMacro(svtkDualDepthPeelingPass, VolumetricPass, svtkRenderPass);

namespace
{
void annotate(const std::string& str)
{
  svtkOpenGLRenderUtilities::MarkDebugEvent(str);
}
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::Render(const svtkRenderState* s)
{
  SVTK_SCOPED_RENDER_EVENT(
    "svtkDualDepthPeelingPass::Render", s->GetRenderer()->GetRenderWindow()->GetRenderTimer());

  svtkOpenGLRenderWindow* renWin =
    static_cast<svtkOpenGLRenderWindow*>(s->GetRenderer()->GetRenderWindow());

  this->State = renWin->GetState();

  // Setup svtkOpenGLRenderPass
  this->PreRender(s);

  this->Initialize(s);
  this->Prepare();

  if (this->IsRenderingVolumes())
  {
    this->PeelVolumesOutsideTranslucentRange();
  }

#ifndef DEBUG_VOLUME_PREPASS_PIXELS
  while (!this->PeelingDone())
  {
    this->Peel();
  }
#endif // DEBUG_VOLUME_PREPASS_PIXELS

  this->Finalize();

  this->PostRender(s);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::ReleaseGraphicsResources(svtkWindow* win)
{
  if (this->VolumetricPass)
  {
    this->VolumetricPass->ReleaseGraphicsResources(win);
  }
  if (this->BlendHelper)
  {
    delete this->BlendHelper;
    this->BlendHelper = nullptr;
  }
  if (this->BackBlendHelper)
  {
    delete this->BackBlendHelper;
    this->BackBlendHelper = nullptr;
  }
  if (this->CopyColorHelper)
  {
    delete this->CopyColorHelper;
    this->CopyColorHelper = nullptr;
  }
  if (this->CopyDepthHelper)
  {
    delete this->CopyDepthHelper;
    this->CopyDepthHelper = nullptr;
  }

  this->FreeGLObjects();
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::PreReplaceShaderValues(std::string& vertexShader,
  std::string& geometryShader, std::string& fragmentShader, svtkAbstractMapper* mapper,
  svtkProp* prop)
{
  switch (this->CurrentPeelType)
  {
    case svtkDualDepthPeelingPass::TranslucentPeel:
      // Do nothing -- these are handled in the post-replacements.
      return true;
    case svtkDualDepthPeelingPass::VolumetricPeel:
      // Forward to volumetric implementation:
      return this->PreReplaceVolumetricShaderValues(
        vertexShader, geometryShader, fragmentShader, mapper, prop);
    default:
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::PostReplaceShaderValues(std::string& vertexShader,
  std::string& geometryShader, std::string& fragmentShader, svtkAbstractMapper* mapper,
  svtkProp* prop)
{
  switch (this->CurrentPeelType)
  {
    case svtkDualDepthPeelingPass::TranslucentPeel:
      // Forward to translucent implementation:
      return this->PostReplaceTranslucentShaderValues(
        vertexShader, geometryShader, fragmentShader, mapper, prop);
    case svtkDualDepthPeelingPass::VolumetricPeel:
      // Do nothing; these are handled in the pre-replacements.
      return true;

    default:
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::SetShaderParameters(svtkShaderProgram* program,
  svtkAbstractMapper* mapper, svtkProp* prop, svtkOpenGLVertexArrayObject* VAO)
{
  switch (this->CurrentPeelType)
  {
    case svtkDualDepthPeelingPass::TranslucentPeel:
      return this->SetTranslucentShaderParameters(program, mapper, prop, VAO);
    case svtkDualDepthPeelingPass::VolumetricPeel:
      return this->SetVolumetricShaderParameters(program, mapper, prop, VAO);
    default:
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
svtkMTimeType svtkDualDepthPeelingPass::GetShaderStageMTime()
{
  return this->CurrentStageTimeStamp.GetMTime();
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::PostReplaceTranslucentShaderValues(
  std::string&, std::string&, std::string& fragmentShader, svtkAbstractMapper*, svtkProp*)
{
  switch (this->CurrentStage)
  {
    case svtkDualDepthPeelingPass::InitializingDepth:
      // Set gl_FragDepth if it isn't set already. It may have already been
      // replaced by the mapper, in which case the substitution will fail and
      // the previously set depth value will be used.
      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::Depth::Impl", "gl_FragDepth = gl_FragCoord.z;");
      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::DepthPeeling::Dec", "uniform sampler2D opaqueDepth;\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::PreColor",
        "ivec2 pixel = ivec2(gl_FragCoord.xy);\n"
        "  float oDepth = texelFetch(opaqueDepth, pixel, 0).y;\n"
        "  if (oDepth != -1. && gl_FragDepth > oDepth)\n"
        "    { // Ignore fragments that are occluded by opaque geometry:\n"
        "    gl_FragData[1].xy = vec2(-1., oDepth);\n"
        "    return;\n"
        "    }\n"
        "  else\n"
        "    {\n"
        "    gl_FragData[1].xy = vec2(-gl_FragDepth, gl_FragDepth);\n"
        "    return;\n"
        "    }\n");
      break;

    case svtkDualDepthPeelingPass::Peeling:
      // Set gl_FragDepth if it isn't set already. It may have already been
      // replaced by the mapper, in which case the substitution will fail and
      // the previously set depth value will be used.
      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::Depth::Impl", "gl_FragDepth = gl_FragCoord.z;");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Dec",
        "uniform sampler2D lastFrontPeel;\n"
        "uniform sampler2D lastDepthPeel;\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::PreColor",
        "  ivec2 pixelCoord = ivec2(gl_FragCoord.xy);\n"
        "  vec4 front = texelFetch(lastFrontPeel, pixelCoord, 0);\n"
        "  vec2 minMaxDepth = texelFetch(lastDepthPeel, pixelCoord, 0).xy;\n"
        "  float minDepth = -minMaxDepth.x;\n"
        "  float maxDepth = minMaxDepth.y;\n"
        "  // Use a tolerance when checking if we're on a current peel.\n"
        "  // Some OSX drivers compute slightly different fragment depths\n"
        "  // from one pass to the next. This value was determined\n"
        "  // through trial-and-error -- it may need to be increased at\n"
        "  // some point. See also the comment in svtkDepthPeelingPass's\n"
        "  // shader.\n"
        "  float epsilon = 0.0000001;\n"
        "\n"
        "  // Default outputs (no data/change):\n"
        "  gl_FragData[0] = vec4(0.);\n"
        "  gl_FragData[1] = front;\n"
        "  gl_FragData[2].xy = vec2(-1.);\n"
        "\n"
        "  // Is this fragment outside the current peels?\n"
        "  if (gl_FragDepth < minDepth - epsilon ||\n"
        "      gl_FragDepth > maxDepth + epsilon)\n"
        "    {\n"
#ifndef NO_PRECOLOR_EARLY_RETURN
        "    return;\n"
#else
        "    // Early return removed to avoid instruction-reordering bug\n"
        "    // with dFdx/dFdy on OSX drivers.\n"
        "    // return;\n"
#endif
        "    }\n"
        "\n"
        "  // Is this fragment inside the current peels?\n"
        "  if (gl_FragDepth > minDepth + epsilon &&\n"
        "      gl_FragDepth < maxDepth - epsilon)\n"
        "    {\n"
        "    // Write out depth so this frag will be peeled later:\n"
        "    gl_FragData[2].xy = vec2(-gl_FragDepth, gl_FragDepth);\n"
#ifndef NO_PRECOLOR_EARLY_RETURN
        "    return;\n"
#else
        "    // Early return removed to avoid instruction-reordering bug\n"
        "    // with dFdx/dFdy on OSX drivers.\n"
        "    // return;\n"
#endif
        "    }\n"
        "\n"
        "  // Continue processing for fragments on the current peel:\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Impl",
        "vec4 frag = gl_FragData[0];\n"
        "  // Default outputs (no data/change):\n"
        "\n"
        "  // This fragment is on a current peel:\n"
        "  if (gl_FragDepth >= minDepth - epsilon &&\n"
        "      gl_FragDepth <= minDepth + epsilon)\n"
        "    { // Front peel:\n"
        "    // Clear the back color:\n"
        "    gl_FragData[0] = vec4(0.);\n"
        "\n"
        "    // We store the front alpha value as (1-alpha) to allow MAX\n"
        "    // blending. This also means it is really initialized to 1,\n"
        "    // as it should be for under-blending.\n"
        "    front.a = 1. - front.a;\n"
        "\n"
        "    // Use under-blending to combine fragment with front color:\n"
        "    gl_FragData[1].rgb = front.a * frag.a * frag.rgb + front.rgb;\n"
        "    // Write out (1-alpha):\n"
        "    gl_FragData[1].a = 1. - (front.a * (1. - frag.a));\n"
        "    }\n"
#ifndef NO_PRECOLOR_EARLY_RETURN
        // just 'else' is ok. We'd return earlier in this case.
        "  else // (gl_FragDepth == maxDepth)\n"
#else
        // Need to explicitly test if this is the back peel, since early
        // returns are removed.
        "  else if (gl_FragDepth >= maxDepth - epsilon &&\n"
        "           gl_FragDepth <= maxDepth + epsilon)\n"
#endif
        "    { // Back peel:\n"
        "    // Dump premultiplied fragment, it will be blended later:\n"
        "    frag.rgb *= frag.a;\n"
        "    gl_FragData[0] = frag;\n"
        "    }\n"
#ifdef NO_PRECOLOR_EARLY_RETURN
        // Since the color outputs now get clobbered without the early
        // returns, reset them here.
        "  else\n"
        "    { // Need to clear the colors if not on a current peel.\n"
        "    gl_FragData[0] = vec4(0.);\n"
        "    gl_FragData[1] = front;\n"
        "    }\n"
#endif
      );
      break;

    case svtkDualDepthPeelingPass::AlphaBlending:
      // Set gl_FragDepth if it isn't set already. It may have already been
      // replaced by the mapper, in which case the substitution will fail and
      // the previously set depth value will be used.
      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::Depth::Impl", "gl_FragDepth = gl_FragCoord.z;");
      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::DepthPeeling::Dec", "uniform sampler2D lastDepthPeel;\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::PreColor",
        "  ivec2 pixelCoord = ivec2(gl_FragCoord.xy);\n"
        "  vec2 minMaxDepth = texelFetch(lastDepthPeel, pixelCoord, 0).xy;\n"
        "  float minDepth = -minMaxDepth.x;\n"
        "  float maxDepth = minMaxDepth.y;\n"
        "\n"
        "  // Discard all fragments outside of the last set of peels:\n"
        "  if (gl_FragDepth < minDepth || gl_FragDepth > maxDepth)\n"
        "    {\n"
        "    discard;\n"
        "    }\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Impl",
        "\n"
        "  // Pre-multiply alpha for depth peeling:\n"
        "  gl_FragData[0].rgb *= gl_FragData[0].a;\n");
      break;

    case svtkDualDepthPeelingPass::Inactive:
    case svtkDualDepthPeelingPass::NumberOfPasses:
    default:
      break;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::PreReplaceVolumetricShaderValues(
  std::string&, std::string&, std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp*)
{
  auto vmapper = svtkAbstractVolumeMapper::SafeDownCast(mapper);
  if (!vmapper)
  { // not a volume
    return true;
  }

  std::string rayInit =
    "  // Transform zStart and zEnd to texture_coordinates\n"
    "  mat4 NDCToTextureCoords = ip_inverseTextureDataAdjusted * in_inverseVolumeMatrix[0] *\n"
    "    in_inverseModelViewMatrix * in_inverseProjectionMatrix;\n"
    "  \n"
    "  // Start point\n"
    "  vec4 startPoint = WindowToNDC(gl_FragCoord.x, gl_FragCoord.y, zStart);\n"
    "  startPoint = NDCToTextureCoords * startPoint;\n"
    "  startPoint /= startPoint.w;\n"

    // startPoint could be located outside of the bounding box (bbox), this
    // is the case in:
    // 1. PeelVolumesOutside: Areas external to any geometry.
    // 2. PeelVolumetricGeometry: Areas where the volume is contained within
    // translucent geometry but the containing geometry lies outside of the bbox
    // (startPoint is either in-front or behind the bbox depending on the viewpoint).
    //
    // Given that startPoint could be located either in-front, inside or behind the\n"
    // bbox (the ray exit is unknown hence it is not possible to use clamp() directly),\n"
    // the clamp is divided in these three zones:\n"
    // a. In-front: clamp to ip_textureCoords (bbox's texture coord).\n"
    // b. Inside: use startPoint directly as it is peeling within the bbox.\n"
    // c. Behind: discard by returning vec4(0.f).\n"

    "\n"
    "  // Initialize g_dataPos as if startPoint lies Inside (b.)\n"
    "  g_dataPos = startPoint.xyz + g_rayJitter;\n"
    "\n"
    "  bool isInsideBBox = !(any(greaterThan(g_dataPos, in_texMax[0])) ||\n"
    "                        any(lessThan(g_dataPos, in_texMin[0])));\n"
    "  if (!isInsideBBox)\n"
    "  {\n"
    "    vec3 distStartTexCoord = g_rayOrigin - g_dataPos;\n"
    "    if (dot(distStartTexCoord, g_dirStep) < 0)\n"
    "    {\n"
    "      // startPoint lies behind the bounding box (c.)\n"
    "      return vec4(0.0);\n"
    "    }\n"
    "    // startPoint lies in-front (a.)\n"
    "    g_dataPos = g_rayOrigin + g_rayJitter;\n"
    "  }\n"
    "\n"
    "  // End point\n"
    "  {\n"
    "    vec4 endPoint = WindowToNDC(gl_FragCoord.x, gl_FragCoord.y, zEnd);\n"
    "    endPoint = NDCToTextureCoords * endPoint;\n"
    "    g_terminatePos = endPoint.xyz / endPoint.w;\n"
    "  }\n"
    "\n";

  if (vmapper->GetClippingPlanes())
  {
    rayInit += "  // Adjust the ray segment to account for clipping range:\n"
               "  if (!AdjustSampleRangeForClipping(g_dataPos.xyz, g_terminatePos.xyz))\n"
               "  {\n"
               "    return vec4(0.);\n"
               "  }\n"
               "\n";
  }
  rayInit +=
    "  // Update the number of ray marching steps to account for the clipped entry point (\n"
    "  // this is necessary in case the ray hits geometry after marching behind the plane,\n"
    "  // given that the number of steps was assumed to be from the not-clipped entry).\n"
    "  g_terminatePointMax = length(g_terminatePos.xyz - g_dataPos.xyz) /\n"
    "    length(g_dirStep);\n"
    "\n";

  const std::string pathCheck =
    "  // Make sure that we're sampling consistently across boundaries:\n"
    "  g_dataPos = ClampToSampleLocation(g_rayOrigin, g_dirStep, g_dataPos, true /*ceil*/);\n"
    "\n"
    "  // Ensure end is not located before start. This could be the case\n"
    "  // if end lies outside of the volume's bounding box. In those cases\n"
    "  // a transparent color is returned.\n"
    "  vec3 rgrif = g_terminatePos.xyz - g_dataPos.xyz;\n"
    "  if (dot(rgrif, g_dirStep) < 0)\n"
    "  {\n"
    "    return vec4(0.f);\n"
    "  }\n"
    "\n"
    "  // Compute the number of steps and reinitialize the step counter.\n"
    "  g_terminatePointMax = length(rgrif) / length(g_dirStep);\n"
    "  g_currentT = 0.0;\n"
    "  g_fragColor = vec4(0.0);\n"
    "\n";

  switch (this->CurrentStage)
  {
    case svtkDualDepthPeelingPass::InitializingDepth:
      // At this point, both CopyOpaqueDepthBuffer and InitializeDepth have run.
      //
      // DepthSource (inner) has either:
      // a. Same as outer/DepthDestination, or
      // b. (-transGeoDepthMin, transGeoDepthMax)
      // (a) if no transparent geo in front of opaque, (b) otherwise.
      //
      // DepthDestination (outer) has (-1, opaqueDepth), or (-1, -1) if no
      // opaque geometry.
      //
      // All color buffers are empty, so we can draw directly to them. No input
      // passthrough or blending needed.
      //
      // We'll check both of the depth buffers:
      //
      // 1) If the inner.y < 0, there is no geometry here. Render volume from
      //    0 --> 1 into the back buffer.
      // 2) If the outer.x == -1 and inner.y < 0, we have only opaque geometry
      //    here. Render volumes from 0 --> outer.y into the back buffer.
      // 3) If the 'max' depth differs between the buffers, then peel:
      //    0 --> -inner.x into front buffer
      //    inner.y --> outer.y into back buffer. If outer.y < 0, replace with 1

      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Termination::Init",
        "// Termination is defined somewhere else within the pass (CallWorker::Impl \n "
        "// and Ray::Init), so this tag is substituted for an empty implementation\n"
        "// to avoid unnecessary code.\n");

      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Dec",
        "uniform sampler2D outerDepthTex;\n"
        "uniform sampler2D innerDepthTex;\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::CallWorker::Impl",
        "  vec2 pixelCoord = vec2(gl_FragCoord.x, gl_FragCoord.y);\n"
        "  vec2 inner = texture2D(innerDepthTex, pixelCoord * in_inverseWindowSize).xy;\n"
        "  vec2 outer = texture2D(outerDepthTex, pixelCoord * in_inverseWindowSize).xy;\n"
        "\n"
        "  initializeRayCast();\n"
        "  vec4 front = vec4(0.f);\n"
        "  vec4 back = vec4(0.f);\n"
        "\n"
        "  // Check for the presence of opaque/trans geometry:\n"
        "  bool hasOpaqueGeometry = outer.y >= 0.f;\n"
        "  bool hasTranslucentGeometry = inner.x != -1.f;\n"
        "  bool hasAnyGeometry = hasOpaqueGeometry ||\n"
        "                        hasTranslucentGeometry;\n"
        "\n"
#ifndef DEBUG_VOLUME_PREPASS_PIXELS
        "  vec2 frontRange = vec2(1.f, -1.f);\n"
        "  vec2 backRange = vec2(1.f, -1.f);\n"
        "\n"
#endif // not DEBUG_VOLUME_PREPASS_PIXELS
        "  if (!hasAnyGeometry)\n"
        "  { // No opaque or translucent geometry\n"
#ifndef DEBUG_VOLUME_PREPASS_PIXELS
        "    backRange = vec2(0., 1.);\n"
#else  // not DEBUG_VOLUME_PREPASS_PIXELS
        "    back = vec4(1.f, 0.f, 0.f, 1.f);\n"
#endif // not DEBUG_VOLUME_PREPASS_PIXELS
        "  }\n"
        "  else if (!hasTranslucentGeometry)\n"
        "  { // Opaque geometry only.\n"
#ifndef DEBUG_VOLUME_PREPASS_PIXELS
        "    float opaqueDepth = inner.y;\n"
        "    backRange = vec2(0.f, opaqueDepth);\n"
#else  // not DEBUG_VOLUME_PREPASS_PIXELS
        "    back = vec4(0.f, 1.f, 0.f, 1.f);\n"
#endif // not DEBUG_VOLUME_PREPASS_PIXELS
        "  }\n"
        "  else // translucent geometry, maybe opaque, too:\n"
        "  {\n"
#ifndef DEBUG_VOLUME_PREPASS_PIXELS
        "    float opaqueDepth = hasOpaqueGeometry ? outer.y : 1.f;\n"
        "    frontRange = vec2(0.f, -inner.x);\n"
        "    backRange = vec2(inner.y, opaqueDepth);\n"
        "\n"
#else  // not DEBUG_VOLUME_PREPASS_PIXELS
        "    float blue = hasOpaqueGeometry ? 1.f : 0.f;\n"
        "    back = vec4(blue, 0.f, 1.f, 1.f);\n"
#endif // not DEBUG_VOLUME_PREPASS_PIXELS
        "  }\n"
        "\n"
#ifndef DEBUG_VOLUME_PREPASS_PIXELS
        "  if (frontRange.x < frontRange.y)\n"
        "  {\n"
        "    front = castRay(frontRange.x, frontRange.y);\n"
        "  }\n"
        "  if (backRange.x < backRange.y && // range valid\n"
        "      front.a < g_opacityThreshold) // early termination\n"
        "  {\n"
        "    back = castRay(backRange.x, backRange.y);\n"
        "  }\n"
        "\n"
#endif // not DEBUG_VOLUME_PREPASS_PIXELS
        "  gl_FragData[0] = back;\n"
        "  gl_FragData[1] = front;\n");

      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Ray::Init", rayInit);

      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::DepthPeeling::Ray::PathCheck", pathCheck);

      return true;

    case svtkDualDepthPeelingPass::Peeling:
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Dec",
        "uniform sampler2D outerDepthTex;\n"
        "uniform sampler2D innerDepthTex;\n"
        "uniform sampler2D lastFrontColorTex;\n"
        "uniform sampler2D opaqueDepthTex;\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::CallWorker::Impl",
        "  vec2 pixelCoord = vec2(gl_FragCoord.x, gl_FragCoord.y);\n"
        "  vec2 innerDepths = texture2D(innerDepthTex, pixelCoord * in_inverseWindowSize).xy;\n"
        "  vec2 outerDepths = texture2D(outerDepthTex, pixelCoord * in_inverseWindowSize).xy;\n"
        "  vec4 lastFrontColor = texture2D(lastFrontColorTex, pixelCoord * in_inverseWindowSize);\n"
        "\n"
        "  // Discard processed fragments\n"
        "  if (outerDepths.x == -1)\n"
        "  {\n"
        "    discard;\n"
        "  }\n"
        "\n"
        "  // Negate the near depths; they're negative for MAX blending:\n"
        "  float frontStartDepth = -outerDepths.x;\n"
        "  float frontEndDepth   = -innerDepths.x;\n"
        "  float backStartDepth  = innerDepths.y;\n"
        "  float backEndDepth    = outerDepths.y;\n"
        "\n"
        "  // Only record the back color (for occlusion queries) if the\n"
        "  // front/back ranges are the same:\n"
        "  bool onlyBack = frontStartDepth == backStartDepth &&\n"
        "                  frontEndDepth == backEndDepth;\n"
        "\n"

        // In the last peel, innerDepths may be (-1, -1) for most of the
        // fragments. Casting a ray from [outerDepths.x, 1.0] would result
        // in accumulating areas that have already been accounted for in
        // former volume peels.  In this case frontEndDepth should be the
        // outer max instead. Because of this, the back castRay() is also
        // skipped.

        "  bool noInnerDepths = innerDepths.x == -1.0;\n"
        "  if (noInnerDepths)\n"
        "  {\n"
        "    frontEndDepth = outerDepths.y;\n"
        "  }\n"
        "\n"

        // Peel passes set -1 in pixels that contain only opaque geometry,
        // so the opaque depth is fetched in order to z-composite volumes
        // with opaque goemetry. To do this, the end point of front is clamped
        // to opaque-depth and back ray-cast is skipped altogether since it
        // would be covered by opaque geometry anyway.

        "  float oDepth = texture2D(opaqueDepthTex, pixelCoord * in_inverseWindowSize).x;\n"
        "  bool endBehindOpaque = frontEndDepth >= oDepth;\n"
        "  float clampedFrontEnd = frontEndDepth;\n"
        "  if (endBehindOpaque)\n"
        "  {\n"
        "    clampedFrontEnd = clamp(frontEndDepth, oDepth, oDepth);\n"
        "  }\n"
        "  \n"
        "  initializeRayCast();\n"
        "  vec4 frontColor = vec4(0.f);\n"
        "  if (!onlyBack)\n"
        "  {\n"
        "    frontColor = castRay(frontStartDepth,\n"
        "                         clampedFrontEnd);\n"
        "  }\n"
        "\n"
        "  vec4 backColor = vec4(0.);\n"
        "  if (!endBehindOpaque && !noInnerDepths)"
        "  {\n"
        "    backColor = castRay(backStartDepth,\n"
        "                        backEndDepth);\n"
        "  }\n"
        "\n"
        "  // The color returned by castRay() has alpha pre-multiplied,\n"
        "  // as required for back-blending.\n"
        "  gl_FragData[0] = backColor;\n"
        "\n"
        "  // Front color is written with negated alpha for MAX blending:\n"
        "  lastFrontColor.a = 1. - lastFrontColor.a;\n"
        "\n"
        "  // Use under-blending to mix the front color on-the-fly:\n"
        "  // (note that frontColor.rgb is already multiplied by its\n"
        "  // alpha, this is done within castRay())\n"
        "  gl_FragData[1].rgb =\n"
        "    lastFrontColor.a * frontColor.rgb + lastFrontColor.rgb;\n"
        "\n"
        "  // Write out (1-alpha) for MAX blending:\n"
        "  gl_FragData[1].a = 1. - (lastFrontColor.a * (1. - frontColor.a));\n");

      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Ray::Init", rayInit);

      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::DepthPeeling::Ray::PathCheck", pathCheck);

      break;

    case svtkDualDepthPeelingPass::AlphaBlending:
      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::DepthPeeling::Dec", "uniform sampler2D depthRangeTex;\n");
      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::CallWorker::Impl",
        "  vec2 pixelCoord = vec2(gl_FragCoord.x, gl_FragCoord.y);\n"
        "  vec2 depthRange = texture2D(depthRangeTex, pixelCoord * in_inverseWindowSize).xy;\n"
        "\n"
        "  // Discard processed fragments\n"
        "  if (depthRange.x == -1.0)\n"
        "  {\n"
        "    discard;\n"
        "  }\n"
        "\n"
        "  float startDepth = -depthRange.x;\n"
        "  float endDepth = depthRange.y;\n"
        "\n"
        "  initializeRayCast();\n"
        "  vec4 color = castRay(startDepth, endDepth);\n"
        "\n"
        "  // The color returned by castRay() has alpha pre-multiplied,\n"
        "  // as required for back-blending.\n"
        "  gl_FragData[0] = color;\n");

      svtkShaderProgram::Substitute(fragmentShader, "//SVTK::DepthPeeling::Ray::Init", rayInit);

      svtkShaderProgram::Substitute(
        fragmentShader, "//SVTK::DepthPeeling::Ray::PathCheck", pathCheck);

      break;

    case svtkDualDepthPeelingPass::Inactive:
    case svtkDualDepthPeelingPass::NumberOfPasses:
    default:
      break;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::SetTranslucentShaderParameters(
  svtkShaderProgram* program, svtkAbstractMapper*, svtkProp*, svtkOpenGLVertexArrayObject*)
{
  switch (this->CurrentStage)
  {
    case svtkDualDepthPeelingPass::InitializingDepth:
      program->SetUniformi("opaqueDepth", this->Textures[this->DepthDestination]->GetTextureUnit());
      break;

    case svtkDualDepthPeelingPass::Peeling:
      program->SetUniformi("lastDepthPeel", this->Textures[this->DepthSource]->GetTextureUnit());
      program->SetUniformi("lastFrontPeel", this->Textures[this->FrontSource]->GetTextureUnit());
      break;

    case svtkDualDepthPeelingPass::AlphaBlending:
      program->SetUniformi("lastDepthPeel", this->Textures[this->DepthSource]->GetTextureUnit());
      break;

    case svtkDualDepthPeelingPass::Inactive:
    case svtkDualDepthPeelingPass::NumberOfPasses:
    default:
      break;
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::SetVolumetricShaderParameters(
  svtkShaderProgram* program, svtkAbstractMapper*, svtkProp*, svtkOpenGLVertexArrayObject*)
{
  switch (this->CurrentStage)
  {
    case svtkDualDepthPeelingPass::InitializingDepth:
      program->SetUniformi(
        "outerDepthTex", this->Textures[this->DepthDestination]->GetTextureUnit());
      program->SetUniformi("innerDepthTex", this->Textures[this->DepthSource]->GetTextureUnit());
      return true;

    case svtkDualDepthPeelingPass::Peeling:
      program->SetUniformi("outerDepthTex", this->Textures[this->DepthSource]->GetTextureUnit());
      program->SetUniformi(
        "innerDepthTex", this->Textures[this->DepthDestination]->GetTextureUnit());
      program->SetUniformi(
        "lastFrontColorTex", this->Textures[this->FrontSource]->GetTextureUnit());
      program->SetUniformi("opaqueDepthTex", this->Textures[OpaqueDepth]->GetTextureUnit());
      break;

    case svtkDualDepthPeelingPass::AlphaBlending:
      program->SetUniformi("depthRangeTex", this->Textures[this->DepthSource]->GetTextureUnit());
      break;

    case svtkDualDepthPeelingPass::Inactive:
    case svtkDualDepthPeelingPass::NumberOfPasses:
    default:
      break;
  }

  return true;
}

//------------------------------------------------------------------------------
svtkDualDepthPeelingPass::svtkDualDepthPeelingPass()
  : VolumetricPass(nullptr)
  , RenderState(nullptr)
  , CopyColorHelper(nullptr)
  , CopyDepthHelper(nullptr)
  , BackBlendHelper(nullptr)
  , BlendHelper(nullptr)
  , FrontSource(FrontA)
  , FrontDestination(FrontB)
  , DepthSource(DepthA)
  , DepthDestination(DepthB)
  , CurrentStage(Inactive)
  , CurrentPeelType(TranslucentPeel)
  , LastPeelHadVolumes(false)
  , CurrentPeel(0)
  , TranslucentOcclusionQueryId(0)
  , TranslucentWrittenPixels(0)
  , VolumetricOcclusionQueryId(0)
  , VolumetricWrittenPixels(0)
  , OcclusionThreshold(0)
  , TranslucentRenderCount(0)
  , VolumetricRenderCount(0)
  , SaveScissorTestState(false)
  , CullFaceMode(0)
  , CullFaceEnabled(false)
  , DepthTestEnabled(true)
{
  std::fill(this->Textures, this->Textures + static_cast<int>(NumberOfTextures),
    static_cast<svtkTextureObject*>(nullptr));
}

//------------------------------------------------------------------------------
svtkDualDepthPeelingPass::~svtkDualDepthPeelingPass()
{
  this->FreeGLObjects();

  if (this->VolumetricPass)
  {
    this->SetVolumetricPass(nullptr);
  }
  if (this->BlendHelper)
  {
    delete this->BlendHelper;
    this->BlendHelper = nullptr;
  }
  if (this->BackBlendHelper)
  {
    delete this->BackBlendHelper;
    this->BackBlendHelper = nullptr;
  }
  if (this->CopyColorHelper)
  {
    delete this->CopyColorHelper;
    this->CopyColorHelper = nullptr;
  }
  if (this->CopyDepthHelper)
  {
    delete this->CopyDepthHelper;
    this->CopyDepthHelper = nullptr;
  }
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::SetCurrentStage(ShaderStage stage)
{
  if (stage != this->CurrentStage)
  {
    this->CurrentStage = stage;
    this->CurrentStageTimeStamp.Modified();
  }
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::FreeGLObjects()
{
  for (int i = 0; i < static_cast<int>(NumberOfTextures); ++i)
  {
    if (this->Textures[i])
    {
      this->Textures[i]->Delete();
      this->Textures[i] = nullptr;
    }
  }
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::RenderTranslucentPass()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::RenderTranslucentPass);
  this->TranslucentPass->Render(this->RenderState);
  ++this->TranslucentRenderCount;
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::RenderVolumetricPass()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::RenderVolumetricPass);
  this->VolumetricPass->Render(this->RenderState);
  ++this->VolumetricRenderCount;
  this->LastPeelHadVolumes = this->VolumetricPass->GetNumberOfRenderedProps() > 0;
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::IsRenderingVolumes()
{
  return this->VolumetricPass && this->LastPeelHadVolumes;
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::Initialize(const svtkRenderState* s)
{
  this->RenderState = s;
  this->LastPeelHadVolumes = true;

  // Get current viewport size:
  svtkRenderer* r = s->GetRenderer();
  if (s->GetFrameBuffer() == nullptr)
  {
    // get the viewport dimensions
    r->GetTiledSizeAndOrigin(
      &this->ViewportWidth, &this->ViewportHeight, &this->ViewportX, &this->ViewportY);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    this->ViewportWidth = size[0];
    this->ViewportHeight = size[1];
    this->ViewportX = 0;
    this->ViewportY = 0;
  }

  this->Timer = r->GetRenderWindow()->GetRenderTimer();

  // The above code shouldn't touch the OpenGL command stream, so it's okay to
  // start the event here:
  TIME_FUNCTION(svtkDualDepthPeelingPass::Initialize);

  // adjust size as needed
  for (int i = 0; i < static_cast<int>(NumberOfTextures); ++i)
  {
    if (this->Textures[i])
    {
      this->Textures[i]->Resize(this->ViewportWidth, this->ViewportHeight);
    }
  }

  // Allocate new textures if needed:
  if (!this->Framebuffer)
  {
    this->Framebuffer = svtkOpenGLFramebufferObject::New();
  }

  if (!this->Textures[BackTemp])
  {
    std::generate(
      this->Textures, this->Textures + static_cast<int>(NumberOfTextures), &svtkTextureObject::New);

    this->InitColorTexture(this->Textures[BackTemp], s);
    this->InitColorTexture(this->Textures[Back], s);
    this->InitColorTexture(this->Textures[FrontA], s);
    this->InitColorTexture(this->Textures[FrontB], s);
    this->InitDepthTexture(this->Textures[DepthA], s);
    this->InitDepthTexture(this->Textures[DepthB], s);
    this->InitOpaqueDepthTexture(this->Textures[OpaqueDepth], s);
  }

  this->InitFramebuffer(s);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitColorTexture(svtkTextureObject* tex, const svtkRenderState* s)
{
  tex->SetContext(static_cast<svtkOpenGLRenderWindow*>(s->GetRenderer()->GetRenderWindow()));
  tex->SetFormat(GL_RGBA);
  tex->SetInternalFormat(GL_RGBA8);
  tex->Allocate2D(
    this->ViewportWidth, this->ViewportHeight, 4, svtkTypeTraits<svtkTypeUInt8>::SVTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitDepthTexture(svtkTextureObject* tex, const svtkRenderState* s)
{
  tex->SetContext(static_cast<svtkOpenGLRenderWindow*>(s->GetRenderer()->GetRenderWindow()));
  tex->SetFormat(GL_RG);
  tex->SetInternalFormat(GL_RG32F);
  tex->Allocate2D(
    this->ViewportWidth, this->ViewportHeight, 2, svtkTypeTraits<svtkTypeFloat32>::SVTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitOpaqueDepthTexture(svtkTextureObject* tex, const svtkRenderState* s)
{
  tex->SetContext(static_cast<svtkOpenGLRenderWindow*>(s->GetRenderer()->GetRenderWindow()));
  tex->AllocateDepth(this->ViewportWidth, this->ViewportHeight, svtkTextureObject::Float32);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitFramebuffer(const svtkRenderState* s)
{
  this->Framebuffer->SetContext(
    static_cast<svtkOpenGLRenderWindow*>(s->GetRenderer()->GetRenderWindow()));

  // Save the current FBO bindings to restore them later.
  this->State->PushDrawFramebufferBinding();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::ActivateDrawBuffers(const TextureName* ids, size_t numTex)
{
  this->Framebuffer->DeactivateDrawBuffers();
  for (size_t i = 0; i < numTex; ++i)
  {
    this->Framebuffer->AddColorAttachment(static_cast<unsigned int>(i), this->Textures[ids[i]]);
  }

  const unsigned int numBuffers = static_cast<unsigned int>(numTex);
  this->SetActiveDrawBuffers(numBuffers);
  this->Framebuffer->ActivateDrawBuffers(numBuffers);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::Prepare()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::Prepare);

  // Since we're rendering into a temporary non-default framebuffer, we need to
  // remove the translation from the viewport and disable the scissor test;
  // otherwise we'll capture the wrong area of the rendered geometry.
  this->State->svtkglViewport(0, 0, this->ViewportWidth, this->ViewportHeight);
  this->SaveScissorTestState = this->State->GetEnumState(GL_SCISSOR_TEST);
  this->State->svtkglDisable(GL_SCISSOR_TEST);

  // bad sync here
  this->State->svtkglGetIntegerv(GL_CULL_FACE_MODE, &this->CullFaceMode);
  this->CullFaceEnabled = this->State->GetEnumState(GL_CULL_FACE);

  this->DepthTestEnabled = this->State->GetEnumState(GL_DEPTH_TEST);

  // Prevent svtkOpenGLActor from messing with the depth mask:
  size_t numProps = this->RenderState->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    svtkProp* prop = this->RenderState->GetPropArray()[i];
    svtkInformation* info = prop->GetPropertyKeys();
    if (!info)
    {
      info = svtkInformation::New();
      prop->SetPropertyKeys(info);
      info->FastDelete();
    }
    info->Set(svtkOpenGLActor::GLDepthMaskOverride(), -1);
  }

  // Setup GL state:
  this->State->svtkglDisable(GL_DEPTH_TEST);
  this->InitializeOcclusionQuery();
  this->CurrentPeel = 0;
  this->TranslucentRenderCount = 0;
  this->VolumetricRenderCount = 0;

  // Save the current FBO bindings to restore them later.
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  // The source front buffer must be initialized, since it simply uses additive
  // blending.
  // The back-blending may discard fragments, so the back peel accumulator needs
  // initialization as well.
  std::array<TextureName, 2> targets = { { Back, this->FrontSource } };
  this->ActivateDrawBuffers(targets);
  this->State->svtkglClearColor(0.f, 0.f, 0.f, 0.f);
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT);

  // Fill both depth buffers with -1, -1. This lets us discard fragments in
  // CopyOpaqueDepthBuffers, which gives a moderate performance boost.
  targets[0] = this->DepthSource;
  targets[1] = this->DepthDestination;
  this->ActivateDrawBuffers(targets);
  this->State->svtkglClearColor(-1, -1, 0, 0);
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT);

  // Pre-fill the depth buffer with opaque pass data:
  this->CopyOpaqueDepthBuffer();

  // Initialize the transparent depths for the peeling algorithm:
  this->InitializeDepth();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitializeOcclusionQuery()
{
  glGenQueries(1, &this->TranslucentOcclusionQueryId);
  glGenQueries(1, &this->VolumetricOcclusionQueryId);

  int numPixels = this->ViewportHeight * this->ViewportWidth;
  this->OcclusionThreshold = numPixels * this->OcclusionRatio;
  this->TranslucentWrittenPixels = this->OcclusionThreshold + 1;
  this->VolumetricWrittenPixels = this->OcclusionThreshold + 1;
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::CopyOpaqueDepthBuffer()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::CopyOpaqueDepthBuffer);

  // Initialize the peeling depth buffer using the existing opaque depth buffer.
  // Note that the min component is stored as -depth, allowing
  // glBlendEquation = GL_MAX to be used during peeling.

  // Copy from the current (default) framebuffer's depth buffer into a texture:
  this->State->PopDrawFramebufferBinding();
  this->Textures[OpaqueDepth]->CopyFromFrameBuffer(
    this->ViewportX, this->ViewportY, 0, 0, this->ViewportWidth, this->ViewportHeight);
  this->State->PushDrawFramebufferBinding();
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  // Fill both depth buffers with the opaque fragment depths. InitializeDepth
  // will compare translucent fragment depths with values in DepthDestination
  // and write to DepthSource using MAX blending, so we need both to have opaque
  // fragments (src/dst seem reversed because they're named for their usage in
  // PeelRender).
  std::array<TextureName, 2> targets = { { this->DepthSource, this->DepthDestination } };
  this->ActivateDrawBuffers(targets);
  this->Textures[OpaqueDepth]->Activate();

  this->State->svtkglDisable(GL_BLEND);

  svtkOpenGLRenderWindow* renWin =
    static_cast<svtkOpenGLRenderWindow*>(this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->CopyDepthHelper)
  {
    std::string fragShader = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
    svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Decl",
      "uniform float clearValue;\n"
      "uniform sampler2D oDepth;\n");
    svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Impl",
      "  float d = texture2D(oDepth, texCoord).x;\n"
      "  if (d == clearValue)\n"
      "    { // If no depth value has been written, discard the frag:\n"
      "    discard;\n"
      "    }\n"
      "  gl_FragData[0] = gl_FragData[1] = vec4(-1, d, 0., 0.);\n");
    this->CopyDepthHelper = new svtkOpenGLQuadHelper(renWin, nullptr, fragShader.c_str(), nullptr);
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->CopyDepthHelper->Program);
  }

  if (!this->CopyDepthHelper->Program)
  {
    return;
  }

  // Get the clear value. We don't set this, so it should still be what the
  // opaque pass uses:
  GLfloat clearValue = 1.f;
  glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearValue);
  this->CopyDepthHelper->Program->SetUniformf("clearValue", clearValue);
  this->CopyDepthHelper->Program->SetUniformi(
    "oDepth", this->Textures[OpaqueDepth]->GetTextureUnit());

  annotate("Copying opaque depth!");
  this->CopyDepthHelper->Render();
  annotate("Opaque depth copied!");

  this->Textures[OpaqueDepth]->Deactivate();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitializeDepth()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::InitializeDepth);

  // Add the translucent geometry to our depth peeling buffer:

  // We bind the back temporary buffer as render target 0 -- the data we
  // write to it isn't used, but this makes it easier to work with the existing
  // polydata shaders as they expect gl_FragData[0] to be RGBA. The front
  // destination buffer is cleared prior to peeling, so it's just a dummy
  // buffer at this point.
  std::array<TextureName, 2> targets = { { BackTemp, this->DepthSource } };
  this->ActivateDrawBuffers(targets);

  this->SetCurrentStage(InitializingDepth);
  this->SetCurrentPeelType(TranslucentPeel);
  this->Textures[this->DepthDestination]->Activate();

  this->State->svtkglEnable(GL_BLEND);
  this->State->svtkglBlendEquation(GL_MAX);
  annotate("Initializing depth.");
  this->RenderTranslucentPass();
  annotate("Depth initialized");

  this->Textures[this->DepthDestination]->Deactivate();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::PeelVolumesOutsideTranslucentRange()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::PeelVolumesOutsideTranslucentRange);

  // Enable the destination targets. Note that we're rendering directly into
  // the Back accumulation buffer and the FrontSource buffer, since we know
  // this is the first time these buffers will be drawn into.
  std::array<TextureName, 2> targets = { { Back, this->FrontSource } };
  this->ActivateDrawBuffers(targets);

  // Cull back fragments of the volume's proxy geometry since they are
  // not necessary anyway.
  this->State->svtkglCullFace(GL_BACK);
  this->State->svtkglEnable(GL_CULL_FACE);

  this->SetCurrentStage(InitializingDepth);
  this->SetCurrentPeelType(VolumetricPeel);

  this->Textures[this->DepthSource]->Activate();
  this->Textures[this->DepthDestination]->Activate();

  annotate("Peeling volumes external to translucent geometry.");
  this->RenderVolumetricPass();
  annotate("External volume peel done.");

  this->State->svtkglCullFace(this->CullFaceMode);
  this->State->svtkglDisable(GL_CULL_FACE);

  this->Textures[this->DepthSource]->Deactivate();
  this->Textures[this->DepthDestination]->Deactivate();
}

//------------------------------------------------------------------------------
bool svtkDualDepthPeelingPass::PeelingDone()
{
  const auto writtenPix = this->TranslucentWrittenPixels + this->VolumetricWrittenPixels;

  return this->CurrentPeel >= this->MaximumNumberOfPeels || writtenPix <= this->OcclusionThreshold;
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::Peel()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::Peel);

  this->InitializeTargetsForTranslucentPass();
  this->PeelTranslucentGeometry();
  this->StartTranslucentOcclusionQuery();
  this->BlendBackBuffer();
  this->EndTranslucentOcclusionQuery();
  this->SwapFrontBufferSourceDest();

  if (this->IsRenderingVolumes())
  {
    this->InitializeTargetsForVolumetricPass();
    this->PeelVolumetricGeometry();

    this->StartVolumetricOcclusionQuery();
    this->BlendBackBuffer();
    this->EndVolumetricOcclusionQuery();
    this->SwapFrontBufferSourceDest();
  }

  this->SwapDepthBufferSourceDest();

  ++this->CurrentPeel;

#ifdef DEBUG_PEEL
  std::cout << "Peel " << this->CurrentPeel
            << ": Pixels written: trans=" << this->TranslucentWrittenPixels
            << " volume=" << this->VolumetricWrittenPixels
            << " (threshold: " << this->OcclusionThreshold << ")\n";
#endif // DEBUG_PEEL
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::PrepareFrontDestination()
{
  // If we're not using volumes, clear the front destination buffer and just
  // let the shaders pass-through the colors from the previous peel.
  //
  // If we are rendering volumes, we can't rely on the shader pass-through,
  // since the volumetric and translucent geometry may not cover the same
  // pixels, and information would be lost if we simply cleared the front
  // buffer. In this case, we're essentially forcing a fullscreen pass-through
  // prior to the any actual rendering calls.
  if (!this->IsRenderingVolumes())
  {
    this->ClearFrontDestination();
  }
  else
  {
    this->CopyFrontSourceToFrontDestination();
  }
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::ClearFrontDestination()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::ClearFrontDestination);
  annotate("ClearFrontDestination()");
  this->ActivateDrawBuffer(this->FrontDestination);
  this->State->svtkglClearColor(0.f, 0.f, 0.f, 0.f);
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::CopyFrontSourceToFrontDestination()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::CopyFrontSourceToFrontDestination);

  this->ActivateDrawBuffer(this->FrontDestination);

  this->State->svtkglDisable(GL_BLEND);

  svtkOpenGLRenderWindow* renWin =
    static_cast<svtkOpenGLRenderWindow*>(this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->CopyColorHelper)
  {
    std::string fragShader = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
    svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Decl", "uniform sampler2D inTex;\n");
    svtkShaderProgram::Substitute(
      fragShader, "//SVTK::FSQ::Impl", "  gl_FragData[0] = texture2D(inTex, texCoord);\n");
    this->CopyColorHelper = new svtkOpenGLQuadHelper(renWin, nullptr, fragShader.c_str(), nullptr);
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->CopyColorHelper->Program);
  }

  if (!this->CopyColorHelper->Program)
  {
    return;
  }

  this->Textures[this->FrontSource]->Activate();
  this->CopyColorHelper->Program->SetUniformi(
    "inTex", this->Textures[this->FrontSource]->GetTextureUnit());

  annotate("Copying front texture src -> dst for pre-pass initialization!");
  this->CopyColorHelper->Render();
  annotate("Front texture copied!");

  this->Textures[this->FrontSource]->Deactivate();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitializeTargetsForTranslucentPass()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::InitializeTargetsForTranslucentPass);

  // Initialize destination buffers to their minima, since we're MAX blending,
  // this ensures that valid outputs are captured.
  this->ActivateDrawBuffer(BackTemp);
  this->State->svtkglClearColor(0.f, 0.f, 0.f, 0.f);
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT);

  this->ActivateDrawBuffer(this->DepthDestination);
  this->State->svtkglClearColor(-1.f, -1.f, 0.f, 0.f);
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT);

  this->PrepareFrontDestination();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::InitializeTargetsForVolumetricPass()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::InitializeTargetsForVolumetricPass);

  // Clear the back buffer to ensure that current fragments are captured for
  // later blending into the back accumulation buffer:
  this->ActivateDrawBuffer(BackTemp);
  this->State->svtkglClearColor(0.f, 0.f, 0.f, 0.f);
  this->State->svtkglClear(GL_COLOR_BUFFER_BIT);

  this->PrepareFrontDestination();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::PeelTranslucentGeometry()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::PeelTranslucentGeometry);

  // Enable the destination targets:
  std::array<TextureName, 3> targets = { { BackTemp, this->FrontDestination,
    this->DepthDestination } };
  this->ActivateDrawBuffers(targets);

  // Use MAX blending to capture peels:
  this->State->svtkglEnable(GL_BLEND);
  this->State->svtkglBlendEquation(GL_MAX);

  this->SetCurrentStage(Peeling);
  this->SetCurrentPeelType(TranslucentPeel);
  this->Textures[this->FrontSource]->Activate();
  this->Textures[this->DepthSource]->Activate();

  annotate("Start translucent peeling!");
  this->RenderTranslucentPass();
  annotate("Translucent peeling done!");

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[this->DepthSource]->Deactivate();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::PeelVolumetricGeometry()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::PeelVolumeGeometry);

  // Enable the destination targets:
  std::array<TextureName, 2> targets = { { BackTemp, this->FrontDestination } };
  this->ActivateDrawBuffers(targets);

  // Cull back fragments of the volume's proxy geometry since they are
  // not necessary anyway.
  this->State->svtkglCullFace(GL_BACK);
  this->State->svtkglEnable(GL_CULL_FACE);

  // Use MAX blending to capture peels:
  this->State->svtkglEnable(GL_BLEND);
  this->State->svtkglBlendEquation(GL_MAX);

  this->SetCurrentStage(Peeling);
  this->SetCurrentPeelType(VolumetricPeel);

  this->Textures[this->FrontSource]->Activate();
  this->Textures[this->DepthSource]->Activate();
  this->Textures[this->DepthDestination]->Activate();
  this->Textures[OpaqueDepth]->Activate();

  annotate("Start volumetric peeling!");
  this->RenderVolumetricPass();
  annotate("Volumetric peeling done!");

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[this->DepthSource]->Deactivate();
  this->Textures[this->DepthDestination]->Deactivate();
  this->Textures[OpaqueDepth]->Deactivate();

  this->State->svtkglCullFace(this->CullFaceMode);
  this->State->svtkglDisable(GL_CULL_FACE);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::BlendBackBuffer()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::BlendBackBuffer);

  this->ActivateDrawBuffer(Back);
  this->Textures[BackTemp]->Activate();

  /* For this step, we blend the last peel's back fragments into a back-
   * accumulation buffer. The full over-blending equations are:
   *
   * (f = front frag (incoming peel); b = back frag (current accum. buffer))
   *
   * a = f.a + (1. - f.a) * b.a
   *
   * if a == 0, C == (0, 0, 0). Otherwise,
   *
   * C = ( f.a * f.rgb + (1. - f.a) * b.a * b.rgb ) / a
   *
   * We use premultiplied alphas to save on computations, resulting in:
   *
   * [a * C] = [f.a * f.rgb] + (1 - f.a) * [ b.a * b.rgb ]
   * a = f.a + (1. - f.a) * b.a
   */

  this->State->svtkglEnable(GL_BLEND);
  this->State->svtkglBlendEquation(GL_FUNC_ADD);
  this->State->svtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  svtkOpenGLRenderWindow* renWin =
    static_cast<svtkOpenGLRenderWindow*>(this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->BackBlendHelper)
  {
    std::string fragShader = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
    svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Decl", "uniform sampler2D newPeel;\n");
    svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Impl",
      "  vec4 f = texture2D(newPeel, texCoord); // new frag\n"
      "  if (f.a == 0.)\n"
      "    {\n"
      "    discard;\n"
      "    }\n"
      "\n"
      "  gl_FragData[0] = f;\n");
    this->BackBlendHelper = new svtkOpenGLQuadHelper(renWin, nullptr, fragShader.c_str(), nullptr);
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BackBlendHelper->Program);
  }

  if (!this->BackBlendHelper->Program)
  {
    return;
  }

  this->BackBlendHelper->Program->SetUniformi(
    "newPeel", this->Textures[BackTemp]->GetTextureUnit());

  annotate("Start blending back!");
  this->BackBlendHelper->Render();
  annotate("Back blended!");

  this->Textures[BackTemp]->Deactivate();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::StartTranslucentOcclusionQuery()
{
  // ES 3.0 only supports checking if *any* samples passed. We'll just use
  // that query to stop peeling once all frags are processed, and ignore the
  // requested occlusion ratio.
#ifdef GL_ES_VERSION_3_0
  glBeginQuery(GL_ANY_SAMPLES_PASSED, this->TranslucentOcclusionQueryId);
#else  // GL ES 3.0
  glBeginQuery(GL_SAMPLES_PASSED, this->TranslucentOcclusionQueryId);
#endif // GL ES 3.0
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::EndTranslucentOcclusionQuery()
{
  // We time the end, but not the start, since this is where we stall to
  // sync the stream.
  TIME_FUNCTION(svtkDualDepthPeelingPass::EndTranslucentOcclusionQuery);

#ifdef GL_ES_VERSION_3_0
  glEndQuery(GL_ANY_SAMPLES_PASSED);
  GLuint anySamplesPassed;
  glGetQueryObjectuiv(this->TranslucentOcclusionQueryId, GL_QUERY_RESULT, &anySamplesPassed);
  this->TranslucentWrittenPixels = anySamplesPassed ? this->OcclusionThreshold + 1 : 0;
#else  // GL ES 3.0
  glEndQuery(GL_SAMPLES_PASSED);
  glGetQueryObjectuiv(
    this->TranslucentOcclusionQueryId, GL_QUERY_RESULT, &this->TranslucentWrittenPixels);
#endif // GL ES 3.0
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::StartVolumetricOcclusionQuery()
{
  // ES 3.0 only supports checking if *any* samples passed. We'll just use
  // that query to stop peeling once all frags are processed, and ignore the
  // requested occlusion ratio.
#ifdef GL_ES_VERSION_3_0
  glBeginQuery(GL_ANY_SAMPLES_PASSED, this->VolumetricOcclusionQueryId);
#else  // GL ES 3.0
  glBeginQuery(GL_SAMPLES_PASSED, this->VolumetricOcclusionQueryId);
#endif // GL ES 3.0
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::EndVolumetricOcclusionQuery()
{
  // We time the end, but not the start, since this is where we stall to
  // sync the stream.
  TIME_FUNCTION(svtkDualDepthPeelingPass::EndVolumetricOcclusionQuery);

#ifdef GL_ES_VERSION_3_0
  glEndQuery(GL_ANY_SAMPLES_PASSED);
  GLuint anySamplesPassed;
  glGetQueryObjectuiv(this->VolumetricOcclusionQueryId, GL_QUERY_RESULT, &anySamplesPassed);
  this->VolumetricWrittenPixels = anySamplesPassed ? this->OcclusionThreshold + 1 : 0;
#else  // GL ES 3.0
  glEndQuery(GL_SAMPLES_PASSED);
  glGetQueryObjectuiv(
    this->VolumetricOcclusionQueryId, GL_QUERY_RESULT, &this->VolumetricWrittenPixels);
#endif // GL ES 3.0
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::SwapFrontBufferSourceDest()
{
  std::swap(this->FrontSource, this->FrontDestination);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::SwapDepthBufferSourceDest()
{
  std::swap(this->DepthSource, this->DepthDestination);
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::Finalize()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::Finalize);

  // Mop up any unrendered fragments using simple alpha blending into the back
  // buffer.
#ifndef DEBUG_VOLUME_PREPASS_PIXELS
  if (this->TranslucentWrittenPixels > 0 || this->VolumetricWrittenPixels > 0)
  {
    this->AlphaBlendRender();
  }
#endif // DEBUG_VOLUME_PREPASS_PIXELS

  this->NumberOfRenderedProps = this->TranslucentPass->GetNumberOfRenderedProps();

  if (this->IsRenderingVolumes())
  {
    this->NumberOfRenderedProps += this->VolumetricPass->GetNumberOfRenderedProps();
  }

  this->Framebuffer->UnBind(GL_DRAW_FRAMEBUFFER);
  this->State->PopDrawFramebufferBinding();
  this->BlendFinalImage();

  // Restore blending parameters:
  this->State->svtkglEnable(GL_BLEND);
  this->State->svtkglBlendEquation(GL_FUNC_ADD);
  this->State->svtkglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  size_t numProps = this->RenderState->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    svtkProp* prop = this->RenderState->GetPropArray()[i];
    svtkInformation* info = prop->GetPropertyKeys();
    if (info)
    {
      info->Remove(svtkOpenGLActor::GLDepthMaskOverride());
    }
  }

  this->Timer = nullptr;
  this->RenderState = nullptr;
  this->DeleteOcclusionQueryIds();
  this->SetCurrentStage(Inactive);

  if (this->CullFaceEnabled)
  {
    this->State->svtkglEnable(GL_CULL_FACE);
  }
  else
  {
    this->State->svtkglDisable(GL_CULL_FACE);
  }
  if (this->DepthTestEnabled)
  {
    this->State->svtkglEnable(GL_DEPTH_TEST);
  }
#ifdef DEBUG_FRAME
  std::cout << "Depth peel done:\n"
            << "  - Number of peels: " << this->CurrentPeel << "\n"
            << "  - Number of geometry passes: " << this->TranslucentRenderCount << "\n"
            << "  - Number of volume passes: " << this->VolumetricRenderCount << "\n"
            << "  - Occlusion Ratio: trans="
            << static_cast<float>(this->TranslucentWrittenPixels) /
      static_cast<float>(this->ViewportWidth * this->ViewportHeight)
            << " volume="
            << static_cast<float>(this->VolumetricWrittenPixels) /
      static_cast<float>(this->ViewportWidth * this->ViewportHeight)
            << " (target: " << this->OcclusionRatio << ")\n";
#endif // DEBUG_FRAME
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::AlphaBlendRender()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::AlphaBlendRender);

  /* This pass is mopping up the remaining fragments when we exceed the max
   * number of peels or hit the occlusion limit. We'll simply render all of the
   * remaining fragments into the back destination buffer using the
   * premultiplied-alpha over-blending equations:
   *
   * aC = f.a * f.rgb + (1 - f.a) * b.a * b.rgb
   * a = f.a + (1 - f.a) * b.a
   */
  this->State->svtkglEnable(GL_BLEND);
  this->State->svtkglBlendEquation(GL_FUNC_ADD);
  this->State->svtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  this->SetCurrentStage(AlphaBlending);
  this->ActivateDrawBuffer(Back);
  this->Textures[this->DepthSource]->Activate();

  if (this->TranslucentWrittenPixels > 0)
  {
    this->SetCurrentPeelType(TranslucentPeel);
    annotate("Alpha blend translucent render start");
    this->RenderTranslucentPass();
    annotate("Alpha blend translucent render end");
  }

  // Do not check VolumetricWrittenPixels to determine if alpha blending
  // volumes is needed -- there's no guarantee that a previous slice had
  // volume data if the current slice does.
  if (this->IsRenderingVolumes())
  {
    this->SetCurrentPeelType(VolumetricPeel);
    annotate("Alpha blend volumetric render start");
    this->RenderVolumetricPass();
    annotate("Alpha blend volumetric render end");
  }

  this->Textures[this->DepthSource]->Deactivate();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::BlendFinalImage()
{
  TIME_FUNCTION(svtkDualDepthPeelingPass::BlendFinalImage);

  this->Textures[this->FrontSource]->Activate();
  this->Textures[Back]->Activate();

  /* Peeling is done, time to blend the front and back peel textures with the
   * opaque geometry in the existing framebuffer. First, we'll underblend the
   * back texture beneath the front texture in the shader:
   *
   * Blend 'b' under 'f' to form 't':
   * t.rgb = f.a * b.a * b.rgb + f.rgb
   * t.a   = (1 - b.a) * f.a
   *
   * ( t = translucent layer (back + front), f = front layer, b = back layer )
   *
   * Also in the shader, we adjust the translucent layer's alpha so that it
   * can be used for back-to-front blending, so
   *
   * alphaOverBlend = 1. - alphaUnderBlend
   *
   * To blend the translucent layer over the opaque layer, use regular
   * overblending via glBlendEquation/glBlendFunc:
   *
   * Blend 't' over 'o'
   * C = t.rgb + o.rgb * (1 - t.a)
   * a = t.a + o.a * (1 - t.a)
   *
   * These blending parameters and fragment shader perform this work.
   * Note that the opaque fragments are assumed to have premultiplied alpha
   * in this implementation. */
  this->State->svtkglEnable(GL_BLEND);
  this->State->svtkglBlendEquation(GL_FUNC_ADD);
  this->State->svtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Restore the original viewport and scissor test settings (see note in
  // Prepare).
  this->State->svtkglViewport(
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  if (this->SaveScissorTestState)
  {
    this->State->svtkglEnable(GL_SCISSOR_TEST);
  }
  else
  {
    this->State->svtkglDisable(GL_SCISSOR_TEST);
  }

  svtkOpenGLRenderWindow* renWin =
    static_cast<svtkOpenGLRenderWindow*>(this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->BlendHelper)
  {
    std::string fragShader = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
    svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Decl",
      "uniform sampler2D frontTexture;\n"
      "uniform sampler2D backTexture;\n");
    svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Impl",
      "  vec4 front = texture2D(frontTexture, texCoord);\n"
      "  vec4 back = texture2D(backTexture, texCoord);\n"
      "  front.a = 1. - front.a; // stored as (1 - alpha)\n"
      "  // Underblend. Back color is premultiplied:\n"
      "  gl_FragData[0].rgb = (front.rgb + back.rgb * front.a);\n"
      "  // The first '1. - ...' is to convert the 'underblend' alpha to\n"
      "  // an 'overblend' alpha, since we'll be letting GL do the\n"
      "  // transparent-over-opaque blending pass.\n"
      "  gl_FragData[0].a = (1. - front.a * (1. - back.a));\n");
    this->BlendHelper = new svtkOpenGLQuadHelper(renWin, nullptr, fragShader.c_str(), nullptr);
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BlendHelper->Program);
  }

  if (!this->BlendHelper->Program)
  {
    return;
  }

  this->BlendHelper->Program->SetUniformi(
    "frontTexture", this->Textures[this->FrontSource]->GetTextureUnit());
  this->BlendHelper->Program->SetUniformi("backTexture", this->Textures[Back]->GetTextureUnit());

  annotate("blending final!");
  this->BlendHelper->Render();
  annotate("final blended!");

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[Back]->Deactivate();
}

//------------------------------------------------------------------------------
void svtkDualDepthPeelingPass::DeleteOcclusionQueryIds()
{
  glDeleteQueries(1, &this->TranslucentOcclusionQueryId);
  glDeleteQueries(1, &this->VolumetricOcclusionQueryId);
}
