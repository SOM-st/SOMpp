/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "VMMethod.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <queue>
#include <string>

#include "../compiler/BytecodeGenerator.h"
#include "../compiler/Disassembler.h"
#include "../compiler/LexicalScope.h"
#include "../compiler/MethodGenerationContext.h"
#include "../compiler/Variable.h"
#include "../interpreter/Interpreter.h"
#include "../interpreter/bytecodes.h"
#include "../memory/Heap.h"
#include "../misc/Murmur3Hash.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "ObjectFormats.h"
#include "Signature.h"
#include "VMClass.h"
#include "VMFrame.h"
#include "VMObject.h"
#include "VMSymbol.h"

VMMethod::VMMethod(VMSymbol* signature, size_t bcCount,
                   size_t numberOfConstants, size_t numLocals,
                   size_t maxStackDepth, LexicalScope* lexicalScope,
                   BackJump* inlinedLoops)
    : VMInvokable(signature), numberOfLocals(numLocals),
      maximumNumberOfStackElements(maxStackDepth), bcLength(bcCount),
      numberOfArguments(signature == nullptr
                            ? 0
                            : Signature::GetNumberOfArguments(signature)),
      numberOfConstants(numberOfConstants), lexicalScope(lexicalScope),
      inlinedLoops(inlinedLoops) {
#ifdef UNSAFE_FRAME_OPTIMIZATION
    cachedFrame = nullptr;
#endif

    indexableFields = (gc_oop_t*)(&indexableFields + 2);
    for (size_t i = 0; i < numberOfConstants; ++i) {
        indexableFields[i] = nilObject;
    }
    bytecodes = (uint8_t*)(&indexableFields + 2 + GetNumberOfIndexableFields());

    write_barrier(this, signature);
}

VMMethod* VMMethod::CloneForMovingGC() const {
    auto* clone =
        new (GetHeap<HEAP_CLS>(),
             GetObjectSize() - sizeof(VMMethod) ALLOC_MATURE) VMMethod(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this, sizeof(VMObject)),
           GetObjectSize() - sizeof(VMObject));
    clone->indexableFields = (gc_oop_t*)(&(clone->indexableFields) + 2);

    size_t const numIndexableFields = GetNumberOfIndexableFields();
    clone->bytecodes =
        (uint8_t*)(&(clone->indexableFields) + 2 + numIndexableFields);

    // Use of GetNumberOfIndexableFields() is problematic here, because it may
    // be invalid object while cloning/moving within GC
    return clone;
}

void VMMethod::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);

#ifdef UNSAFE_FRAME_OPTIMIZATION
    if (cachedFrame != nullptr) {
        cachedFrame = static_cast<VMFrame*>(walk(cachedFrame));
    }
#endif

    size_t const numIndexableFields = GetNumberOfIndexableFields();
    for (size_t i = 0; i < numIndexableFields; ++i) {
        if (indexableFields[i] != nullptr) {
            indexableFields[i] = walk(indexableFields[i]);
        }
    }
}

#ifdef UNSAFE_FRAME_OPTIMIZATION
VMFrame* VMMethod::GetCachedFrame() const {
    return cachedFrame;
}

void VMMethod::SetCachedFrame(VMFrame* frame) {
    cachedFrame = frame;
    if (frame != nullptr) {
        frame->SetContext(nullptr);
        frame->SetBytecodeIndex(0);
        frame->ResetStackPointer();
        write_barrier(this, cachedFrame);
    }
}
#endif

VMFrame* VMMethod::Invoke(VMFrame* frame) {
    // since an invokable is able to change/use the frame, we have to write
    // cached values before, and read cached values after calling
    frame->SetBytecodeIndex(Interpreter::GetBytecodeIndex());

    VMFrame* frm = Interpreter::PushNewFrame(this);
    frm->CopyArgumentsFrom(frame);
    return frm;
}

VMFrame* VMMethod::Invoke1(VMFrame* frame) {
    // since an invokable is able to change/use the frame, we have to write
    // cached values before, and read cached values after calling
    frame->SetBytecodeIndex(Interpreter::GetBytecodeIndex());

    VMFrame* frm = Interpreter::PushNewFrame(this);
    frm->SetArgument(0, frame->Top());
    return frm;
}

void VMMethod::SetHolder(VMClass* hld) {
    VMInvokable::SetHolder(hld);
    SetHolderAll(hld);
}

void VMMethod::SetHolderAll(VMClass* hld) const {
    size_t const numIndexableFields = GetNumberOfIndexableFields();
    for (size_t i = 0; i < numIndexableFields; ++i) {
        vm_oop_t o = GetIndexableField(i);
        if (!IS_TAGGED(o)) {
            auto* vmi = dynamic_cast<VMInvokable*>(AS_OBJ(o));
            if (vmi != nullptr) {
                vmi->SetHolder(hld);
            }
        }
    }
}

void VMMethod::Dump(const char* indent, bool printObjects) {
    Disassembler::DumpMethod(this, indent, printObjects);
}

std::string VMMethod::AsDebugString() const {
    VMClass* holder = GetHolder();
    std::string holder_str;
    if (holder == load_ptr(nilObject)) {
        holder_str = "nil";
    } else {
        holder_str = holder->GetName()->GetStdString();
    }
    return "Method(" + holder_str + ">>#" + GetSignature()->GetStdString() +
           ")";
}

void VMMethod::InlineInto(MethodGenerationContext& mgenc, const Parser& parser,
                          bool mergeScope) {
    if (mergeScope) {
        mgenc.MergeIntoScope(*lexicalScope);
    }
    inlineInto(mgenc, parser);
}

std::priority_queue<BackJump> VMMethod::createBackJumpHeap() {
    std::priority_queue<BackJump> loops;
    if (inlinedLoops != nullptr) {
        size_t i = 0;
        while (inlinedLoops[i].IsValid()) {
            loops.emplace(inlinedLoops[i]);
            i += 1;
        }
    }
    return loops;
}

void VMMethod::inlineInto(MethodGenerationContext& mgenc,
                          const Parser& parser) {
    std::priority_queue<Jump>
        jumps;  // priority queue sorted by originalJumpTargetIdx
    std::priority_queue<BackJump> backJumps = createBackJumpHeap();
    std::priority_queue<BackJumpPatch> backJumpsToPatch;

    size_t i = 0;
    const size_t numBytecodes = GetNumberOfBytecodes();
    while (i < numBytecodes) {
        prepareBackJumpToCurrentAddress(backJumps, backJumpsToPatch, i, mgenc);
        patchJumpToCurrentAddress(i, jumps, mgenc);

        const uint8_t bytecode = bytecodes[i];
        const uint8_t bcLength = Bytecode::GetBytecodeLength(bytecode);

        switch (bytecode) {
            case BC_DUP:
            case BC_DUP_SECOND: {
                Emit1(mgenc, bytecode, 1);
                break;
            }
            case BC_PUSH_FIELD:
            case BC_PUSH_FIELD_0:
            case BC_PUSH_FIELD_1:
            case BC_POP_FIELD:
            case BC_POP_FIELD_0:
            case BC_POP_FIELD_1: {
                uint8_t idx = 0;
                if (bytecode == BC_PUSH_FIELD || bytecode == BC_POP_FIELD) {
                    idx = bytecodes[i + 1];
                } else {
                    switch (bytecode) {
                        case BC_PUSH_FIELD_0: {
                            idx = 0;
                            break;
                        }
                        case BC_PUSH_FIELD_1: {
                            idx = 1;
                            break;
                        }
                        case BC_POP_FIELD_0: {
                            idx = 0;
                            break;
                        }
                        case BC_POP_FIELD_1: {
                            idx = 1;
                            break;
                        }
                        default: {
                            ErrorExit("Unexpected bytecode in inlineInto");
                        }
                    }
                }

                if (bytecode == BC_PUSH_FIELD || bytecode == BC_PUSH_FIELD_0 ||
                    bytecode == BC_PUSH_FIELD_1) {
                    EmitPushFieldWithIndex(mgenc, idx);
                } else {
                    EmitPopFieldWithIndex(mgenc, idx);
                }
                break;
            }

            case BC_POP_ARGUMENT: {
                const uint8_t idx = bytecodes[i + 1];
                const uint8_t ctxLevel = bytecodes[i + 2];
                assert(ctxLevel > 0);

                assert(bytecode == BC_POP_ARGUMENT);
                EmitPOPARGUMENT(mgenc, parser, idx, ctxLevel - 1);
                break;
            }
            case BC_PUSH_ARGUMENT: {
                const uint8_t idx = bytecodes[i + 1];
                const uint8_t ctxLevel = bytecodes[i + 2];
                assert(ctxLevel > 0);

                EmitPUSHARGUMENT(mgenc, parser, idx, ctxLevel - 1);
                break;
            }
            case BC_INC_FIELD:
            case BC_INC_FIELD_PUSH: {
                const uint8_t idx = bytecodes[i + 1];
                Emit2(mgenc, bytecode, idx, bytecode == BC_INC_FIELD ? 0 : 1);
                break;
            }
            case BC_PUSH_LOCAL:
            case BC_POP_LOCAL: {
                uint8_t idx = bytecodes[i + 1];
                uint8_t ctxLevel = bytecodes[i + 2];

                if (ctxLevel == 0) {
                    // these have been inlined into the outer context already
                    // so, we need to look up the right one
                    const Variable* const var = lexicalScope->GetLocal(idx, 0);
                    idx = mgenc.GetInlinedLocalIdx(var);
                } else {
                    ctxLevel -= 1;
                }

                if (bytecode == BC_PUSH_LOCAL) {
                    EmitPUSHLOCAL(mgenc, parser, idx, ctxLevel);
                } else {
                    EmitPOPLOCAL(mgenc, parser, idx, ctxLevel);
                }
                break;
            }

            case BC_PUSH_LOCAL_0:
            case BC_PUSH_LOCAL_1:
            case BC_PUSH_LOCAL_2: {
                uint8_t const idx = bytecode - BC_PUSH_LOCAL_0;
                const auto* oldVar = lexicalScope->GetLocal(idx, 0);
                uint8_t const newIdx = mgenc.GetInlinedLocalIdx(oldVar);
                EmitPUSHLOCAL(mgenc, parser, newIdx, 0);
                break;
            }

            case BC_POP_LOCAL_0:
            case BC_POP_LOCAL_1:
            case BC_POP_LOCAL_2: {
                uint8_t const idx = bytecode - BC_POP_LOCAL_0;
                const auto* oldVar = lexicalScope->GetLocal(idx, 0);
                uint8_t const newIdx = mgenc.GetInlinedLocalIdx(oldVar);
                EmitPOPLOCAL(mgenc, parser, newIdx, 0);
                break;
            }

            case BC_PUSH_ARG_1:
            case BC_PUSH_ARG_2: {
                // this can now happen with inlining #to:do:
                size_t const argIdx = bytecode == BC_PUSH_ARG_1 ? 1 : 2;

                const Variable* arg = lexicalScope->GetArgument(argIdx, 0);
                size_t const inlinedLocalIndex = mgenc.GetInlinedLocalIdx(arg);
                EmitPUSHLOCAL(mgenc, parser, inlinedLocalIndex, 0);
                break;
            }

            case BC_PUSH_BLOCK: {
                auto* blockMethod = (VMInvokable*)GetConstant(i);
                blockMethod->AdaptAfterOuterInlined(1, mgenc);
                EmitPUSHBLOCK(mgenc, parser, blockMethod);
                break;
            }
            case BC_PUSH_CONSTANT: {
                vm_oop_t literal = GetConstant(i);
                EmitPUSHCONSTANT(mgenc, parser, literal);
                break;
            }
            case BC_PUSH_CONSTANT_0:
            case BC_PUSH_CONSTANT_1:
            case BC_PUSH_CONSTANT_2: {
                const uint8_t literalIdx = bytecode - BC_PUSH_CONSTANT_0;
                vm_oop_t literal = GetIndexableField(literalIdx);
                EmitPUSHCONSTANT(mgenc, parser, literal);
                break;
            }
            case BC_PUSH_0:
            case BC_PUSH_1:
            case BC_PUSH_NIL: {
                Emit1(mgenc, bytecode, 1);
                break;
            }
            case BC_POP: {
                // TODO(smarr): PySOM simply does Emit1
                //   not sure whether EmitPOP might cause issues if we try to do
                //   optimizations here again
                EmitPOP(mgenc);
                break;
            }
            case BC_INC:
            case BC_DEC: {
                Emit1(mgenc, bytecode, 0);
                break;
            }
            case BC_PUSH_GLOBAL: {
                auto* const sym = (VMSymbol*)GetConstant(i);
                EmitPUSHGLOBAL(mgenc, parser, sym);
                break;
            }
            case BC_SEND:
            case BC_SEND_1:
            case BC_SEND_2:
            case BC_SEND_3:
            case BC_SEND_N: {
                auto* const sym = (VMSymbol*)GetConstant(i);
                EmitSEND(mgenc, parser, sym);
                break;
            }
            case BC_SUPER_SEND: {
                auto* const sym = (VMSymbol*)GetConstant(i);
                EmitSUPERSEND(mgenc, parser, sym);
                break;
            }
            case BC_RETURN_LOCAL: {
                // NO OP, doesn't need to be translated
                break;
            }
            case BC_RETURN_NON_LOCAL: {
                if (mgenc.IsBlockMethod()) {
                    assert(mgenc.GetMaxContextLevel() > 0);
                    EmitRETURNNONLOCAL(mgenc);
                } else {
                    EmitRETURNLOCAL(mgenc, parser);
                }
                break;
            }
            case BC_RETURN_FIELD_0:
            case BC_RETURN_FIELD_1:
            case BC_RETURN_FIELD_2: {
                uint8_t const index = bytecode - BC_RETURN_FIELD_0;
                EmitPushFieldWithIndex(mgenc, index);
                break;
            }

            case BC_JUMP:
            case BC_JUMP_ON_TRUE_TOP_NIL:
            case BC_JUMP_ON_FALSE_TOP_NIL:
            case BC_JUMP_ON_NOT_NIL_TOP_TOP:
            case BC_JUMP_ON_NIL_TOP_TOP:
            case BC_JUMP2:
            case BC_JUMP2_ON_TRUE_TOP_NIL:
            case BC_JUMP2_ON_FALSE_TOP_NIL:
            case BC_JUMP2_ON_NOT_NIL_TOP_TOP:
            case BC_JUMP2_ON_NIL_TOP_TOP:
            case BC_JUMP_IF_GREATER:
            case BC_JUMP2_IF_GREATER: {
                // emit the jump, but instead of the offset, emit a dummy
                const size_t idx = Emit3WithDummy(mgenc, bytecode, 0);
                const size_t offset =
                    ComputeOffset(bytecodes[i + 1], bytecodes[i + 2]);

                jumps.emplace(i + offset, bytecode, idx);
                break;
            }
            case BC_JUMP_ON_TRUE_POP:
            case BC_JUMP_ON_FALSE_POP:
            case BC_JUMP2_ON_TRUE_POP:
            case BC_JUMP2_ON_FALSE_POP:
            case BC_JUMP_ON_NOT_NIL_POP:
            case BC_JUMP_ON_NIL_POP:
            case BC_JUMP2_ON_NOT_NIL_POP:
            case BC_JUMP2_ON_NIL_POP: {
                // emit the jump, but instead of the offset, emit a dummy
                const size_t idx = Emit3WithDummy(mgenc, bytecode, -1);
                const size_t offset =
                    ComputeOffset(bytecodes[i + 1], bytecodes[i + 2]);
                jumps.emplace(i + offset, bytecode, idx);
                break;
            }
            case BC_JUMP_BACKWARD:
            case BC_JUMP2_BACKWARD: {
                const size_t loopBeginIdx = backJumpsToPatch.top().loopBeginIdx;
                assert(backJumpsToPatch.top().backwardsJumpIdx == i &&
                       "the jump should match with the jump instructions");

                backJumpsToPatch.pop();
                mgenc.EmitBackwardsJumpOffsetToTarget(loopBeginIdx);
                break;
            }

            case BC_HALT:
            case BC_PUSH_SELF:
            case BC_RETURN_SELF: {
                char msg[120];
                (void)snprintf(
                    msg, 120,
                    "inlineInto: Found %s bytecode, but it's not expected in a "
                    "block method",
                    Bytecode::GetBytecodeName(bytecode));
                ErrorExit(msg);
                break;
            }
            default: {
                char msg[120];
                (void)snprintf(
                    msg, 120,
                    "inlineInto: Found %s bytecode, but inlining of it is "
                    "not yet "
                    "supported.",
                    Bytecode::GetBytecodeName(bytecode));
                ErrorExit(msg);
                break;
            }
        }

        i += bcLength;
    }

    assert(jumps.empty());
}

void VMMethod::patchJumpToCurrentAddress(size_t i,
                                         std::priority_queue<Jump>& jumps,
                                         MethodGenerationContext& mgenc) {
    while (!jumps.empty() && jumps.top().originalJumpTargetIdx <= i) {
        Jump const jump = jumps.top();
        jumps.pop();

        assert(
            jump.originalJumpTargetIdx == i &&
            "we use the less or equal, but actually expect it to be strictly "
            "equal");
        mgenc.PatchJumpOffsetToPointToNextInstruction(jump.idx);
    }
}

void VMMethod::prepareBackJumpToCurrentAddress(
    std::priority_queue<BackJump>& backJumps,
    std::priority_queue<BackJumpPatch>& backJumpsToPatch, size_t i,
    MethodGenerationContext& mgenc) {
    while (!backJumps.empty() && backJumps.top().loopBeginIdx <= i) {
        BackJump const jump = backJumps.top();
        backJumps.pop();

        assert(
            jump.loopBeginIdx == i &&
            "we use the less or equal, but actually expect it to be strictly "
            "equal");
        backJumpsToPatch.emplace(jump.backwardJumpIdx,
                                 mgenc.OffsetOfNextInstruction());
    }
}

void VMMethod::AdaptAfterOuterInlined(
    uint8_t removedCtxLevel, MethodGenerationContext& mgencWithInlined) {
    size_t i = 0;
    size_t const numBytecodes = GetNumberOfBytecodes();

    while (i < numBytecodes) {
        uint8_t const bytecode = bytecodes[i];
        size_t const bcLength = Bytecode::GetBytecodeLength(bytecode);

        switch (bytecode) {
            case BC_DUP:
            case BC_DUP_SECOND:
            case BC_PUSH_CONSTANT:
            case BC_PUSH_CONSTANT_0:
            case BC_PUSH_CONSTANT_1:
            case BC_PUSH_CONSTANT_2:
            case BC_PUSH_0:
            case BC_PUSH_1:
            case BC_PUSH_NIL:
            case BC_PUSH_GLOBAL:  // BC_PUSH_GLOBAL doesn't encode context
            case BC_PUSH_FIELD:
            case BC_POP_FIELD:
            case BC_POP:
            case BC_SEND:
            case BC_SEND_1:
            case BC_SEND_2:
            case BC_SEND_3:
            case BC_SUPER_SEND:
            case BC_RETURN_LOCAL:
            case BC_RETURN_NON_LOCAL:
            case BC_INC:
            case BC_DEC:
            case BC_JUMP:
            case BC_INC_FIELD_PUSH:  // just field index, implicit/dynamic self
                                     // lookup in SOM++
            case BC_INC_FIELD:       // just field index, implicit/dynamic self
                                     // lookup in SOM++
            case BC_JUMP_ON_TRUE_TOP_NIL:
            case BC_JUMP_ON_TRUE_POP:
            case BC_JUMP_ON_FALSE_TOP_NIL:
            case BC_JUMP_ON_FALSE_POP:
            case BC_JUMP_ON_NOT_NIL_TOP_TOP:
            case BC_JUMP_ON_NIL_TOP_TOP:
            case BC_JUMP_ON_NOT_NIL_POP:
            case BC_JUMP_ON_NIL_POP:
            case BC_JUMP_BACKWARD:
            case BC_JUMP2:
            case BC_JUMP2_ON_TRUE_TOP_NIL:
            case BC_JUMP2_ON_TRUE_POP:
            case BC_JUMP2_ON_FALSE_TOP_NIL:
            case BC_JUMP2_ON_FALSE_POP:
            case BC_JUMP2_ON_NOT_NIL_TOP_TOP:
            case BC_JUMP2_ON_NIL_TOP_TOP:
            case BC_JUMP2_ON_NOT_NIL_POP:
            case BC_JUMP2_ON_NIL_POP:
            case BC_JUMP2_BACKWARD: {
                // these bytecodes do not use context and don't need to be
                // adapted
                break;
            }

            case BC_PUSH_ARGUMENT:
            case BC_POP_ARGUMENT: {
                uint8_t const idx = bytecodes[i + 1];
                uint8_t const ctxLevel = bytecodes[i + 2];

                uint8_t const newContextLevel =
                    ctxLevel > removedCtxLevel ? ctxLevel - 1 : ctxLevel;

                const Variable* oldArg =
                    lexicalScope->GetArgument(idx, ctxLevel);
                const Variable* newVar =
                    mgencWithInlined.GetInlinedVariable(oldArg);

                if (newVar->IsArgument()) {
                    if (ctxLevel > removedCtxLevel) {
                        bytecodes[i + 2] = newContextLevel;
                    }
                    assert(newVar->GetIndex() == idx);
                } else {
                    bytecodes[i] = bytecode == BC_PUSH_ARGUMENT ? BC_PUSH_LOCAL
                                                                : BC_POP_LOCAL;
                    bytecodes[i + 1] = newVar->GetIndex();
                    if (ctxLevel > removedCtxLevel) {
                        bytecodes[i + 2] = newContextLevel;
                    }
                }
                break;
            }

            case BC_PUSH_BLOCK: {
                auto* blockMethod = static_cast<VMMethod*>(GetConstant(i));
                blockMethod->AdaptAfterOuterInlined(removedCtxLevel + 1,
                                                    mgencWithInlined);
                break;
            }

            case BC_PUSH_LOCAL:
            case BC_POP_LOCAL: {
                uint8_t const ctxLevel = bytecodes[i + 2];
                if (ctxLevel == removedCtxLevel) {
                    uint8_t const idx = bytecodes[i + 1];

                    // locals have been inlined into the outer context already
                    // so, we need to look up the right one and fix up the index
                    // at this point, the lexical scope has not been changed
                    // so, we should still be able to find the right one
                    const auto* oldVar = lexicalScope->GetLocal(idx, ctxLevel);
                    uint8_t const newIdx =
                        mgencWithInlined.GetInlinedLocalIdx(oldVar);
                    bytecodes[i + 1] = newIdx;
                } else if (ctxLevel > removedCtxLevel) {
                    bytecodes[i + 2] = ctxLevel - 1;
                }
                break;
            }

            case BC_PUSH_SELF:
            case BC_PUSH_ARG_1:
            case BC_PUSH_ARG_2:
            case BC_PUSH_LOCAL_0:
            case BC_PUSH_LOCAL_1:
            case BC_PUSH_LOCAL_2:
            case BC_PUSH_FIELD_0:
            case BC_PUSH_FIELD_1:
            case BC_POP_LOCAL_0:
            case BC_POP_LOCAL_1:
            case BC_POP_LOCAL_2:
            case BC_POP_FIELD_0:
            case BC_POP_FIELD_1: {
                break;
            }

            case BC_HALT:
            case BC_RETURN_SELF:
            case BC_RETURN_FIELD_0:
            case BC_RETURN_FIELD_1:
            case BC_RETURN_FIELD_2: {
                char msg[120];
                (void)snprintf(
                    msg, 120,
                    "AdaptAfterOuterInlined: Found %s bytecode, but it's not "
                    "expected in a block method",
                    Bytecode::GetBytecodeName(bytecode));
                ErrorExit(msg);
            }

            default: {
                char msg[120];
                (void)snprintf(
                    msg, 120,
                    "Found %s bytecode, but AdaptAfterOuterInlined does "
                    "not yet "
                    "support it.",
                    Bytecode::GetBytecodeName(bytecode));
                ErrorExit(msg);
            }
        }

        i += bcLength;
    }

    if (removedCtxLevel == 1) {
        lexicalScope->DropInlinedScope();
    }
}

bool operator<(const Jump& a, const Jump& b) {
    return a.originalJumpTargetIdx > b.originalJumpTargetIdx;
}

bool operator<(const BackJump& a, const BackJump& b) {
    return a.loopBeginIdx > b.loopBeginIdx;
}

bool operator<(const BackJumpPatch& a, const BackJumpPatch& b) {
    return a.backwardsJumpIdx > b.backwardsJumpIdx;
}

void VMMethod::MergeScopeInto(MethodGenerationContext& mgenc) {
    assert(lexicalScope != nullptr);
    mgenc.MergeIntoScope(*lexicalScope);
}

size_t VMMethod::GetBytecodeHash() const {
    return murmur3_32(bytecodes, bcLength, 0x00000000);
}
