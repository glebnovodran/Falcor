/***************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
#include "stdafx.h"
#include "ProgramVars.h"
#include "GraphicsProgram.h"
#include "ComputeProgram.h"
#include "Core/API/ComputeContext.h"
#include "Core/API/RenderContext.h"

namespace Falcor
{
    static bool compareRootSets(const DescriptorSet::Layout& a, const DescriptorSet::Layout& b)
    {
        if (a.getRangeCount() != b.getRangeCount()) return false;
        if (a.getVisibility() != b.getVisibility()) return false;
        for (uint32_t i = 0; i < a.getRangeCount(); i++)
        {
            const auto& rangeA = a.getRange(i);
            const auto& rangeB = b.getRange(i);
            if (rangeA.baseRegIndex != rangeB.baseRegIndex) return false;
            if (rangeA.descCount != rangeB.descCount) return false;
#ifdef FALCOR_D3D12
            if (rangeA.regSpace != rangeB.regSpace) return false;
#endif
            if (rangeA.type != rangeB.type) return false;
        }
        return true;
    }

    static uint32_t findRootIndex(const DescriptorSet::Layout& blockSet, const RootSignature::SharedPtr& pRootSig)
    {
        for (uint32_t i = 0; i < pRootSig->getDescriptorSetCount(); i++)
        {
            const auto& rootSet = pRootSig->getDescriptorSet(i);
            if (compareRootSets(rootSet, blockSet))
            {
#ifdef FALCOR_D3D12
                return i;
#else
                return rootSet.getRange(0).regSpace;
#endif
            }
        }
        should_not_get_here();
        return -1;
    }

    ProgramVars::BlockData ProgramVars::initParameterBlock(const ParameterBlockReflection::SharedConstPtr& pBlockReflection, bool createBuffers)
    {
        BlockData data;
        data.pBlock = ParameterBlock::create(pBlockReflection, createBuffers);
        // For each set, find the matching root-index. 
        const auto& sets = pBlockReflection->getDescriptorSetLayouts();
        data.rootIndex.resize(sets.size());
        for (size_t i = 0; i < sets.size(); i++)
        {
            data.rootIndex[i] = findRootIndex(sets[i], mpRootSignature);
        }

        return data;
    }

    ProgramVars::ProgramVars(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig) : mpReflector(pReflector)
    {
        assert(pReflector);
        mpRootSignature = pRootSig ? pRootSig : RootSignature::create(pReflector.get());
        ParameterBlockReflection::SharedConstPtr pDefaultBlock = pReflector->getDefaultParameterBlock();
        // Initialize the global-block first so that it's the first entry in the vector
        for (uint32_t i = 0; i < pReflector->getParameterBlockCount(); i++)
        {
            const auto& pBlock = pReflector->getParameterBlock(i);
            BlockData data = initParameterBlock(pBlock, createBuffers);
            mParameterBlocks.push_back(data);
        }
        mDefaultBlock = mParameterBlocks[mpReflector->getParameterBlockIndex("")];
    }

    ParameterBlock::SharedPtr ProgramVars::getParameterBlock(const std::string& name) const
    {
        uint32_t index = mpReflector->getParameterBlockIndex(name);
        if (index == ProgramReflection::kInvalidLocation)
        {
            logWarning("Can't find parameter block named " + name + ". Ignoring getParameterBlock() call");
            return nullptr;
        }
        return mParameterBlocks[index].pBlock;
    }

    ParameterBlock::SharedPtr ProgramVars::getParameterBlock(uint32_t blockIndex) const
    {
        return (blockIndex < mParameterBlocks.size()) ? mParameterBlocks[blockIndex].pBlock : nullptr;
    }

    bool ProgramVars::setParameterBlock(const std::string& name, const std::shared_ptr<ParameterBlock>& pBlock)
    {
        uint32_t index = mpReflector->getParameterBlockIndex(name);
        if (index == ProgramReflection::kInvalidLocation)
        {
            logWarning("Can't find parameter block named " + name + ". Ignoring setParameterBlock() call");
            return false;
        }
        mParameterBlocks[index].bind = true;
        mParameterBlocks[index].pBlock = pBlock ? pBlock : ParameterBlock::create(mpReflector->getParameterBlock(index), true);
        return true;
    }

    bool ProgramVars::setParameterBlock(uint32_t blockIndex, const std::shared_ptr<ParameterBlock>& pBlock)
    {
        if (blockIndex >= mParameterBlocks.size())
        {
            logWarning("setParameterBlock() - block index out-of-bounds");
            return false;
        }
        mParameterBlocks[blockIndex].bind = true;
        mParameterBlocks[blockIndex].pBlock = pBlock ? pBlock : ParameterBlock::create(mpReflector->getParameterBlock(blockIndex), true);
        return true;
    }

    GraphicsVars::SharedPtr GraphicsVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig)
    {
        return SharedPtr(new GraphicsVars(pReflector, createBuffers, pRootSig));
    }

    GraphicsVars::SharedPtr GraphicsVars::create(const GraphicsProgram* pProg)
    {
        return create(pProg->getReflector());
    }

    ComputeVars::SharedPtr ComputeVars::create(const ProgramReflection::SharedConstPtr& pReflector, bool createBuffers, const RootSignature::SharedPtr& pRootSig)
    {
        return SharedPtr(new ComputeVars(pReflector, createBuffers, pRootSig));
    }

    ComputeVars::SharedPtr ComputeVars::create(const ComputeProgram* pProg)
    {
        return create(pProg->getReflector());
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(const std::string& name) const
    {
        return mDefaultBlock.pBlock->getConstantBuffer(name);
    }

    ConstantBuffer::SharedPtr ProgramVars::getConstantBuffer(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Cbv);
        return mDefaultBlock.pBlock->getConstantBuffer(loc, arrayIndex);
    }

    bool ProgramVars::setConstantBuffer(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const ConstantBuffer::SharedPtr& pCB)
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Cbv);
        return mDefaultBlock.pBlock->setConstantBuffer(loc, arrayIndex, pCB);
    }

    bool ProgramVars::setConstantBuffer(const std::string& name, const ConstantBuffer::SharedPtr& pCB)
    {
        return mDefaultBlock.pBlock->setConstantBuffer(name, pCB);
    }

    bool ProgramVars::setRawBuffer(const std::string& name, const Buffer::SharedPtr& pBuf)
    {
        return mDefaultBlock.pBlock->setRawBuffer(name, pBuf);
    }

    bool ProgramVars::setTypedBuffer(const std::string& name, const TypedBufferBase::SharedPtr& pBuf)
    {
        return mDefaultBlock.pBlock->setTypedBuffer(name, pBuf);
    }
    
    bool ProgramVars::setStructuredBuffer(const std::string& name, const StructuredBuffer::SharedPtr& pBuf)
    {
        return mDefaultBlock.pBlock->setStructuredBuffer(name, pBuf);
    }
    
    Buffer::SharedPtr ProgramVars::getRawBuffer(const std::string& name) const
    {
        return mDefaultBlock.pBlock->getRawBuffer(name);
    }

    TypedBufferBase::SharedPtr ProgramVars::getTypedBuffer(const std::string& name) const
    {
        return mDefaultBlock.pBlock->getTypedBuffer(name);
    }

    StructuredBuffer::SharedPtr ProgramVars::getStructuredBuffer(const std::string& name) const
    {
        return mDefaultBlock.pBlock->getStructuredBuffer(name);
    }

    bool ProgramVars::setSampler(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const Sampler::SharedPtr& pSampler)
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Sampler);
        return mDefaultBlock.pBlock->setSampler(loc, arrayIndex, pSampler);
    }

    bool ProgramVars::setSampler(const std::string& name, const Sampler::SharedPtr& pSampler)
    {
        return mDefaultBlock.pBlock->setSampler(name, pSampler);
    }

    Sampler::SharedPtr ProgramVars::getSampler(const std::string& name) const
    {
        return mDefaultBlock.pBlock->getSampler(name);
    }

    Sampler::SharedPtr ProgramVars::getSampler(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Sampler);
        return mDefaultBlock.pBlock->getSampler(loc, arrayIndex);
    }

    ShaderResourceView::SharedPtr ProgramVars::getSrv(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Srv);
        return mDefaultBlock.pBlock->getSrv(loc, arrayIndex);
    }

    UnorderedAccessView::SharedPtr ProgramVars::getUav(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex) const
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Uav);
        return mDefaultBlock.pBlock->getUav(loc, arrayIndex);
    }

    bool ProgramVars::setTexture(const std::string& name, const Texture::SharedPtr& pTexture)
    {
        return mDefaultBlock.pBlock->setTexture(name, pTexture);
    }

    Texture::SharedPtr ProgramVars::getTexture(const std::string& name) const
    {
        return mDefaultBlock.pBlock->getTexture(name);
    }

    bool ProgramVars::setSrv(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const ShaderResourceView::SharedPtr& pSrv)
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Srv);
        return mDefaultBlock.pBlock->setSrv(loc, arrayIndex, pSrv);
    }
    
    bool ProgramVars::setUav(uint32_t regSpace, uint32_t baseRegIndex, uint32_t arrayIndex, const UnorderedAccessView::SharedPtr& pUav)
    {
        const auto& loc = mpReflector->translateRegisterIndicesToBindLocation(regSpace, baseRegIndex, ProgramReflection::BindType::Uav);
        return mDefaultBlock.pBlock->setUav(loc, arrayIndex, pUav);
    }

    template<bool forGraphics>
    bool ProgramVars::bindRootSetsCommon(CopyContext* pContext, bool bindRootSig)
    {
        // Bind the sets
        for(uint32_t b = 0 ; b < getParameterBlockCount() ; b++)
        {
            ParameterBlock* pBlock = mParameterBlocks[b].pBlock.get(); // #PARAMBLOCK getParameterBlock() because we don't want the user to change blocks directly, but we need it non-const here
            if (pBlock->prepareForDraw(pContext) == false) return false; // #PARAMBLOCK Get rid of it. getRootSets() should have a dirty flag

            const auto& rootIndices = mParameterBlocks[b].rootIndex;
            auto& rootSets = pBlock->getRootSets();
            bool forceBind = bindRootSig || mParameterBlocks[b].bind;
            mParameterBlocks[b].bind = false;

            for (uint32_t s = 0; s < rootSets.size(); s++)
            {
                if (rootSets[s].dirty || forceBind)
                {
                    rootSets[s].dirty = false;
                    uint32_t rootIndex = rootIndices[s];
                    if (forGraphics)
                    {
                        rootSets[s].pSet->bindForGraphics(pContext, mpRootSignature.get(), rootIndex);
                    }
                    else
                    {
                        rootSets[s].pSet->bindForCompute(pContext, mpRootSignature.get(), rootIndex);
                    }
                }
            }
        }
        return true;
    }

    template<bool forGraphics>
    bool ProgramVars::applyProgramVarsCommon(CopyContext* pContext, bool bindRootSig)
    {
        if (bindRootSig)
        {
            if (forGraphics)
            {
                mpRootSignature->bindForGraphics(pContext);
            }
            else
            {
                mpRootSignature->bindForCompute(pContext);
            }
        }

        return bindRootSetsCommon<forGraphics>(pContext, bindRootSig);
    }


    bool ComputeVars::apply(ComputeContext* pContext, bool bindRootSig)
    {
        return applyProgramVarsCommon<false>(pContext, bindRootSig);
    }

    bool GraphicsVars::apply(RenderContext* pContext, bool bindRootSig)
    {
        return applyProgramVarsCommon<true>(pContext, bindRootSig);
    }
}
