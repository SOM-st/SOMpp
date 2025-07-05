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

#include "MethodGenerationContext.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

#include "../interpreter/bytecodes.h"
#include "../misc/VectorUtil.h"
#include "../vm/Globals.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSymbol.h"
#include "../vmobjects/VMTrivialMethod.h"
#include "BytecodeGenerator.h"
#include "ClassGenerationContext.h"
#include "LexicalScope.h"
#include "Parser.h"
#include "SourceCoordinate.h"
#include "Variable.h"

MethodGenerationContext::MethodGenerationContext(ClassGenerationContext& holder,
                                                 MethodGenerationContext* outer)
    : holderGenc(holder), outerGenc(outer),
      maxContextLevel(outer == nullptr ? 0 : outer->GetMaxContextLevel() + 1),
      blockMethod(outer != nullptr),
      last4Bytecodes({BC_INVALID, BC_INVALID, BC_INVALID, BC_INVALID}) {}

VMInvokable* MethodGenerationContext::Assemble() {
    VMTrivialMethod* trivialMethod = assembleTrivialMethod();
    if (trivialMethod != nullptr) {
        return trivialMethod;
    }

    // create a method instance with the given number of bytecodes and literals
    size_t const numLiterals = literals.size();
    size_t const numLocals = locals.size();
    VMMethod* meth =
        Universe::NewMethod(signature, bytecode.size(), numLiterals, numLocals,
                            maxStackDepth, lexicalScope, inlinedLoops);

    // copy literals into the method
    for (size_t i = 0; i < numLiterals; i++) {
        vm_oop_t l = literals[i];
        meth->SetIndexableField(i, l);
    }

    // copy bytecodes into method
    size_t const bc_size = bytecode.size();
    for (size_t i = 0; i < bc_size; i++) {
        meth->SetBytecode(i, bytecode[i]);
    }

    // return the method - the holder field is to be set later on!
    return meth;
}

VMTrivialMethod* MethodGenerationContext::assembleTrivialMethod() {
    if (LastBytecodeIs(0, BC_RETURN_LOCAL)) {
        uint8_t pushCandidate = lastBytecodeIsOneOf(1, &IsPushConstBytecode);
        if (pushCandidate != BC_INVALID) {
            return assembleLiteralReturn(pushCandidate);
        }

        if (LastBytecodeIs(1, BC_PUSH_GLOBAL)) {
            return assembleGlobalReturn();
        }

        pushCandidate = lastBytecodeIsOneOf(1, &IsPushFieldBytecode);
        if (pushCandidate != BC_INVALID) {
            return assembleFieldGetter(pushCandidate);
        }
    }

    // because we check for returning self here, we don't consider block methods
    if (LastBytecodeIs(0, BC_RETURN_SELF)) {
        assert(!IsBlockMethod());
        return assembleFieldSetter();
    }

    uint8_t const returnCandidate =
        lastBytecodeIsOneOf(0, &IsReturnFieldBytecode);
    if (returnCandidate != BC_INVALID) {
        return assembleFieldGetterFromReturn(returnCandidate);
    }

    return nullptr;
}

VMTrivialMethod* MethodGenerationContext::assembleLiteralReturn(
    uint8_t pushCandidate) {
    if (bytecode.size() != (Bytecode::GetBytecodeLength(pushCandidate) +
                            Bytecode::GetBytecodeLength(BC_RETURN_LOCAL))) {
        return nullptr;
    }

    switch (pushCandidate) {
        case BC_PUSH_0: {
            return MakeLiteralReturn(signature, arguments, NEW_INT(0));
        }
        case BC_PUSH_1: {
            return MakeLiteralReturn(signature, arguments, NEW_INT(1));
        }
        case BC_PUSH_NIL: {
            return MakeLiteralReturn(signature, arguments, load_ptr(nilObject));
        }
        case BC_PUSH_CONSTANT_0:
        case BC_PUSH_CONSTANT_1:
        case BC_PUSH_CONSTANT_2:
        case BC_PUSH_CONSTANT: {
            if (literals.size() == 1) {
                return MakeLiteralReturn(signature, arguments, literals.at(0));
            }
        }

        default: {
            ErrorExit(
                "Unexpected situation when trying to create trivial method "
                "that returns a literal");
        }
    }
}

VMTrivialMethod* MethodGenerationContext::assembleGlobalReturn() {
    if (bytecode.size() != (Bytecode::GetBytecodeLength(BC_PUSH_GLOBAL) +
                            Bytecode::GetBytecodeLength(BC_RETURN_LOCAL))) {
        return nullptr;
    }

    if (literals.size() != 1) {
        ErrorExit(
            "Unexpected situation when trying to create trivial method that "
            "reads a global. New Bytecode?");
    }

    auto* globalName = (VMSymbol*)literals.at(0);
    return MakeGlobalReturn(signature, arguments, globalName);
}

VMTrivialMethod* MethodGenerationContext::assembleFieldGetter(
    uint8_t pushCandidate) {
    if (bytecode.size() != (Bytecode::GetBytecodeLength(pushCandidate) +
                            Bytecode::GetBytecodeLength(BC_RETURN_LOCAL))) {
        return nullptr;
    }

    size_t fieldIndex = 0;
    if (pushCandidate == BC_PUSH_FIELD_0) {
        fieldIndex = 0;
    } else if (pushCandidate == BC_PUSH_FIELD_1) {
        fieldIndex = 1;
    } else {
        assert(pushCandidate == BC_PUSH_FIELD);
        // -2: -1 to skip over a 1-byte BC_RETURN_LOCAL,
        // and of course -1 for length vs offset
        fieldIndex = bytecode.at(bytecode.size() - 2);
        assert(fieldIndex > 1 &&
               "BC_PUSH_FIELD with index 0 or 1 is not optimal");
    }

    return MakeGetter(signature, arguments, fieldIndex);
}

VMTrivialMethod* MethodGenerationContext::assembleFieldGetterFromReturn(
    uint8_t returnCandidate) {
    if (bytecode.size() != Bytecode::GetBytecodeLength(returnCandidate)) {
        return nullptr;
    }

    size_t index = 0;
    switch (returnCandidate) {
        case BC_RETURN_FIELD_0:
            index = 0;
            break;

        case BC_RETURN_FIELD_1:
            index = 1;
            break;

        case BC_RETURN_FIELD_2:
            index = 2;
            break;

        default:
            ErrorExit("Unsupported bytecode in assembleFieldGetterFromReturn");
            break;
    }

    return MakeGetter(signature, arguments, index);
}

VMTrivialMethod* MethodGenerationContext::assembleFieldSetter() {
    uint8_t const popCandidate = lastBytecodeIsOneOf(1, IsPopFieldBytecode);
    if (popCandidate == BC_INVALID) {
        return nullptr;
    }

    uint8_t const pushCandidate = lastBytecodeIsOneOf(2, IsPushArgBytecode);
    if (pushCandidate == BC_INVALID) {
        return nullptr;
    }

    size_t const lenReturnSelf = Bytecode::GetBytecodeLength(BC_RETURN_SELF);
    size_t const lenInclPop =
        lenReturnSelf + Bytecode::GetBytecodeLength(popCandidate);
    if (bytecode.size() !=
        (lenInclPop + Bytecode::GetBytecodeLength(pushCandidate))) {
        return nullptr;
    }

    size_t argIndex = 0;

    switch (pushCandidate) {
        case BC_PUSH_SELF:
            argIndex = 0;
            break;
        case BC_PUSH_ARG_1:
            argIndex = 1;
            break;
        case BC_PUSH_ARG_2:
            argIndex = 2;
            break;
        case BC_PUSH_ARGUMENT: {
            argIndex = bytecode.at(bytecode.size() - (lenInclPop + 2));
            break;
        }
        default: {
            ErrorExit("Unexpected bytecode");
        }
    }

    size_t fieldIndex = 0;

    switch (popCandidate) {
        case BC_POP_FIELD_0:
            fieldIndex = 0;
            break;
        case BC_POP_FIELD_1:
            fieldIndex = 1;
            break;
        case BC_POP_FIELD: {
            fieldIndex = bytecode.at(bytecode.size() - (lenReturnSelf + 1));
            break;
        }
        default: {
            ErrorExit("Unexpected bytecode");
        }
    }

    return MakeSetter(signature, arguments, fieldIndex, argIndex);
}

VMInvokable* MethodGenerationContext::AssemblePrimitive(bool classSide) {
    return VMPrimitive::GetEmptyPrimitive(signature, classSide);
}

MethodGenerationContext::~MethodGenerationContext() = default;

int8_t MethodGenerationContext::FindLiteralIndex(vm_oop_t lit) {
    return (int8_t)IndexOf(literals, lit);
}

int64_t MethodGenerationContext::GetFieldIndex(VMSymbol* field) {
    int64_t const idx = holderGenc.GetFieldIndex(field);
    assert(idx >= 0);
    return idx;
}

static bool Contains(std::vector<Variable>& vec, std::string& name) {
    for (Variable const& v : vec) {
        if (v.IsNamed(name)) {
            return true;
        }
    }

    return false;
}

static int64_t IndexOf(std::vector<Variable>& vec, std::string& name) {
    int64_t i = 0;
    for (Variable const& v : vec) {
        if (v.IsNamed(name)) {
            return i;
        }
        i += 1;
    }

    return -1;
}

bool MethodGenerationContext::FindVar(std::string& var, int64_t* index,
                                      int* context, bool* isArgument) {
    *index = IndexOf(locals, var);
    if (*index == -1) {
        *index = IndexOf(arguments, var);
        if (*index == -1) {
            if (outerGenc == nullptr) {
                return false;
            }

            (*context)++;
            return outerGenc->FindVar(var, index, context, isArgument);
        }
        *isArgument = true;
    }

    return true;
}

bool MethodGenerationContext::HasField(VMSymbol* field) {
    return holderGenc.HasField(field);
}

uint8_t MethodGenerationContext::GetNumberOfArguments() {
    return arguments.size();
}

void MethodGenerationContext::SetSignature(VMSymbol* sig) {
    signature = sig;
}

void MethodGenerationContext::SetPrimitive(bool prim) {
    primitive = prim;
}

void MethodGenerationContext::AddArgument(std::string& arg,
                                          const SourceCoordinate& coord) {
    size_t const index = arguments.size();
    arguments.emplace_back(arg, index, true, coord);
}

void MethodGenerationContext::AddLocal(std::string& local,
                                       const SourceCoordinate& coord) {
    size_t const index = locals.size();
    locals.emplace_back(local, index, false, coord);
}

uint8_t MethodGenerationContext::AddLiteral(vm_oop_t lit,
                                            const Parser& parser) {
    assert(!AS_OBJ(lit)->IsMarkedInvalid());

    size_t const index = literals.size();

    if (index > std::numeric_limits<int8_t>::max()) {
        parser.ParseError(
            "The method has too many literals. You may be able to split up "
            "this method into multiple.");
    }

    literals.push_back(lit);
    return index;
}

uint8_t MethodGenerationContext::AddLiteralIfAbsent(vm_oop_t lit,
                                                    const Parser& parser) {
    int64_t const idx = IndexOf(literals, lit);
    if (idx != -1) {
        assert(idx < 256);
        assert(idx >= 0 && (size_t)idx < literals.size() &&
               "Expect index to be inside the literals vector.");
        return (uint8_t)idx;
    }
    return AddLiteral(lit, parser);
}

void MethodGenerationContext::UpdateLiteral(vm_oop_t oldValue, uint8_t index,
                                            vm_oop_t newValue) {
    assert(literals.at(index) == oldValue);
    literals[index] = newValue;
}

bool MethodGenerationContext::AddArgumentIfAbsent(
    std::string& arg, const SourceCoordinate& coord) {
    if (Contains(locals, arg)) {
        return false;
    }
    AddArgument(arg, coord);
    return true;
}

bool MethodGenerationContext::AddLocalIfAbsent(std::string& local,
                                               const SourceCoordinate& coord) {
    if (Contains(locals, local)) {
        return false;
    }

    AddLocal(local, coord);
    return true;
}

void MethodGenerationContext::MarkFinished() {
    this->finished = true;
}

bool MethodGenerationContext::HasBytecodes() {
    return !bytecode.empty();
}

void MethodGenerationContext::AddBytecode(uint8_t bc, int64_t stackEffect) {
    currentStackDepth += stackEffect;
    maxStackDepth = max(maxStackDepth, currentStackDepth);

    bytecode.push_back(bc);

    last4Bytecodes[0] = last4Bytecodes[1];
    last4Bytecodes[1] = last4Bytecodes[2];
    last4Bytecodes[2] = last4Bytecodes[3];
    last4Bytecodes[3] = bc;
}

void MethodGenerationContext::AddBytecodeArgument(uint8_t arg) {
    bytecode.push_back(arg);
}

size_t MethodGenerationContext::AddBytecodeArgumentAndGetIndex(size_t bc) {
    size_t const index = bytecode.size();
    AddBytecodeArgument(bc);
    return index;
}

uint8_t MethodGenerationContext::lastBytecodeAt(size_t indexFromEnd) {
    assert(indexFromEnd >= 0 && indexFromEnd < NUM_LAST_BYTECODES);
    return last4Bytecodes[3 - indexFromEnd];
}

bool MethodGenerationContext::LastBytecodeIs(size_t indexFromEnd,
                                             uint8_t bytecode) {
    assert(indexFromEnd >= 0 && indexFromEnd < NUM_LAST_BYTECODES);
    uint8_t const actual = last4Bytecodes[3 - indexFromEnd];
    return actual == bytecode;
}

uint8_t MethodGenerationContext::lastBytecodeIsOneOf(
    size_t indexFromEnd, uint8_t (*predicate)(uint8_t)) {
    assert(indexFromEnd >= 0 && indexFromEnd < 4);
    uint8_t const actual = last4Bytecodes[3 - indexFromEnd];
    return predicate(actual);
}

void MethodGenerationContext::removeLastBytecodes(size_t numBytecodes) {
    assert(numBytecodes > 0 && numBytecodes <= 4);
    ptrdiff_t bytesToRemove = 0;

    for (size_t idxFromEnd = 0; idxFromEnd < numBytecodes; idxFromEnd += 1) {
        bytesToRemove +=
            Bytecode::GetBytecodeLength(last4Bytecodes[3 - idxFromEnd]);
    }

    assert(bytesToRemove > 0);
    bytecode.erase(bytecode.end() - bytesToRemove, bytecode.end());
}

bool MethodGenerationContext::hasOneLiteralBlockArgument() {
    return LastBytecodeIs(0, BC_PUSH_BLOCK);
}

bool MethodGenerationContext::hasTwoLiteralBlockArguments() {
    if (!LastBytecodeIs(0, BC_PUSH_BLOCK)) {
        return false;
    }

    return LastBytecodeIs(1, BC_PUSH_BLOCK);
}

/**
 * This works only, because we have a simple forward-pass parser,
 * and inlining, where this is used, happens right after the block was added.
 * This also means, we need to remove blocks in reverse order.
 */
VMInvokable* MethodGenerationContext::getLastBlockMethodAndFreeLiteral(
    uint8_t blockLiteralIdx) {
    assert(blockLiteralIdx == literals.size() - 1);
    auto* block = (VMInvokable*)literals.back();
    literals.pop_back();
    return block;
}

VMInvokable* MethodGenerationContext::extractBlockMethodAndRemoveBytecode() {
    uint8_t const blockLitIdx = bytecode.at(bytecode.size() - 1);

    vm_oop_t toBeInlined = getLastBlockMethodAndFreeLiteral(blockLitIdx);

    removeLastBytecodes(1);

    return (VMInvokable*)toBeInlined;
}

std::tuple<VMInvokable*, VMInvokable*>
MethodGenerationContext::extractBlockMethodsAndRemoveBytecodes() {
    uint8_t const block1LitIdx = bytecode.at(bytecode.size() - 3);
    uint8_t const block2LitIdx = bytecode.at(bytecode.size() - 1);

    // grab the blocks' methods for inlining
    VMInvokable* toBeInlined2 = getLastBlockMethodAndFreeLiteral(block2LitIdx);
    VMInvokable* toBeInlined1 = getLastBlockMethodAndFreeLiteral(block1LitIdx);

    removeLastBytecodes(2);

    return {toBeInlined1, toBeInlined2};
}

bool MethodGenerationContext::InlineThenBranch(const Parser& parser,
                                               JumpCondition condition) {
    // HACK: We do assume that the receiver on the stack is a boolean,
    // HACK: similar to the IfTrueIfFalseNode.
    // HACK: We don't support anything but booleans at the moment.
    assert(Bytecode::GetBytecodeLength(BC_PUSH_BLOCK) == 2);
    if (!hasOneLiteralBlockArgument()) {
        return false;
    }

    VMInvokable* toBeInlined = extractBlockMethodAndRemoveBytecode();

    size_t const jumpOffsetIdxToSkipBody =
        EmitJumpOnWithDummyOffset(*this, condition, false);

    isCurrentlyInliningABlock = true;

    toBeInlined->InlineInto(*this, parser);
    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToSkipBody);

    // with the jumping, it's best to prevent any subsequent optimizations here
    // otherwise we may not have the correct jump target
    resetLastBytecodeBuffer();

    return true;
}

bool MethodGenerationContext::InlineThenElseBranches(const Parser& parser,
                                                     JumpCondition condition) {
    // HACK: We do assume that the receiver on the stack is a boolean,
    // HACK: similar to the IfTrueIfFalseNode.
    // HACK: We don't support anything but booleans at the moment.

    if (!hasTwoLiteralBlockArguments()) {
        return false;
    }

    assert(Bytecode::GetBytecodeLength(BC_PUSH_BLOCK) == 2);

    std::tuple<VMInvokable*, VMInvokable*> methods =
        extractBlockMethodsAndRemoveBytecodes();
    VMInvokable* condMethod = std::get<0>(methods);
    VMInvokable* bodyMethod = std::get<1>(methods);

    size_t const jumpOffsetIdxToSkipTrueBranch =
        EmitJumpOnWithDummyOffset(*this, condition, true);

    isCurrentlyInliningABlock = true;
    condMethod->InlineInto(*this, parser);

    size_t const jumpOffsetIdxToSkipFalseBranch = EmitJumpWithDumyOffset(*this);

    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToSkipTrueBranch);

    // prevent optimizations between blocks to avoid issues with jump targets
    resetLastBytecodeBuffer();

    bodyMethod->InlineInto(*this, parser);

    isCurrentlyInliningABlock = false;

    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToSkipFalseBranch);

    // prevent optimizations messing with the final jump target
    resetLastBytecodeBuffer();

    return true;
}

bool MethodGenerationContext::InlineWhile(const Parser& parser,
                                          bool isWhileTrue) {
    if (!hasTwoLiteralBlockArguments()) {
        return false;
    }

    assert(Bytecode::GetBytecodeLength(BC_PUSH_BLOCK) == 2);

    std::tuple<VMInvokable*, VMInvokable*> methods =
        extractBlockMethodsAndRemoveBytecodes();
    VMInvokable* condMethod = std::get<0>(methods);
    VMInvokable* bodyMethod = std::get<1>(methods);

    size_t const loopBeginIdx = OffsetOfNextInstruction();

    isCurrentlyInliningABlock = true;
    condMethod->InlineInto(*this, parser);

    size_t const jumpOffsetIdxToSkipLoopBody = EmitJumpOnWithDummyOffset(
        *this, isWhileTrue ? ON_FALSE : ON_TRUE, true);

    bodyMethod->InlineInto(*this, parser);

    completeJumpsAndEmitReturningNil(parser, loopBeginIdx,
                                     jumpOffsetIdxToSkipLoopBody);

    isCurrentlyInliningABlock = false;

    return true;
}

bool MethodGenerationContext::InlineAndOr(const Parser& parser, bool isOr) {
    // HACK: We do assume that the receiver on the stack is a boolean,
    // HACK: similar to the IfTrueIfFalseNode.
    // HACK: We don't support anything but booleans at the moment.

    assert(Bytecode::GetBytecodeLength(BC_PUSH_BLOCK) == 2);
    if (!hasOneLiteralBlockArgument()) {
        return false;
    }

    VMInvokable* toBeInlined = extractBlockMethodAndRemoveBytecode();

    size_t const jumpOffsetIdxToSkipBranch =
        EmitJumpOnWithDummyOffset(*this, isOr ? ON_TRUE : ON_FALSE, true);

    isCurrentlyInliningABlock = true;
    toBeInlined->InlineInto(*this, parser);
    isCurrentlyInliningABlock = false;

    size_t const jumpOffsetIdxToSkipPushTrue = EmitJumpWithDumyOffset(*this);

    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToSkipBranch);
    EmitPUSHCONSTANT(*this, parser,
                     isOr ? load_ptr(trueObject) : load_ptr(falseObject));

    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToSkipPushTrue);

    resetLastBytecodeBuffer();

    return true;
}

bool MethodGenerationContext::InlineToDo(const Parser& parser) {
    // HACK: We do assume that the receiver on the stack is a integer,
    // HACK: similar to the other inlined messages.
    // HACK: We don't support anything but integer at the moment.
    assert(Bytecode::GetBytecodeLength(BC_PUSH_BLOCK) == 2);
    if (!hasOneLiteralBlockArgument()) {
        return false;
    }

    VMInvokable* toBeInlined = extractBlockMethodAndRemoveBytecode();

    toBeInlined->MergeScopeInto(*this);

    const Variable* blockArg = toBeInlined->GetArgument(1, 0);
    uint8_t const iVarIdx = GetInlinedLocalIdx(blockArg);

    isCurrentlyInliningABlock = true;
    EmitDupSecond(*this);

    size_t const loopBeginIdx = OffsetOfNextInstruction();
    size_t const jumpOffsetIdxToEnd = EmitJumpIfGreaterWithDummyOffset(*this);

    EmitDUP(*this);

    EmitPOPLOCAL(*this, parser, iVarIdx, 0);

    toBeInlined->InlineInto(*this, parser, false);

    EmitPOP(*this);
    EmitINC(*this);

    EmitBackwardsJumpOffsetToTarget(loopBeginIdx);

    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToEnd);

    isCurrentlyInliningABlock = false;

    return true;
}

void MethodGenerationContext::CompleteLexicalScope() {
    lexicalScope = new LexicalScope(
        outerGenc == nullptr ? nullptr : outerGenc->lexicalScope, arguments,
        locals);
}

void MethodGenerationContext::MergeIntoScope(LexicalScope& scopeToBeInlined) {
    if (scopeToBeInlined.GetNumberOfArguments() > 1) {
        InlineAsLocals(scopeToBeInlined.arguments);
    }

    if (scopeToBeInlined.GetNumberOfLocals() > 0) {
        InlineAsLocals(scopeToBeInlined.locals);
    }
}

void MethodGenerationContext::InlineAsLocals(vector<Variable>& vars) {
    for (const Variable& var : vars) {
        Variable freshCopy = var.CopyForInlining(this->locals.size());
        if (freshCopy.IsValid()) {
            assert(!freshCopy.IsArgument());

            // freshCopy can be invalid, because we don't need the $blockSelf
            std::string qualifiedName = var.MakeQualifiedName();
            assert(!Contains(this->locals, qualifiedName));
            lexicalScope->AddInlinedLocal(freshCopy);
            this->locals.push_back(freshCopy);
        }
    }
}

uint8_t MethodGenerationContext::GetInlinedLocalIdx(const Variable* var) const {
    assert(locals.size() < UINT8_MAX);
    uint8_t index = locals.size();

    while (index > 0) {
        index -= 1;
        if (locals[index].IsSame(*var)) {
            return index;
        }
    }

    char msg[200];
    std::string qualifiedName = var->MakeQualifiedName();
    (void)snprintf(
        msg, 200,
        "Unexpected issue trying to find an inlined variable. %s could not "
        "be found.",
        qualifiedName.data());
    ErrorExit(msg);
}

const Variable* MethodGenerationContext::GetInlinedVariable(
    const Variable* oldVar) const {
    for (const Variable& v : locals) {
        if (v.IsSame(*oldVar)) {
            return &v;
        }
    }

    for (const Variable& v : arguments) {
        if (v.IsSame(*oldVar)) {
            return &v;
        }
    }

    if (outerGenc != nullptr) {
        return outerGenc->GetInlinedVariable(oldVar);
    }

    char msg[100];
    const std::string* name = oldVar->GetName();
    (void)snprintf(msg, 100, "Failed to find inlined variable named %s.\n",
                   name->c_str());
    ErrorExit(msg);
}

void MethodGenerationContext::checkJumpOffset(size_t jumpOffset,
                                              uint8_t bytecode) {
    if (jumpOffset < 0 || jumpOffset > 0xFFFF) {
        char msg[100];
        (void)snprintf(
            msg, 100,
            "The jumpOffset for the %s bytecode is out of range: %zu\n",
            Bytecode::GetBytecodeName(bytecode), jumpOffset);
        ErrorExit(msg);
    }
}

void MethodGenerationContext::EmitBackwardsJumpOffsetToTarget(
    size_t loopBeginIdx) {
    size_t const addressOfJump = OffsetOfNextInstruction();

    // we are going to jump backward and want a positive value
    // thus we subtract target_address from address_of_jump
    size_t const jumpOffset = addressOfJump - loopBeginIdx;

    checkJumpOffset(jumpOffset, BC_JUMP_BACKWARD);

    size_t const backwardJumpIdx = OffsetOfNextInstruction();

    EmitJumpBackwardWithOffset(*this, jumpOffset);
    inlinedLoops.emplace_back(loopBeginIdx, backwardJumpIdx);
}

void MethodGenerationContext::completeJumpsAndEmitReturningNil(
    const Parser& parser, size_t loopBeginIdx,
    size_t jumpOffsetIdxToSkipLoopBody) {
    resetLastBytecodeBuffer();
    EmitPOP(*this);

    EmitBackwardsJumpOffsetToTarget(loopBeginIdx);

    PatchJumpOffsetToPointToNextInstruction(jumpOffsetIdxToSkipLoopBody);
    EmitPUSHCONSTANT(*this, parser, load_ptr(nilObject));
    resetLastBytecodeBuffer();
}

void MethodGenerationContext::PatchJumpOffsetToPointToNextInstruction(
    size_t indexOfOffset) {
    size_t const instructionStart = indexOfOffset - 1;
    uint8_t const bytecode = this->bytecode[instructionStart];
    assert(IsJumpBytecode(bytecode));

    size_t const jumpOffset = OffsetOfNextInstruction() - instructionStart;
    checkJumpOffset(jumpOffset, bytecode);

    if (jumpOffset <= 0xFF) {
        this->bytecode[indexOfOffset] = jumpOffset;
        this->bytecode[indexOfOffset + 1] = 0;
    } else {
        // need to use the JUMP2* version of the bytecode
        if (bytecode < FIRST_DOUBLE_BYTE_JUMP_BYTECODE) {
            // still need to bump this one up to
            this->bytecode[instructionStart] += NUM_SINGLE_BYTE_JUMP_BYTECODES;
        }
        assert(IsJumpBytecode(this->bytecode[instructionStart]));
        this->bytecode[indexOfOffset] = jumpOffset & 0xFFU;
        this->bytecode[indexOfOffset + 1] = jumpOffset >> 8U;
    }
}

void MethodGenerationContext::resetLastBytecodeBuffer() {
    last4Bytecodes[0] = BC_INVALID;
    last4Bytecodes[1] = BC_INVALID;
    last4Bytecodes[2] = BC_INVALID;
    last4Bytecodes[3] = BC_INVALID;
}

size_t MethodGenerationContext::getOffsetOfLastBytecode(size_t indexFromEnd) {
    size_t bcOffset = bytecode.size();
    for (size_t i = 0; i < indexFromEnd + 1; i += 1) {
        uint8_t const actual = last4Bytecodes.at(NUM_LAST_BYTECODES - 1 - i);
        if (actual == BC_INVALID) {
            ErrorExit("The requested bytecode is invalid");
        }

        bcOffset -= Bytecode::GetBytecodeLength(actual);
    }
    return bcOffset;
}

void MethodGenerationContext::removeLastBytecodeAt(size_t indexFromEnd) {
    auto const bcOffset = (ptrdiff_t)getOffsetOfLastBytecode(indexFromEnd);

    uint8_t const bcToBeRemoved =
        last4Bytecodes.at(NUM_LAST_BYTECODES - 1 - indexFromEnd);

    auto const bcLength = (ptrdiff_t)Bytecode::GetBytecodeLength(bcToBeRemoved);

    assert(bcLength > 0 && bcOffset >= 0);

    bytecode.erase(bytecode.begin() + bcOffset,
                   bytecode.begin() + bcOffset + bcLength);
}

void MethodGenerationContext::RemoveLastPopForBlockLocalReturn() {
    if (LastBytecodeIs(0, BC_POP)) {
        bytecode.pop_back();
        return;
    }

    if (lastBytecodeIsOneOf(0, IsPopSmthBytecode) != BC_INVALID &&
        !LastBytecodeIs(1, BC_DUP)) {
        // we just removed the DUP and didn't emit the POP using
        // optimizeDupPopPopSequence() so, to make blocks work, we need to
        // reintroduce the DUP
        auto const index =
            (ptrdiff_t)bytecode.size() -
            (ptrdiff_t)Bytecode::GetBytecodeLength(lastBytecodeAt(0));
        assert(IsPopSmthBytecode(bytecode.at(index)));
        bytecode.insert(bytecode.begin() + index, BC_DUP);

        last4Bytecodes[0] = last4Bytecodes[1];
        last4Bytecodes[1] = last4Bytecodes[2];
        last4Bytecodes[2] = BC_DUP;
    }

    if (lastBytecodeAt(0) == BC_INC_FIELD) {
        // we optimized the sequence to an INC_FIELD, which doesn't modify the
        // stack but since we need the value to return it from the block, we
        // need to push it.
        last4Bytecodes[3] = BC_INC_FIELD_PUSH;

        size_t const bcOffset = bytecode.size() - 2;

        // since the bytecodes have the same length, we can just switch the
        // opcode
        assert(Bytecode::GetBytecodeLength(BC_INC_FIELD_PUSH) == 2);
        assert(Bytecode::GetBytecodeLength(BC_INC_FIELD) == 2);
        assert(bytecode[bcOffset] == BC_INC_FIELD);
        bytecode[bcOffset] = BC_INC_FIELD_PUSH;

        currentStackDepth += 1;
        maxStackDepth = max(maxStackDepth, currentStackDepth);
    }
}

bool MethodGenerationContext::OptimizeDupPopPopSequence() {
    // when we are inlining blocks, this already happened
    // and any new opportunities to apply these optimizations are consequently
    // at jump targets for blocks, and we can't remove those
    if (isCurrentlyInliningABlock) {
        return false;
    }

    if (LastBytecodeIs(0, BC_INC_FIELD_PUSH)) {
        return optimizeIncFieldPush();
    }

    uint8_t const popCandidate = lastBytecodeIsOneOf(0, IsPopSmthBytecode);
    if (popCandidate == BC_INVALID) {
        return false;
    }

    if (!LastBytecodeIs(1, BC_DUP)) {
        return false;
    }

    removeLastBytecodeAt(1);  // remove DUP

    // adapt last 4 bytecodes
    assert(last4Bytecodes[3] == popCandidate);
    last4Bytecodes[2] = last4Bytecodes[1];
    last4Bytecodes[1] = last4Bytecodes[0];
    last4Bytecodes[0] = BC_INVALID;

    return true;
}

bool MethodGenerationContext::optimizeIncFieldPush() {
    assert(Bytecode::GetBytecodeLength(BC_INC_FIELD_PUSH) == 2);

    size_t const bcIdx = bytecode.size() - 2;
    assert(bytecode.at(bcIdx) == BC_INC_FIELD_PUSH);

    bytecode[bcIdx] = BC_INC_FIELD;
    last4Bytecodes[3] = BC_INC_FIELD;

    return true;
}

/**
 * Try using a INC_FIELD bytecode instead of the following sequence.
 *
 *  PUSH_FIELD
 *  INC
 *  DUP
 *  POP_FIELD
 *
 * return true, if it optimized it.
 */
bool MethodGenerationContext::OptimizeIncField(uint8_t fieldIdx) {
    if (isCurrentlyInliningABlock) {
        return false;
    }

    if (!LastBytecodeIs(0, BC_DUP)) {
        return false;
    }

    if (!LastBytecodeIs(1, BC_INC)) {
        return false;
    }

    uint8_t const pushCandidate = lastBytecodeIsOneOf(2, IsPushFieldBytecode);
    if (pushCandidate == BC_INVALID) {
        return false;
    }

    assert(Bytecode::GetBytecodeLength(BC_DUP) == 1);
    assert(Bytecode::GetBytecodeLength(BC_INC) == 1);

    size_t const bcOffset = 1 + 1 + Bytecode::GetBytecodeLength(pushCandidate);
    uint8_t candidateFieldIdx = 0;

    switch (pushCandidate) {
        case BC_PUSH_FIELD_0:
            candidateFieldIdx = 0;
            break;
        case BC_PUSH_FIELD_1:
            candidateFieldIdx = 1;
            break;
        case BC_PUSH_FIELD: {
            assert(bytecode.at(bytecode.size() - bcOffset) == pushCandidate);
            candidateFieldIdx = bytecode.at((bytecode.size() - bcOffset) + 1);
            break;
        }

        default:
            ErrorExit("Unexpected bytecode");
            break;
    }

    if (candidateFieldIdx == fieldIdx) {
        removeLastBytecodes(3);
        resetLastBytecodeBuffer();
        EmitIncFieldPush(*this, fieldIdx);
        return true;
    }

    return false;
}

bool MethodGenerationContext::OptimizeReturnField(const Parser& parser) {
    if (isCurrentlyInliningABlock) {
        return false;
    }

    uint8_t const bc = lastBytecodeAt(0);
    uint8_t index = 0;

    switch (bc) {
        case BC_PUSH_FIELD_0:
            index = 0;
            break;

        case BC_PUSH_FIELD_1:
            index = 1;
            break;

        case BC_PUSH_FIELD:
            index = bytecode.at(bytecode.size() - 1);
            assert(index > 1);
            break;

        default:
            return false;
    }

    if (index > 2) {
        // don't have a special bytecode for this
        return false;
    }

    removeLastBytecodes(1);
    resetLastBytecodeBuffer();
    EmitRETURNFIELD(*this, parser, index);
    return true;
}
