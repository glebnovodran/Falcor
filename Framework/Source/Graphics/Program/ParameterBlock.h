/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once
#include "Graphics/Program/ProgramReflection.h"
#include "API/ConstantBuffer.h"
#include "API/StructuredBuffer.h"
#include "API/TypedBuffer.h"

namespace Falcor
{
    class RootSignature;

    class ParameterBlock : public std::enable_shared_from_this<ParameterBlock>
    {
    public:
        template<typename T>
        class SharedPtrT : public std::shared_ptr<T>
        {
        public:
            SharedPtrT() : std::shared_ptr<T>() {}
            SharedPtrT(T* pParamBlock) : std::shared_ptr<T>(pParamBlock) {}
            ConstantBuffer::SharedPtr operator[](const std::string& cbName) const { return get()->getConstantBuffer(cbName); }
            ConstantBuffer::SharedPtr operator[](uint32_t index) = delete; // No set by index. This is here because if we didn't explicitly delete it, the compiler will try to convert to int into a string, resulting in runtime error
        };

        using SharedPtr = SharedPtrT<ParameterBlock>;
        using SharedConstPtr = SharedPtrT<const ParameterBlock>;
        ~ParameterBlock();

        /** Create a new object
        */
        static SharedPtr create(const ParameterBlockReflection::SharedConstPtr& pReflection, const RootSignature* pRootSig, bool createBuffers);

        /** Bind a constant buffer object by name.
        If the name doesn't exists or the CBs size doesn't match the required size, the call will fail.
        If a buffer was previously bound it will be released.
        \param[in] name The name of the constant buffer in the program
        \param[in] pCB The constant buffer object
        \return false is the call failed, otherwise true
        */
        bool setConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB);

        /** Bind a constant buffer object by index.
        If the no CB exists in the specified index or the CB size doesn't match the required size, the call will fail.
        If a buffer was previously bound it will be released.
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \param[in] pCB The constant buffer object
        \return false is the call failed, otherwise true
        */
        bool setConstantBuffer(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const ConstantBuffer::SharedPtr& pCB);

        /** Get a constant buffer object.
        \param[in] name The name of the buffer
        \return If the name is valid, a shared pointer to the CB. Otherwise returns nullptr
        */
        ConstantBuffer::SharedPtr getConstantBuffer(const std::string& name) const;

        /** Get a constant buffer object.
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \return If the indices is valid, a shared pointer to the buffer. Otherwise returns nullptr
        */
        ConstantBuffer::SharedPtr getConstantBuffer(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const;

        /** Set a raw-buffer. Based on the shader reflection, it will be bound as either an SRV or a UAV
        \param[in] name The name of the buffer
        \param[in] pBuf The buffer object
        */
        bool setRawBuffer(const std::string& name, Buffer::SharedPtr pBuf);

        /** Set a typed buffer. Based on the shader reflection, it will be bound as either an SRV or a UAV
        \param[in] name The name of the buffer
        \param[in] pBuf The buffer object
        */
        bool setTypedBuffer(const std::string& name, TypedBufferBase::SharedPtr pBuf);
        
        /** Set a structured buffer. Based on the shader reflection, it will be bound as either an SRV or a UAV
        \param[in] name The name of the buffer
        \param[in] pBuf The buffer object
        */
        bool setStructuredBuffer(const std::string& name, StructuredBuffer::SharedPtr pBuf);

        /** Get a raw-buffer object.
        \param[in] name The name of the buffer
        \return If the name is valid, a shared pointer to the buffer object. Otherwise returns nullptr
        */
        Buffer::SharedPtr getRawBuffer(const std::string& name) const;

        /** Get a typed buffer object.
        \param[in] name The name of the buffer
        \return If the name is valid, a shared pointer to the buffer object. Otherwise returns nullptr
        */
        TypedBufferBase::SharedPtr getTypedBuffer(const std::string& name) const;

        /** Get a structured buffer object.
        \param[in] name The name of the buffer
        \return If the name is valid, a shared pointer to the buffer object. Otherwise returns nullptr
        */
        StructuredBuffer::SharedPtr getStructuredBuffer(const std::string& name) const;

        /** Bind a texture. Based on the shader reflection, it will be bound as either an SRV or a UAV
        \param[in] name The name of the texture object in the shader
        \param[in] pTexture The texture object to bind
        */
        bool setTexture(const std::string& name, const Texture::SharedPtr& pTexture);

        /** Get a texture object.
        \param[in] name The name of the texture
        \return If the name is valid, a shared pointer to the texture object. Otherwise returns nullptr
        */
        Texture::SharedPtr getTexture(const std::string& name) const;

        /** Bind an SRV.
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \param[in] pSrv The shader-resource-view object to bind
        */
        bool setSrv(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const ShaderResourceView::SharedPtr& pSrv);

        /** Bind a UAV.
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \param[in] pSrv The unordered-access-view object to bind
        */
        bool setUav(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const UnorderedAccessView::SharedPtr& pUav);

        /** Get an SRV object.
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \return If the indices is valid, a shared pointer to the SRV. Otherwise returns nullptr
        */
        ShaderResourceView::SharedPtr getSrv(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const;

        /** Get a UAV object
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \return If the index is valid, a shared pointer to the UAV. Otherwise returns nullptr
        */
        UnorderedAccessView::SharedPtr getUav(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const;

        /** Bind a sampler to the program in the global namespace.
        \param[in] name The name of the sampler object in the shader
        \param[in] pSampler The sampler object to bind
        \return false if the sampler was not found in the program, otherwise true
        */
        bool setSampler(const std::string& name, const Sampler::SharedPtr& pSampler);

        /** Bind a sampler to the program in the global namespace.
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \return false if the sampler was not found in the program, otherwise true
        */
        bool setSampler(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const Sampler::SharedPtr& pSampler);

        /** Gets a sampler object.
        \return If the index is valid, a shared pointer to the sampler. Otherwise returns nullptr
        */
        Sampler::SharedPtr getSampler(const std::string& name) const;

        /** Gets a sampler object.
        \param[in] setIndex The set-index in the block
        \param[in] descIndex The descriptor index inside the set
        \return If the index is valid, a shared pointer to the sampler. Otherwise returns nullptr
        */
        Sampler::SharedPtr getSampler(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const;

        /** Get the program reflection interface
        */
        ParameterBlockReflection::SharedConstPtr getReflection() const { return mpReflector; }

        bool prepareForDraw(CopyContext* pContext, const RootSignature* pRootSig);

        struct RootData
        {
            RootData() = default;
            RootData(uint32_t root, uint32_t range) : rootIndex(root), rangeIndex(range) {}
            uint32_t rootIndex = uint32_t(-1);
            uint32_t rangeIndex = uint32_t(-1);
        };

        template<typename ViewType>
        struct ResourceData
        {
            ResourceData(const RootData& data) : rootData(data) {}
            typename ViewType::SharedPtr pView = nullptr;
            Resource::SharedPtr pResource = nullptr;
            RootData rootData;
        };

        template<>
        struct ResourceData<Sampler>
        {
            ResourceData(const RootData& data) : rootData(data) {}
            Sampler::SharedPtr pSampler;
            RootData rootData;
        };


        struct RootSet
        {
            mutable std::shared_ptr<DescriptorSet> pDescSet;
            mutable bool dirty = false;
        };

        union BindLocation
        {
            BindLocation() = default;
            BindLocation(uint32_t space, uint32_t index) : baseRegIndex(index), regSpace(space) {}
            struct
            {
                uint32_t baseRegIndex;
                uint32_t regSpace;
            };
            uint64_t u64 = -1;
            std::size_t operator()(BindLocation b) const { return std::hash<uint64_t>{}(u64); }
            bool operator==(const BindLocation& other) const { return u64 == other.u64; }
        };

        template<typename T>
        using ResourceMap = std::unordered_map<BindLocation, std::vector<ResourceData<T>>, BindLocation>;
        using SamplerMap = std::unordered_map<BindLocation, std::vector<Sampler::SharedConstPtr>, BindLocation>;
        using RootSetVec = std::vector<RootSet>;

        const ResourceMap<ConstantBuffer>& getAssignedCbs() const { return mAssignedCbs; }
        const ResourceMap<ShaderResourceView>& getAssignedSrvs() const { return mAssignedSrvs; }
        const ResourceMap<UnorderedAccessView>& getAssignedUavs() const { return mAssignedUavs; }
        const ResourceMap<Sampler>& getAssignedSamplers() const { return mAssignedSamplers; }
        const RootSetVec getRootSets() const { return mRootSets; }

        // Delete some functions. If they are not deleted, the compiler will try to convert the uints to string, resulting in runtime error
        Sampler::SharedPtr getSampler(uint32_t) const = delete;
        bool setSampler(uint32_t, const Sampler::SharedPtr&) = delete;
        bool setConstantBuffer(uint32_t, const ConstantBuffer::SharedPtr&) = delete;
        ConstantBuffer::SharedPtr getConstantBuffer(uint32_t) const = delete;
    private:
        ParameterBlock(const ParameterBlockReflection::SharedConstPtr& pReflection, const RootSignature* pRootSig, bool createBuffers);
        ParameterBlockReflection::SharedConstPtr mpReflector;

        ResourceMap<ConstantBuffer> mAssignedCbs;        // HLSL 'b' registers
        ResourceMap<ShaderResourceView> mAssignedSrvs;   // HLSL 't' registers
        ResourceMap<UnorderedAccessView> mAssignedUavs;  // HLSL 'u' registers
        ResourceMap<Sampler> mAssignedSamplers;    // HLSL 's' registers

        RootSetVec mRootSets;
    };
}