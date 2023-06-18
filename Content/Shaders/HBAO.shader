---
includes:
- Shaders/Constants.glsl
- Shaders/Math.glsl

glslCommon: |
  #version 460
  #extension GL_ARB_separate_shader_objects : enable
  #extension GL_EXT_control_flow_attributes : enable
  
glslVertex: |
  layout(location=DefaultPositionBinding) in vec3 inPosition;
  layout(location=DefaultTexcoordBinding) in vec2 inTexcoord;
  
  layout(location=0) out vec2 fragTexcoord;
  layout(set=1, binding=1) uniform sampler2D depthSampler;
  //layout(set=1, binding=2) uniform sampler2D linearDepthSampler;
  
  void main() 
  {
      gl_Position = vec4(inPosition, 1);
      fragTexcoord = inTexcoord;
  }
    
glslFragment: |
  layout(set = 0, binding = 0) uniform FrameData
  {
      mat4 view;
      mat4 projection;
      mat4 invProjection;
      vec4 cameraPosition;
      ivec2 viewportSize;
      vec2 cameraZNearZFar;
      float currentTime;
      float deltaTime;
  } frame;
  
  layout(set=1, binding=0) uniform PostProcessDataUBO
  {
    float occlusionRadius;
    float occlusionPower;
    float occlusionAttenuation;
    float occlusionBias;
  } data;
  
  layout(set=1, binding=1) uniform sampler2D depthSampler;  
  
  layout(location=0) in vec2 fragTexcoord;
  layout(location=0) out vec4 outColor;
 
  const uint NumDirections = 8;
  const uint NumSamples = 8;
   const float OcclusionOffset = 0.001f;
  
  const vec2 Directions[8] = 
  {
    {0.0, 1.0},
    {1.0, 0.0},
    {0.0, -1.0},
    {-1.0, 0.0},
    {-0.7071069, 0.7071068},
    {0.7071068, 0.7071069},
    {0.7071069, -0.7071068},
    {-0.7071068, -0.7071069},
  };
  
  float TakeSmallerAbsDelta(float Left, float Mid, float Right)
  {
    float A = Mid - Left;
    float B = Right - Mid;
    return (abs(A) < abs(B)) ? A : B;
  }

  vec3 GetViewSpacePos(vec2 UV)
  {
    float Depth = texture(depthSampler, UV).r;
    return ClipSpaceToViewSpace(vec4(UV.x, UV.y, Depth, 1.0f), frame.invProjection).xyz;
  }
  
  vec3 GetViewSpaceNormal(vec2 UV, vec2 depthPixelSize)
  {   
    vec2 InvDepthPixelSize = rcp(depthPixelSize);
    
    vec2 UVLeft     = UV + vec2(-1.0, 0.0 ) * InvDepthPixelSize.xy;
    vec2 UVRight    = UV + vec2(1.0,  0.0 ) * InvDepthPixelSize.xy;
    vec2 UVDown     = UV + vec2(0.0,  -1.0) * InvDepthPixelSize.xy;
    vec2 UVUp       = UV + vec2(0.0,  1.0 ) * InvDepthPixelSize.xy;

    float Depth      = texture(depthSampler, UV).r;
    float DepthLeft  = texture(depthSampler, UVLeft).r;
    float DepthRight = texture(depthSampler, UVRight).r;
    float DepthDown  = texture(depthSampler, UVDown).r;
    float DepthUp    = texture(depthSampler, UVUp).r;

    float DepthDdx = TakeSmallerAbsDelta(DepthLeft, Depth, DepthRight);
    float DepthDdy = TakeSmallerAbsDelta(DepthDown, Depth, DepthUp);

    vec4 Mid    =  ClipSpaceToViewSpace(vec4(UV.x, UV.y, Depth, 1.0f), frame.invProjection);
    vec4 Right  =  ClipSpaceToViewSpace(vec4(UVRight.x, UVRight.y, Depth + DepthDdx, 1.0f), frame.invProjection) - Mid;
    vec4 Up     =  ClipSpaceToViewSpace(vec4(UVUp.x, UVUp.y, Depth + DepthDdy, 1.0f), frame.invProjection) - Mid;

    return normalize(cross(Right.xyz, Up.xyz));
  }
  
  vec2 SnapTexel(vec2 uv, vec2 depthPixelSize)
  {
     return round(uv * depthPixelSize) * (1.0f / depthPixelSize);
  }
  
  float SampleAO(inout float SinH, vec3 ViewSpaceSamplePos, vec3 ViewSpaceOriginPos, vec3 ViewSpaceOriginNormal)
  {
    vec3 HorizonVector = ViewSpaceSamplePos - ViewSpaceOriginPos;
    float HorizonVectorLength = length(HorizonVector);

    vec3 ViewSpaceSampleTangent = HorizonVector;

    float Occlusion = 0.0f;

    float SinS = sin((PI / 2.0) - acos(dot(ViewSpaceOriginNormal, normalize(ViewSpaceSampleTangent))));

    if(HorizonVectorLength < data.occlusionRadius * data.occlusionRadius && SinS > (SinH + data.occlusionBias * 3))
    {
        float FalloffZ = 1 - saturate(abs(HorizonVector.z) * 0.005);
        float DistanceFactor = 1 - HorizonVectorLength * rcp(data.occlusionRadius * data.occlusionRadius) * rcp(data.occlusionAttenuation);

        Occlusion = (SinS - SinH) * DistanceFactor * FalloffZ;
        SinH = SinS;
    }

    return Occlusion;
  }
  
  float SampleRayAO(
    vec2 RayOrigin,
    vec2 Direction,
    float Jitter,
    vec2 SampleRadius,
    vec3 ViewSpaceOriginPos,
    vec3 ViewSpaceOriginNormal,
    vec2 depthPixelSize
    )
  {
    // calculate the nearest neighbour sample along the direction vector
    vec2 SingleTexelStep = Direction * rcp(depthPixelSize);
    Direction *= SampleRadius;

    // jitter the starting position for ray marching between the nearest neighbour and the sample step size
    vec2 StepUV = SnapTexel(Direction * rcp(NumSamples + 1.0f), depthPixelSize);
    vec2 JitteredOffset = mix(SingleTexelStep, StepUV, Jitter);
    vec2 RayStart = SnapTexel(RayOrigin + JitteredOffset, depthPixelSize);
    vec2 RayEnd = RayStart + Direction;

    // top occlusion keeps track of the occlusion contribution of the last found occluder.
    // set to OcclusionBias value to avoid near-occluders
    float Occlusion = 0.0;

    float SinH = data.occlusionBias;

    [[unroll]]
    for (uint Step = 0; Step < NumSamples; ++Step)
    {
        vec2 UV = SnapTexel(mix(RayStart, RayEnd, Step / float(NumSamples)), depthPixelSize);
        vec3 ViewSpaceSamplePos = GetViewSpacePos(UV);

        Occlusion += SampleAO(SinH, ViewSpaceSamplePos, ViewSpaceOriginPos, ViewSpaceOriginNormal);
    }

    return Occlusion;
  }


  void main()
  {
    vec3 ViewSpacePosition = GetViewSpacePos(fragTexcoord);
    
    // sky dome check
    if (ViewSpacePosition.z > 49000)
    {
        outColor = vec4(1,0,0,1);
        return;
    }
    
    const vec2 depthTextureSize = textureSize(depthSampler, 0);
    vec3 ViewSpaceNormal = normalize(GetViewSpaceNormal(fragTexcoord, depthTextureSize));
    
    ViewSpacePosition += ViewSpaceNormal * OcclusionOffset * (1  + 0.1 * ViewSpacePosition.z / frame.cameraZNearZFar.x);
    
    outColor.xyz = ViewSpaceNormal;
    
     //outColor = texture(depthSampler, fragTexcoord);
  }