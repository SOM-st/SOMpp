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

#include "Parser.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "../misc/ParseInteger.h"
#include "../misc/StringUtil.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Print.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMBigInteger.h"  // NOLINT(misc-include-cleaner)
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h"
#include "BytecodeGenerator.h"
#include "Lexer.h"
#include "MethodGenerationContext.h"

void Parser::GetSym() {
    sym = lexer.GetSym();
    text = lexer.GetText();
}

void Parser::Peek() {
    nextSym = lexer.Peek();
    nextText = lexer.GetNextText();
}

void Parser::PeekForNextSymbolFromLexerIfNecessary() {
    if (!lexer.GetPeekDone()) {
        nextSym = lexer.Peek();
    }
}

Parser::Parser(istream& file, std::string& fname) : lexer(file), fname(fname) {
    GetSym();
}

//
// parsing
//

bool Parser::symIn(Symbol* ss) {
    while (*ss != 0U) {
        if (*ss++ == sym) {
            return true;
        }
    }
    return false;
}

bool Parser::accept(Symbol s) {
    if (sym == s) {
        GetSym();
        return true;
    }
    return false;
}

bool Parser::acceptOneOf(Symbol* ss) {
    if (symIn(ss)) {
        GetSym();
        return true;
    }
    return false;
}

#define _PRINTABLE_SYM (sym == Integer || sym >= STString)

bool Parser::symIsIdentifier() {
    return sym == Identifier || sym == Primitive;
}

bool Parser::expect(Symbol s) {
    if (accept(s)) {
        return true;
    }

    parseError(
        "Unexpected symbol. Expected %(expected)s, but found %(found)s\n", s);
    return false;
}

bool Parser::expectOneOf(Symbol* ss) {
    if (acceptOneOf(ss)) {
        return true;
    }
    parseError(
        "Unexpected symbol. Expected one of %(expected)s, but found "
        "%(found)s\n",
        ss);

    return false;
}

void Parser::genPushVariable(MethodGenerationContext& mgenc,
                             std::string& var) const {
    // The purpose of this function is to find out whether the variable to be
    // pushed on the stack is a local variable, argument, or object field. This
    // is done by examining all available lexical contexts, starting with the
    // innermost (i.e., the one represented by mgenc).
    int64_t index = 0;
    int context = 0;
    bool is_argument = false;

    if (mgenc.FindVar(var, &index, &context, &is_argument)) {
        if (is_argument) {
            EmitPUSHARGUMENT(mgenc, *this, index, context);
        } else {
            EmitPUSHLOCAL(mgenc, *this, index, context);
        }
    } else {
        auto* varSymbol = SymbolFor(var);
        if (mgenc.HasField(varSymbol)) {
            EmitPUSHFIELD(mgenc, *this, varSymbol);
        } else {
            EmitPUSHGLOBAL(mgenc, *this, varSymbol);
        }
    }
}

void Parser::genPopVariable(MethodGenerationContext& mgenc,
                            std::string& var) const {
    // The purpose of this function is to find out whether the variable to be
    // popped off the stack is a local variable, argument, or object field. This
    // is done by examining all available lexical contexts, starting with the
    // innermost (i.e., the one represented by mgenc).
    int64_t index = 0;
    int context = 0;
    bool is_argument = false;

    if (mgenc.FindVar(var, &index, &context, &is_argument)) {
        if (is_argument) {
            EmitPOPARGUMENT(mgenc, *this, index, context);
        } else {
            EmitPOPLOCAL(mgenc, *this, index, context);
        }
    } else {
        auto* varSymbol = SymbolFor(var);
        EmitPOPFIELD(mgenc, *this, varSymbol);
    }
}

//
// grammar
//

static Symbol singleOpSyms[] = {Not,   And,  Or,    Star,  Div,
                                Mod,   Plus, Equal, More,  Less,
                                Comma, At,   Per,   Minus, NONE};

static Symbol binaryOpSyms[] = {Or,   Comma, Minus, Equal, Not,  And,
                                Or,   Star,  Div,   Mod,   Plus, Equal,
                                More, Less,  Comma, At,    Per,  NONE};

static Symbol keywordSelectorSyms[] = {Keyword, KeywordSequence, NONE};

void Parser::Classdef(ClassGenerationContext& cgenc) {
    cgenc.SetName(SymbolFor(text));
    expect(Identifier);

    expect(Equal);

    superclass(cgenc);

    expect(NewTerm);
    instanceFields(cgenc);
    while (symIsIdentifier() || sym == Keyword || sym == OperatorSequence ||
           symIn(binaryOpSyms)) {
        MethodGenerationContext mgenc(cgenc);
        std::string self = strSelf;
        mgenc.AddArgument(self, lexer.GetCurrentSource());

        method(mgenc);

        if (mgenc.IsPrimitive()) {
            cgenc.AddInstanceMethod(mgenc.AssemblePrimitive(false));
        } else {
            cgenc.AddInstanceMethod(mgenc.Assemble());
        }
    }

    if (accept(Separator)) {
        cgenc.SetClassSide(true);
        classFields(cgenc);
        while (symIsIdentifier() || sym == Keyword || sym == OperatorSequence ||
               symIn(binaryOpSyms)) {
            MethodGenerationContext mgenc(cgenc);
            std::string self = strSelf;
            mgenc.AddArgument(self, lexer.GetCurrentSource());

            method(mgenc);

            if (mgenc.IsPrimitive()) {
                cgenc.AddClassMethod(mgenc.AssemblePrimitive(true));
            } else {
                cgenc.AddClassMethod(mgenc.Assemble());
            }
        }
    }
    expect(EndTerm);
}

void Parser::superclass(ClassGenerationContext& cgenc) {
    VMSymbol* superName = nullptr;
    if (sym == Identifier) {
        superName = SymbolFor(text);
        accept(Identifier);
    } else {
        superName = SymbolFor("Object");
    }
    cgenc.SetSuperName(superName);

    // Load the super class, if it is not nil (break the dependency cycle)
    if (superName != SymbolFor("nil")) {
        VMClass* superClass = Universe::LoadClass(superName);
        cgenc.SetInstanceFieldsOfSuper(superClass->GetInstanceFields());
        cgenc.SetClassFieldsOfSuper(
            superClass->GetClass()->GetInstanceFields());
    } else {
        // we hardcode here the field names for Class
        // since Object class superclass = Class
        // We avoid here any kind of dynamic solution to avoid further
        // complexity. However, that makes it static, it is going to make it
        // harder to change the definition of Class and Object
        vector<std::string> const fieldNamesOfClass{
            "class", "name", "instanceFields", "instanceInvokables",
            "superClass"};
        VMArray* fieldNames =
            Universe::NewArrayOfSymbolsFromStrings(fieldNamesOfClass);
        cgenc.SetClassFieldsOfSuper(fieldNames);
    }
}

void Parser::instanceFields(ClassGenerationContext& cgenc) {
    if (accept(Or)) {
        while (symIsIdentifier()) {
            auto v = variable();
            cgenc.AddInstanceField(SymbolFor(v));
        }
        expect(Or);
    }
}

void Parser::classFields(ClassGenerationContext& cgenc) {
    if (accept(Or)) {
        while (symIsIdentifier()) {
            auto v = variable();
            cgenc.AddClassField(SymbolFor(v));
        }
        expect(Or);
    }
}

void Parser::method(MethodGenerationContext& mgenc) {
    pattern(mgenc);

    expect(Equal);
    if (sym == Primitive) {
        mgenc.SetPrimitive(true);
        primitiveBlock();
    } else {
        methodBlock(mgenc);
    }
}

void Parser::primitiveBlock() {
    expect(Primitive);
}

void Parser::pattern(MethodGenerationContext& mgenc) {
    switch (sym) {
        case Identifier:
        case Primitive:
            unaryPattern(mgenc);
            break;
        case Keyword:
            keywordPattern(mgenc);
            break;
        default:
            binaryPattern(mgenc);
            break;
    }
}

void Parser::unaryPattern(MethodGenerationContext& mgenc) {
    mgenc.SetSignature(unarySelector());
}

void Parser::binaryPattern(MethodGenerationContext& mgenc) {
    mgenc.SetSignature(binarySelector());

    auto source = lexer.GetCurrentSource();
    auto a = argument();
    mgenc.AddArgumentIfAbsent(a, source);
}

void Parser::keywordPattern(MethodGenerationContext& mgenc) {
    std::string kw;
    do {
        kw.append(keyword());

        auto source = lexer.GetCurrentSource();
        auto a = argument();
        mgenc.AddArgumentIfAbsent(a, source);
    } while (sym == Keyword);

    mgenc.SetSignature(SymbolFor(kw));
}

void Parser::methodBlock(MethodGenerationContext& mgenc) {
    expect(NewTerm);
    blockContents(mgenc, false);
    // if no return has been generated so far, we can be sure there was no .
    // terminating the last expression, so the last expression's value must be
    // popped off the stack and a ^self be generated
    if (!mgenc.IsFinished()) {
        // don't need to pop of the value, because RETURN_SELF doesn't need
        // stack space
        EmitRETURNSELF(mgenc);
        mgenc.MarkFinished();
    }

    expect(EndTerm);
}

VMSymbol* Parser::unarySelector() {
    return SymbolFor(identifier());
}

VMSymbol* Parser::binarySelector() {
    std::string const s(text);

    if (acceptOneOf(singleOpSyms) || accept(OperatorSequence)) {
    } else {
        expect(NONE);
    }

    VMSymbol* symb = SymbolFor(s);
    return symb;
}

std::string Parser::identifier() {
    std::string s(text);
    if (accept(Primitive)) {
        // text is set
    } else {
        expect(Identifier);
    }

    return s;
}

std::string Parser::keyword() {
    std::string s(text);
    expect(Keyword);

    return s;
}

std::string Parser::argument() {
    return variable();
}

void Parser::blockContents(MethodGenerationContext& mgenc, bool is_inlined) {
    if (accept(Or)) {
        locals(mgenc);
        expect(Or);
    }

    mgenc.CompleteLexicalScope();
    blockBody(mgenc, false, is_inlined);
}

void Parser::locals(MethodGenerationContext& mgenc) {
    while (symIsIdentifier()) {
        auto source = lexer.GetCurrentSource();
        auto v = variable();
        mgenc.AddLocalIfAbsent(v, source);
    }
}

void Parser::blockBody(MethodGenerationContext& mgenc, bool seen_period,
                       bool is_inlined) {
    if (accept(Exit)) {
        result(mgenc);
    } else if (sym == EndBlock) {
        if (seen_period) {
            // a POP has been generated which must be elided (blocks always
            // return the value of the last expression, regardless of whether it
            // was terminated with a . or not)
            mgenc.RemoveLastPopForBlockLocalReturn();
        }
        if (!is_inlined) {
            // if the block is empty, we need to return nil
            if (mgenc.IsBlockMethod() && !mgenc.HasBytecodes()) {
                EmitPUSHCONSTANT(mgenc, *this, load_ptr(nilObject));
            }
            EmitRETURNLOCAL(mgenc, *this);
            mgenc.MarkFinished();
        }
    } else if (sym == EndTerm) {
        // it does not matter whether a period has been seen, as the end of the
        // method has been found (EndTerm). It's safe to emit a "return self"
        EmitRETURNSELF(mgenc);
        mgenc.MarkFinished();
    } else {
        expression(mgenc);
        if (accept(Period)) {
            EmitPOP(mgenc);
            blockBody(mgenc, true, is_inlined);
        }
    }
}

void Parser::result(MethodGenerationContext& mgenc) {
    // try first to parse a `^ self` to emit RETURN_SELF
    if (!mgenc.IsBlockMethod() && sym == Identifier && text == strSelf) {
        PeekForNextSymbolFromLexerIfNecessary();
        if (nextSym == Period || nextSym == EndTerm) {
            expect(Identifier);

            EmitRETURNSELF(mgenc);
            mgenc.MarkFinished();

            accept(Period);
            return;
        }
    }

    expression(mgenc);

    if (mgenc.IsBlockMethod()) {
        EmitRETURNNONLOCAL(mgenc);
    } else {
        EmitRETURNLOCAL(mgenc, *this);
    }

    mgenc.MarkFinished();
    accept(Period);
}

void Parser::expression(MethodGenerationContext& mgenc) {
    PeekForNextSymbolFromLexerIfNecessary();
    if (nextSym == Assign) {
        assignation(mgenc);
    } else {
        evaluation(mgenc);
    }
}

void Parser::assignation(MethodGenerationContext& mgenc) {
    list<std::string> l;

    assignments(mgenc, l);
    evaluation(mgenc);
    list<std::string>::iterator i;
    for (i = l.begin(); i != l.end(); ++i) {
        EmitDUP(mgenc);
    }

    for (i = l.begin(); i != l.end(); ++i) {
        genPopVariable(mgenc, (*i));
    }
}

void Parser::assignments(MethodGenerationContext& mgenc, list<std::string>& l) {
    if (symIsIdentifier()) {
        l.push_back(assignment());
        Peek();

        if (nextSym == Assign) {
            assignments(mgenc, l);
        }
    }
}

std::string Parser::assignment() {
    auto v = variable();

    expect(Assign);

    return v;
}

void Parser::evaluation(MethodGenerationContext& mgenc) {
    bool const super = primary(mgenc);
    if (symIsIdentifier() || sym == Keyword || sym == OperatorSequence ||
        symIn(binaryOpSyms)) {
        messages(mgenc, super);
    }
}

bool Parser::primary(MethodGenerationContext& mgenc) {
    bool super = false;
    switch (sym) {
        case Primitive:
        case Identifier: {
            auto v = variable();
            if (v == strSuper) {
                super = true;
                // sends to super push self as the receiver
                v = strSelf;
            }

            genPushVariable(mgenc, v);
            break;
        }
        case NewTerm:
            nestedTerm(mgenc);
            break;
        case NewBlock: {
            MethodGenerationContext bgenc{*mgenc.GetHolder(), &mgenc};

            nestedBlock(bgenc);

            VMInvokable* blockMethod = bgenc.Assemble();
            EmitPUSHBLOCK(mgenc, *this, blockMethod);
            break;
        }
        default:
            literal(mgenc);
            break;
    }

    return super;
}

std::string Parser::variable() {
    return identifier();
}

void Parser::messages(MethodGenerationContext& mgenc, bool super) {
    if (symIsIdentifier()) {
        do {
            // only the first message in a sequence can be a super send
            unaryMessage(mgenc, super);
            super = false;
        } while (symIsIdentifier());

        while (sym == OperatorSequence || symIn(binaryOpSyms)) {
            binaryMessage(mgenc, false);
        }

        if (sym == Keyword) {
            keywordMessage(mgenc, false);
        }
    } else if (sym == OperatorSequence || symIn(binaryOpSyms)) {
        do {
            // only the first message in a sequence can be a super send
            binaryMessage(mgenc, super);
            super = false;
        } while (sym == OperatorSequence || symIn(binaryOpSyms));

        if (sym == Keyword) {
            keywordMessage(mgenc, false);
        }
    } else {
        keywordMessage(mgenc, super);
    }
}

void Parser::unaryMessage(MethodGenerationContext& mgenc, bool super) {
    VMSymbol* msg = unarySelector();

    if (super) {
        EmitSUPERSEND(mgenc, *this, msg);
    } else {
        EmitSEND(mgenc, *this, msg);
    }
}

bool Parser::tryIncOrDecBytecodes(VMSymbol* msg, bool isSuperSend,
                                  MethodGenerationContext& mgenc) {
    if (isSuperSend) {
        return false;
    }

    bool const isPlus = msg == load_ptr(symbolPlus);
    bool const isMinus = msg == load_ptr(symbolMinus);

    if (!isPlus && !isMinus) {
        return false;
    }

    if (sym != Integer || text != "1") {
        return false;
    }

    expect(Integer);
    if (isPlus) {
        EmitINC(mgenc);
    } else {
        assert(isMinus);
        EmitDEC(mgenc);
    }
    return true;
}

void Parser::binaryMessage(MethodGenerationContext& mgenc, bool super) {
    std::string const msgSelector(text);
    VMSymbol* msg = binarySelector();

    if (tryIncOrDecBytecodes(msg, super, mgenc)) {
        return;
    }

    binaryOperand(mgenc);

    if (!super && ((msgSelector == "||" && mgenc.InlineAndOr(*this, true)) ||
                   (msgSelector == "&&" && mgenc.InlineAndOr(*this, false)))) {
        return;
    }

    if (super) {
        EmitSUPERSEND(mgenc, *this, msg);
    } else {
        EmitSEND(mgenc, *this, msg);
    }
}

bool Parser::binaryOperand(MethodGenerationContext& mgenc) {
    bool super = primary(mgenc);

    while (symIsIdentifier()) {
        unaryMessage(mgenc, super);
        super = false;
    }

    return super;
}

void Parser::keywordMessage(MethodGenerationContext& mgenc, bool super) {
    std::string kw = keyword();
    int numParts = 1;

    formula(mgenc);
    while (sym == Keyword) {
        kw.append(keyword());
        numParts += 1;
        formula(mgenc);
    }

    if (!super) {
        if (numParts == 1 &&
            ((kw == "ifTrue:" &&
              mgenc.InlineThenBranch(*this, JumpCondition::ON_FALSE)) ||
             (kw == "ifFalse:" &&
              mgenc.InlineThenBranch(*this, JumpCondition::ON_TRUE)) ||
             (kw == "ifNil:" &&
              mgenc.InlineThenBranch(*this, JumpCondition::ON_NOT_NIL)) ||
             (kw == "ifNotNil:" &&
              mgenc.InlineThenBranch(*this, JumpCondition::ON_NIL)) ||
             (kw == "whileTrue:" && mgenc.InlineWhile(*this, true)) ||
             (kw == "whileFalse:" && mgenc.InlineWhile(*this, false)) ||
             (kw == "or:" && mgenc.InlineAndOr(*this, true)) ||
             (kw == "and:" && mgenc.InlineAndOr(*this, false)))) {
            return;
        }

        if (numParts == 2 &&
            ((kw == "ifTrue:ifFalse:" &&
              mgenc.InlineThenElseBranches(*this, JumpCondition::ON_FALSE)) ||
             (kw == "ifFalse:ifTrue:" &&
              mgenc.InlineThenElseBranches(*this, JumpCondition::ON_TRUE)) ||
             (kw == "ifNil:ifNotNil:" &&
              mgenc.InlineThenElseBranches(*this, JumpCondition::ON_NOT_NIL)) ||
             (kw == "ifNotNil:ifNil:" &&
              mgenc.InlineThenElseBranches(*this, JumpCondition::ON_NIL)) ||
             (kw == "to:do:" && mgenc.InlineToDo(*this)))) {
            return;
        }
    }

    VMSymbol* msg = SymbolFor(kw);

    if (super) {
        EmitSUPERSEND(mgenc, *this, msg);
    } else {
        EmitSEND(mgenc, *this, msg);
    }
}

void Parser::formula(MethodGenerationContext& mgenc) {
    bool const super = binaryOperand(mgenc);

    // only the first message in a sequence can be a super send
    if (sym == OperatorSequence || symIn(binaryOpSyms)) {
        binaryMessage(mgenc, super);
    }

    while (sym == OperatorSequence || symIn(binaryOpSyms)) {
        binaryMessage(mgenc, false);
    }
}

void Parser::nestedTerm(MethodGenerationContext& mgenc) {
    expect(NewTerm);
    expression(mgenc);
    expect(EndTerm);
}

void Parser::literal(MethodGenerationContext& mgenc) {
    switch (sym) {
        case Pound:
            PeekForNextSymbolFromLexerIfNecessary();
            if (nextSym == NewTerm) {
                literalArray(mgenc);
            } else {
                literalSymbol(mgenc);
            }
            break;
        case STString:
            literalString(mgenc);
            break;
        default:
            literalNumber(mgenc);
            break;
    }
}

vm_oop_t Parser::literalNumberOop() {
    if (sym == Minus) {
        return negativeDecimal();
    }
    return literalDecimal(false);
}

void Parser::literalNumber(MethodGenerationContext& mgenc) {
    vm_oop_t lit = literalNumberOop();
    EmitPUSHCONSTANT(mgenc, *this, lit);
}

vm_oop_t Parser::literalDecimal(bool negateValue) {
    if (sym == Integer) {
        return literalInteger(negateValue);
    }
    return literalDouble(negateValue);
}

vm_oop_t Parser::negativeDecimal() {
    expect(Minus);
    return literalDecimal(true);
}

vm_oop_t Parser::literalInteger(bool negateValue) {
    vm_oop_t i = ParseInteger(text.c_str(), negateValue);
    expect(Integer);
    return i;
}

vm_oop_t Parser::literalDouble(bool negateValue) {
    double d = std::strtod(text.c_str(), nullptr);
    if (negateValue) {
        d = 0 - d;
    }
    expect(Double);
    return Universe::NewDouble(d);
}

void Parser::literalSymbol(MethodGenerationContext& mgenc) {
    VMSymbol* symb = nullptr;
    expect(Pound);
    if (sym == STString) {
        std::string const s = _string();
        symb = SymbolFor(s);

    } else {
        symb = selector();
    }
    EmitPUSHCONSTANT(mgenc, *this, symb);
}

void Parser::literalArray(MethodGenerationContext& mgenc) {
    expect(Pound);
    expect(NewTerm);

    VMSymbol* arrayClassName = SymbolFor("Array");
    VMSymbol* arraySizePlaceholder = SymbolFor("ArraySizeLiteralPlaceholder");
    VMSymbol* newMessage = SymbolFor("new:");
    VMSymbol* atPutMessage = SymbolFor("at:put:");

    const uint8_t arraySizeLiteralIndex =
        mgenc.AddLiteral(arraySizePlaceholder, *this);

    // create bytecode sequence for instantiating new array
    EmitPUSHGLOBAL(mgenc, *this, arrayClassName);
    EmitPUSHCONSTANT(mgenc, arraySizeLiteralIndex);
    EmitSEND(mgenc, *this, newMessage);

    int64_t i = 1;

    while (sym != EndTerm) {
        vm_oop_t pushIndex = NEW_INT(i);
        EmitPUSHCONSTANT(mgenc, *this, pushIndex);
        literal(mgenc);
        EmitSEND(mgenc, *this, atPutMessage);
        i += 1;
    }

    // replace the placeholder with the actual array size
    mgenc.UpdateLiteral(arraySizePlaceholder, arraySizeLiteralIndex,
                        NEW_INT(i - 1));

    expect(EndTerm);
}

void Parser::literalString(MethodGenerationContext& mgenc) {
    std::string const s = _string();

    VMString* str = Universe::NewString(s);
    EmitPUSHCONSTANT(mgenc, *this, str);
}

VMSymbol* Parser::selector() {
    if (sym == OperatorSequence || symIn(singleOpSyms)) {
        return binarySelector();
    }
    if (sym == Keyword || sym == KeywordSequence) {
        return keywordSelector();
    }
    return unarySelector();
}

VMSymbol* Parser::keywordSelector() {
    std::string const s(text);
    expectOneOf(keywordSelectorSyms);
    VMSymbol* symb = SymbolFor(s);
    return symb;
}

std::string Parser::_string() {
    std::string s(text);
    expect(STString);
    return s;  // <-- Literal strings are At Most BUFSIZ chars long.
}

void Parser::nestedBlock(MethodGenerationContext& mgenc) {
    std::string blockSelf = strBlockSelf;
    mgenc.AddArgumentIfAbsent(blockSelf, lexer.GetCurrentSource());

    expect(NewBlock);
    if (sym == Colon) {
        blockPattern(mgenc);
    }

    // generate Block signature
    std::string block_sig =
        "$blockMethod@" + to_string(lexer.GetCurrentLineNumber());
    uint8_t const arg_size = mgenc.GetNumberOfArguments();
    for (uint8_t i = 1; i < arg_size; i++) {
        block_sig += ":";
    }

    mgenc.SetSignature(SymbolFor(block_sig));

    blockContents(mgenc, false);

    // if no return has been generated, we can be sure that the last expression
    // in the block was not terminated by ., and can generate a return
    if (!mgenc.IsFinished()) {
        if (!mgenc.HasBytecodes()) {
            // if the block is empty, we need to return nil
            EmitPUSHCONSTANT(mgenc, *this, load_ptr(nilObject));
        }
        EmitRETURNLOCAL(mgenc, *this);
        mgenc.MarkFinished();
    }

    expect(EndBlock);
}

void Parser::blockPattern(MethodGenerationContext& mgenc) {
    blockArguments(mgenc);
    expect(Or);
}

void Parser::blockArguments(MethodGenerationContext& mgenc) {
    do {
        expect(Colon);

        auto source = lexer.GetCurrentSource();
        auto a = argument();
        mgenc.AddArgumentIfAbsent(a, source);

    } while (sym == Colon);
}

__attribute__((noreturn)) void Parser::parseError(const char* msg,
                                                  Symbol expected) {
    std::string const expectedStr(symnames[expected]);
    parseError(msg, expectedStr);
}

__attribute__((noreturn)) void Parser::parseError(const char* msg,
                                                  std::string expected) {
    std::string msgWithMeta =
        "%(file)s:%(line)d:%(column)d: error: " + std::string(msg);

    std::string found;
    if (_PRINTABLE_SYM) {
        found = symnames[sym] + std::string(" (") + text + ")";
    } else {
        found = symnames[sym];
    }

    ReplacePattern(msgWithMeta, "%(file)s", fname);

    std::string line = std::to_string(lexer.GetCurrentLineNumber());
    ReplacePattern(msgWithMeta, "%(line)d", line);

    std::string column = std::to_string(lexer.GetCurrentColumn());
    ReplacePattern(msgWithMeta, "%(column)d", column);
    ReplacePattern(msgWithMeta, "%(expected)s", expected);
    ReplacePattern(msgWithMeta, "%(found)s", found);

    ErrorPrint(msgWithMeta);
    Quit(ERR_FAIL);
}

__attribute__((noreturn)) void Parser::ParseError(const char* msg) const {
    std::string msgWithMeta =
        "%(file)s:%(line)d:%(column)d: error: " + std::string(msg) + "\n";

    ReplacePattern(msgWithMeta, "%(file)s", fname);

    std::string line = std::to_string(lexer.GetCurrentLineNumber());
    ReplacePattern(msgWithMeta, "%(line)d", line);

    std::string column = std::to_string(lexer.GetCurrentColumn());
    ReplacePattern(msgWithMeta, "%(column)d", column);

    ErrorPrint(msgWithMeta);
    Quit(ERR_FAIL);
}

__attribute__((noreturn)) void Parser::parseError(const char* msg,
                                                  Symbol* expected) {
    bool first = true;
    std::string expectedStr;

    Symbol* next = expected;
    while (*next != 0U) {
        if (first) {
            first = false;
        } else {
            expectedStr += ", ";
        }

        expectedStr += symnames[*next];
        next += 1;
    }

    parseError(msg, expectedStr);
}
