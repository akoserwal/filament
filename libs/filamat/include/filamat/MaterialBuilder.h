/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TNT_FILAMAT_MATERIAL_PACKAGE_BUILDER_H
#define TNT_FILAMAT_MATERIAL_PACKAGE_BUILDER_H

#include <cstddef>
#include <cstdint>

#include <string>
#include <vector>

#include <filament/driver/DriverEnums.h>
#include <filament/EngineEnums.h>
#include <filament/MaterialEnums.h>

#include <filamat/Package.h>

#include <utils/bitset.h>
#include <utils/compiler.h>
#include <utils/CString.h>

namespace filamat {

// Shader postprocessor, called after generation of a shader but before writing it to the package.
// Must return false if an error occured while postProcessing the shader and true if everything was
// ok.
using PostProcessCallBack = std::function<bool(
        const std::string& /* inputShader */,
        filament::driver::ShaderType,
        filament::driver::ShaderModel,
        std::string* /* outputGlsl */,
        std::vector<uint32_t>* /* outputSpirv */ )>;

struct MaterialInfo;

class UTILS_PUBLIC MaterialBuilderBase {
public:
    // High-level hint that works in concert with TargetApi to determine the shader models
    // (used to generate GLSL) and final output representations (spirv and/or text).
    enum class Platform {
        DESKTOP,
        MOBILE,
        ALL
    };

    enum class TargetApi {
        ALL,
        OPENGL,
        VULKAN,
    };

protected:
    // Looks at platform and target API, then decides on shader models and output formats.
    void prepare();

    using ShaderModel = filament::driver::ShaderModel;
    Platform mPlatform = Platform::DESKTOP;
    TargetApi mTargetApi = TargetApi::OPENGL;
    TargetApi mCodeGenTargetApi = TargetApi::OPENGL;
    utils::bitset32 mShaderModels;
    struct CodeGenParams {
        int shaderModel;
        TargetApi targetApi;
        TargetApi codeGenTargetApi;
    };
    std::vector<CodeGenParams> mCodeGenPermutations;
    uint8_t mVariantFilter = 0;
};

class UTILS_PUBLIC MaterialBuilder : public MaterialBuilderBase {
public:
    MaterialBuilder();

    using Property = filament::Property;
    using Variable = filament::Variable;
    using BlendingMode = filament::BlendingMode;
    using Shading = filament::Shading;
    using Interpolation = filament::Interpolation;
    using VertexDomain = filament::VertexDomain;
    using TransparencyMode = filament::TransparencyMode;

    using UniformType = filament::driver::UniformType;
    using SamplerType = filament::driver::SamplerType;
    using SamplerFormat = filament::driver::SamplerFormat;
    using SamplerPrecision = filament::driver::Precision;
    using CullingMode = filament::driver::CullingMode;

    // Each shader generated while building the package content can be post-processed via this
    // callback.
    MaterialBuilder& postProcessor(PostProcessCallBack callback);

    // set name of this material
    MaterialBuilder& name(const char* name) noexcept;

    // set the shading model
    MaterialBuilder& shading(Shading shading) noexcept;

    // set the interpolation mode
    MaterialBuilder& interpolation(Interpolation interpolation) noexcept;

    // declares that this property is modified by the material
    MaterialBuilder& set(Property p) noexcept;

    // add a parameter (i.e.: a uniform) to this material
    MaterialBuilder& parameter(UniformType type, const char* name) noexcept;

    // add a parameter array to this material
    MaterialBuilder& parameter(UniformType type, size_t size, const char* name) noexcept;

    // add a sampler parameter to this material
    // When SamplerType::SAMPLER_EXTERNAL is specifed, format and precision are ignored
    MaterialBuilder& parameter(SamplerType samplerType, SamplerFormat format,
            SamplerPrecision precision, const char* name) noexcept;
    MaterialBuilder& parameter(SamplerType samplerType, SamplerFormat format,
            const char* name) noexcept;
    MaterialBuilder& parameter(SamplerType samplerType, SamplerPrecision precision,
            const char* name) noexcept;
    MaterialBuilder& parameter(SamplerType samplerType, const char* name) noexcept;

    // custom variables (all float4)
    MaterialBuilder& variable(Variable v, const char* name) noexcept;

    // require a specified attribute, position is always required and normal
    // depends on the shading model
    MaterialBuilder& require(filament::VertexAttribute attribute) noexcept;

    // set the code content of this material
    // must declare a function "void material(inout MaterialInputs material)"
    // this function *must* call "prepareMaterial(material)" before it returns
    MaterialBuilder& material(const char* code, size_t line = 0) noexcept;

    // set the vertex code content of this material
    // must declare a function "void materialVertex(inout MaterialVertexInputs material)"
    MaterialBuilder& materialVertex(const char* code, size_t line = 0) noexcept;

    // set blending mode for this material
    MaterialBuilder& blending(BlendingMode blending) noexcept;

    // set vertex domain for this material
    MaterialBuilder& vertexDomain(VertexDomain domain) noexcept;

    // how triangles are culled (doesn't affect points or lines, back-face culling by default)
    MaterialBuilder& culling(CullingMode culling) noexcept;

    // enable/disable color-buffer write (enabled by default)
    MaterialBuilder& colorWrite(bool enable) noexcept;

    // enable/disable depth-buffer write (enabled by default for opaque, disabled for others)
    MaterialBuilder& depthWrite(bool enable) noexcept;

    // enable/disable depth based culling (enabled by default)
    MaterialBuilder& depthCulling(bool enable) noexcept;

    // double-sided materials don't cull faces, equivalent to culling(CullingMode::NONE)
    // doubleSided() overrides culling() if called
    MaterialBuilder& doubleSided(bool doubleSided) noexcept;

    // any fragment with an alpha below this threshold is clipped (MASKED blending mode only)
    MaterialBuilder& maskThreshold(float threshold) noexcept;

    // the material output is multiplied by the shadowing factor (UNLIT model only)
    MaterialBuilder& shadowMultiplier(bool shadowMultiplier) noexcept;

    // specifies how transparent objects should be rendered (default is DEFAULT)
    MaterialBuilder& transparencyMode(TransparencyMode mode) noexcept;

    // specifies desktop vs mobile; works in concert with TargetApi to determine the shader models
    // (used to generate code) and final output representations (spirv and/or text).
    MaterialBuilder& platform(Platform platform) noexcept;

    // specifies vulkan vs opengl; works in concert with Platform to determine the shader models
    // (used to generate code) and final output representations (spirv and/or text).
    MaterialBuilder& targetApi(TargetApi targetApi) noexcept;

    // specifies vulkan vs opengl; this method can be used to override which target API is used
    // during the code generation step. This can be useful when the post-processor uses a
    // different intermediate representation.
    MaterialBuilder& codeGenTargetApi(TargetApi targetApi) noexcept;

    // specifies a list of variants that should be filtered out during code generation.
    MaterialBuilder& variantFilter(uint8_t variantFilter) noexcept;

    // build the material
    Package build() noexcept;

public:
    // The methods and types below are for internal use
    struct Parameter {
        Parameter() noexcept = default;
        Parameter(const char* paramName, SamplerType t, SamplerFormat f, SamplerPrecision p)
                : name(paramName), size(1), samplerType(t), samplerFormat(f), samplerPrecision(p),
                isSampler(true) { }
        Parameter(const char* paramName, UniformType t, size_t typeSize)
                : name(paramName), size(typeSize), uniformType(t), isSampler(false) { }
        utils::CString name;
        size_t size;
        union {
            UniformType uniformType;
            struct {
                SamplerType samplerType;
                SamplerFormat samplerFormat;
                SamplerPrecision samplerPrecision;
            };
        };
        bool isSampler;
    };

    // Preview the first shader that would generated in the MaterialPackage.
    // This is used to run Static Code Analysis before generating a package.
    // Outputs the chosen shader model in the model parameter
    const std::string peek(filament::driver::ShaderType type,
            filament::driver::ShaderModel& model) noexcept;

    // Returns true if any of the parameter samplers is of type samplerExternal
    bool hasExternalSampler() const noexcept;

    using PropertyList = bool[filament::MATERIAL_PROPERTIES_COUNT];
    using VariableList = utils::CString[filament::MATERIAL_VARIABLES_COUNT];

    static constexpr size_t MAX_PARAMETERS_COUNT = 32;
    using ParameterList = Parameter[MAX_PARAMETERS_COUNT];

    // returns the number of parameters declared in this material
    uint8_t getParameterCount() const noexcept { return mParameterCount; }

    // returns a list of at least getParameterCount() parameters
    const ParameterList& getParameters() const noexcept { return mParameters; }

    TargetApi getTargetApi() const { return mTargetApi; }

    Platform getPlatform() const { return mPlatform; }

    uint8_t getVariantFilter() const { return mVariantFilter; }

private:
    void prepareToBuild(MaterialInfo& info) noexcept;

    bool isLit() const noexcept { return mShading != filament::Shading::UNLIT; }

    utils::CString mMaterialName;

    utils::CString mMaterialCode;
    utils::CString mMaterialVertexCode;
    size_t mMaterialLineOffset = 0;
    size_t mMaterialVertexLineOffset = 0;

    PropertyList mProperties;
    ParameterList mParameters;
    VariableList mVariables;

    BlendingMode mBlendingMode = BlendingMode::OPAQUE;
    CullingMode mCullingMode = CullingMode::BACK;
    Shading mShading = Shading::LIT;
    Interpolation mInterpolation = Interpolation::SMOOTH;
    VertexDomain mVertexDomain = VertexDomain::OBJECT;
    TransparencyMode mTransparencyMode = TransparencyMode::DEFAULT;

    filament::AttributeBitset mRequiredAttributes;

    float mMaskThreshold = 0.4f;
    bool mShadowMultiplier = false;

    uint8_t mParameterCount = 0;

    bool mDoubleSided = false;
    bool mDoubleSidedSet = false;
    bool mColorWrite = true;
    bool mDepthTest = true;
    bool mDepthWrite = true;
    bool mDepthWriteSet = false;

    PostProcessCallBack mPostprocessorCallback = nullptr;
};

} // namespace filamat
#endif
