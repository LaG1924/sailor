includes:
- Shaders/Constants.glsl
- Shaders/Math.glsl
- Shaders/Lighting.glsl 
defines: []
glslCommon: |
  #version 450 core
  #extension GL_ARB_separate_shader_objects : enable
  #extension GL_EXT_shader_atomic_float : enable
glslCompute: |
  // Pre-filters environment cube map using GGX NDF importance sampling.
  // Part of specular IBL split-sum approximation.
  const float Epsilon = 0.00001;
  
  const uint NumSamples = 1024;
  const float InvNumSamples = 1.0 / float(NumSamples);
  
  //layout(constant_id=0) const int NumMipLevels = 1;
  const int NumMipLevels = 9;
  layout(set=0, binding=0) uniform samplerCube rawEnvMap;
  layout(set=0, binding=1, rgba16f) restrict writeonly uniform imageCube envMap[NumMipLevels];
  
  layout(push_constant) uniform PushConstants
  {
    // Output texture mip level (without base mip level).
    int level;
    // Roughness value to pre-filter for.
    float roughness;
  } pushConstants;
  
  // Sample i-th point from Hammersley point set of NumSamples points total.
  vec2 SampleHammersley(uint i)
  {
    return vec2(i * InvNumSamples, RadicalInverse_VdC(i));
  }
  
  // Calculate normalized sampling direction vector based on current fragment coordinates (gl_GlobalInvocationID.xyz).
  // This is essentially "inverse-sampling": we reconstruct what the sampling vector would be if we wanted it to "hit"
  // this particular fragment in a cubemap.
  // See: OpenGL core profile specs, section 8.13.
  vec3 GetSamplingVector()
  {
      vec2 st = gl_GlobalInvocationID.xy/vec2(imageSize(envMap[pushConstants.level]));
      vec2 uv = 2.0 * vec2(st.x, 1.0-st.y) - vec2(1.0);
  
      vec3 ret;
      // Sadly 'switch' doesn't seem to work, at least on NVIDIA.
      if(gl_GlobalInvocationID.z == 0)      ret = vec3(1.0,  uv.y, -uv.x);
      else if(gl_GlobalInvocationID.z == 1) ret = vec3(-1.0, uv.y,  uv.x);
      else if(gl_GlobalInvocationID.z == 2) ret = vec3(uv.x, 1.0, -uv.y);
      else if(gl_GlobalInvocationID.z == 3) ret = vec3(uv.x, -1.0, uv.y);
      else if(gl_GlobalInvocationID.z == 4) ret = vec3(uv.x, uv.y, 1.0);
      else if(gl_GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y, -1.0);
      return normalize(ret);
  }
  
  // Compute orthonormal basis for converting from tanget/shading space to world space.
  void ComputeBasisVectors(const vec3 N, out vec3 S, out vec3 T)
  {
    // Branchless select non-degenerate T.
    T = cross(N, vec3(0.0, 1.0, 0.0));
    T = mix(cross(N, vec3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));
  
    T = normalize(T);
    S = normalize(cross(N, T));
  }
  
  // Convert point from tangent/shading space to world space.
  vec3 TangentToWorld(const vec3 v, const vec3 N, const vec3 S, const vec3 T)
  {
    return S * v.x + T * v.y + N * v.z;
  }
  
  layout(local_size_x=32, local_size_y=32, local_size_z=1) in;
  void main(void)
  {
    // Make sure we won't write past output when computing higher mipmap levels.
    ivec2 outputSize = imageSize(envMap[pushConstants.level]);
    if(gl_GlobalInvocationID.x >= outputSize.x || gl_GlobalInvocationID.y >= outputSize.y) 
    {
      return;
    }
    
    // Solid angle associated with a single cubemap texel at zero mipmap level.
    // This will come in handy for importance sampling below.
    vec2 inputSize = vec2(textureSize(rawEnvMap, 0));
    float wt = 4.0 * PI / (6 * inputSize.x * inputSize.y);
    
    // Approximation: Assume zero viewing angle (isotropic reflections).
    vec3 N = GetSamplingVector();
    vec3 Lo = N;
    
    vec3 S, T;
    ComputeBasisVectors(N, S, T);
  
    vec3 color = vec3(0);
    float weight = 0;
  
    // Convolve environment map using GGX NDF importance sampling.
    // Weight by cosine term since Epic claims it generally improves quality.
    for(uint i = 0; i < NumSamples; ++i) 
    {
      vec2 u = SampleHammersley(i);
      vec3 Lh = TangentToWorld(SampleGGX(u.x, u.y, pushConstants.roughness), N, S, T);
  
      // Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
      vec3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;
  
      float cosLi = dot(N, Li);
      if(cosLi > 0.0) 
      {
        // Use Mipmap Filtered Importance Sampling to improve convergence.
        // See: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html, section 20.4
  
        float cosLh = max(dot(N, Lh), 0.0);
  
        // GGX normal distribution function (D term) probability density function.
        // Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V, rest of the scaling factor cancels out).
        float pdf = NdfGGX(cosLh, pushConstants.roughness) * 0.25;
  
        // Solid angle associated with this sample.
        float ws = 1.0 / (NumSamples * pdf);
  
        // Mip level to sample from.
        float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);
  
        color  += textureLod(rawEnvMap, Li, mipLevel).rgb * cosLi;
        weight += cosLi;
      }
    }
    color /= weight;
  
    imageStore(envMap[pushConstants.level], ivec3(gl_GlobalInvocationID), vec4(color, 1.0));
  }
  