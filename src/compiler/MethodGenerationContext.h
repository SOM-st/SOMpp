#pragma once

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

#include <array>
#include <vector>

#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMMethod.h"
#include "ClassGenerationContext.h"
#include "LexicalScope.h"
#include "SourceCoordinate.h"

class Parser;

#define NUM_LAST_BYTECODES 4

enum JumpCondition : std::uint8_t { ON_TRUE, ON_FALSE, ON_NIL, ON_NOT_NIL };

class MethodGenerationContext {
public:
    explicit MethodGenerationContext(ClassGenerationContext& holder,
                                     MethodGenerationContext* outer = nullptr);
    ~MethodGenerationContext();

    VMInvokable* Assemble();
    VMInvokable* AssemblePrimitive(bool classSide);

    int8_t FindLiteralIndex(vm_oop_t lit);
    bool FindVar(std::string& var, int64_t* index, int* context,
                 bool* isArgument);
    bool HasField(VMSymbol* field);

    int64_t GetFieldIndex(VMSymbol* field);

    void SetSignature(VMSymbol* sig);

    void AddArgument(std::string& arg, const SourceCoordinate& coord);
    void AddLocal(std::string& local, const SourceCoordinate& coord);
    bool AddArgumentIfAbsent(std::string& arg, const SourceCoordinate& coord);
    bool AddLocalIfAbsent(std::string& local, const SourceCoordinate& coord);

    void SetPrimitive(bool prim = true);

    uint8_t AddLiteral(vm_oop_t lit);
    uint8_t AddLiteralIfAbsent(vm_oop_t lit);
    void UpdateLiteral(vm_oop_t oldValue, uint8_t index, vm_oop_t newValue);

    void MarkFinished();

    [[nodiscard]] ClassGenerationContext* GetHolder() const {
        return &holderGenc;
    }

    [[nodiscard]] MethodGenerationContext* GetOuter() const {
        return outerGenc;
    }

    [[nodiscard]] uint8_t GetMaxContextLevel() const { return maxContextLevel; }

    VMSymbol* GetSignature() { return signature; }

    [[nodiscard]] bool IsPrimitive() const { return primitive; }

    [[nodiscard]] bool IsBlockMethod() const { return blockMethod; }

    [[nodiscard]] bool IsFinished() const { return finished; }

    void RemoveLastBytecode() { bytecode.pop_back(); };
    void RemoveLastPopForBlockLocalReturn();

    uint8_t GetNumberOfArguments();
    void AddBytecode(uint8_t bc, int64_t stackEffect);
    void AddBytecodeArgument(uint8_t bc);
    size_t AddBytecodeArgumentAndGetIndex(uint8_t bc);

    bool HasBytecodes();

    std::vector<uint8_t> GetBytecodes() { return bytecode; }

    bool InlineWhile(Parser& parser, bool isWhileTrue);
    bool InlineThenElseBranches(JumpCondition condition);
    bool InlineThenBranch(JumpCondition condition);
    bool InlineAndOr(bool isOr);
    bool InlineToDo();

    inline size_t OffsetOfNextInstruction() { return bytecode.size(); }

    void CompleteLexicalScope();
    void MergeIntoScope(LexicalScope& scopeToBeInlined);
    void InlineAsLocals(vector<Variable>& vars);

    uint8_t GetInlinedLocalIdx(const Variable* var) const;
    const Variable* GetInlinedVariable(const Variable* oldVar) const;

    void EmitBackwardsJumpOffsetToTarget(size_t loopBeginIdx);
    void PatchJumpOffsetToPointToNextInstruction(size_t indexOfOffset);

    bool OptimizeDupPopPopSequence();
    bool OptimizeIncField(uint8_t fieldIdx);
    bool OptimizeReturnField();

    bool LastBytecodeIs(size_t indexFromEnd, uint8_t bytecode);

private:
    VMTrivialMethod* assembleTrivialMethod();
    VMTrivialMethod* assembleLiteralReturn(uint8_t pushCandidate);
    VMTrivialMethod* assembleGlobalReturn();
    VMTrivialMethod* assembleFieldGetter(uint8_t pushCandidate);
    VMTrivialMethod* assembleFieldSetter();
    VMTrivialMethod* assembleFieldGetterFromReturn(uint8_t returnCandidate);

    bool optimizeIncFieldPush();

    void removeLastBytecodes(size_t numBytecodes);
    void removeLastBytecodeAt(size_t indexFromEnd);

    bool hasOneLiteralBlockArgument();
    bool hasTwoLiteralBlockArguments();
    uint8_t lastBytecodeAt(size_t indexFromEnd);

    uint8_t lastBytecodeIsOneOf(size_t indexFromEnd,
                                uint8_t (*predicate)(uint8_t));

    size_t getOffsetOfLastBytecode(size_t indexFromEnd);

    std::tuple<VMInvokable*, VMInvokable*>
    extractBlockMethodsAndRemoveBytecodes();
    VMInvokable* extractBlockMethodAndRemoveBytecode();

    VMInvokable* getLastBlockMethodAndFreeLiteral(uint8_t blockLiteralIdx);

    void completeJumpsAndEmitReturningNil(Parser& parser, size_t loopBeginIdx,
                                          size_t jumpOffsetIdxToSkipLoopBody);

    static void checkJumpOffset(size_t jumpOffset, uint8_t bytecode);
    void resetLastBytecodeBuffer();

    ClassGenerationContext& holderGenc;
    MethodGenerationContext* const outerGenc;

    const uint8_t maxContextLevel;

    const bool blockMethod;
    VMSymbol* signature{nullptr};
    std::vector<Variable> arguments;
    bool primitive{false};
    std::vector<Variable> locals;
    std::vector<vm_oop_t> literals;
    bool finished{false};
    std::vector<uint8_t> bytecode;
    LexicalScope* lexicalScope{nullptr};

    size_t currentStackDepth{0};
    size_t maxStackDepth{0};

    std::array<uint8_t, NUM_LAST_BYTECODES> last4Bytecodes;

    std::vector<BackJump> inlinedLoops;

    bool isCurrentlyInliningABlock{false};

    make_testable(public);
    vm_oop_t GetLiteral(size_t idx) { return literals.at(idx); }
};
