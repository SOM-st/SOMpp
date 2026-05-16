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

#include "Disassembler.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../interpreter/bytecodes.h"
#include "../misc/debug.h"
#include "../vm/Globals.h"
#include "../vm/Print.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/Signature.h"
#include "../vmobjects/VMBigInteger.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h"

/**
 * Dispatch an object to its content and write out
 */
void Disassembler::dispatch(vm_oop_t o) {
    if (o == nullptr) {
        return;  // nullptr isn't interesting.
    }
    if (o == load_ptr(nilObject)) {
        DebugPrint("{Nil}");
    } else if (o == load_ptr(trueObject)) {
        DebugPrint("{True}");
    } else if (o == load_ptr(falseObject)) {
        DebugPrint("{False}");
    } else if (o == load_ptr(systemClass)) {
        DebugPrint("{System Class object}");
    } else if (o == load_ptr(blockClass)) {
        DebugPrint("{Block Class object}");
    } else if (o == Universe::GetGlobal(SymbolFor("system"))) {
        DebugPrint("{System}");
    } else if (IS_SMALL_INT(o)) {
        DebugPrint("%lld", SMALL_INT_VAL(o));
    } else if (IS_BIG_INT(o)) {
        DebugPrint("%s", AS_BIG_INT(o)->embeddedInteger.toString().c_str());
    } else {
        VMClass* c = CLASS_OF(o);
        if (c == load_ptr(stringClass)) {
            DebugPrint("\"%s\"",
                       static_cast<VMString*>(o)->GetStdString().c_str());
        } else if (c == load_ptr(doubleClass)) {
            DebugPrint("%g", static_cast<VMDouble*>(o)->GetEmbeddedDouble());
        } else if (c == load_ptr(symbolClass)) {
            DebugPrint("#%s",
                       static_cast<VMSymbol*>(o)->GetStdString().c_str());
        } else if (c == load_ptr(methodClass)) {
            DebugPrint("#%s",
                       static_cast<VMMethod*>(o)
                           ->GetSignature()
                           ->GetStdString()
                           .c_str());
        } else if (c == load_ptr(primitiveClass)) {
            DebugPrint("#%s",
                       static_cast<VMPrimitive*>(o)
                           ->GetSignature()
                           ->GetStdString()
                           .c_str());
        } else {
            DebugPrint("address: %p", (void*)o);
        }
    }
}

/**
 * Dump a class and all subsequent methods.
 */
void Disassembler::Dump(VMClass* cl) {
    size_t const numInvokables = cl->GetNumberOfInstanceInvokables();
    for (size_t i = 0; i < numInvokables; ++i) {
        auto* inv = cl->GetInstanceInvokable(i);
        // output header and skip if the Invokable is a Primitive
        VMSymbol* sig = inv->GetSignature();
        VMSymbol* cname = cl->GetName();
        DebugDump("%s>>%s = ", cname->GetStdString().c_str(),
                  sig->GetStdString().c_str());

        inv->Dump("\t", true);
    }
}

/**
 * Dump all Bytecode of a method.
 */
void Disassembler::DumpMethod(VMMethod* method, const char* indent,
                              bool printObjects) {
    dumpMethod(method->GetBytecodes(), method->GetNumberOfBytecodes(), indent,
               method, printObjects);
}

void Disassembler::DumpMethod(MethodGenerationContext* mgenc,
                              const char* indent) {
    auto bytecodes = mgenc->GetBytecodes();
    dumpMethod(bytecodes.data(), bytecodes.size(), indent, nullptr, true);
}

static void PrintFieldAccess(VMMethod* method, bool printObjects,
                             uint8_t const fieldIdx) {
    if (method != nullptr && printObjects) {
        auto* holder = dynamic_cast<VMClass*>((VMObject*)method->GetHolder());
        if (holder != nullptr) {
            VMSymbol* name = holder->GetInstanceFieldName(fieldIdx);
            if (name != nullptr) {
                DebugPrint("(index: %d) field: %s\n", fieldIdx,
                           name->GetStdString().c_str());
            } else {
                DebugPrint("(index: %d) field: !nullptr!: error!\n", fieldIdx);
            }
        } else {
            DebugPrint("(index: %d) block holder is not a class!!\n", fieldIdx);
        }
    } else {
        DebugPrint("(index: %d)\n", fieldIdx);
    }
}

void Disassembler::dumpMethod(uint8_t* bytecodes, size_t numberOfBytecodes,
                              const char* indent, VMMethod* method,
                              bool printObjects) {
    DebugPrint("(\n");
    if (method != nullptr) {  // output stack information
        size_t const locals = method->GetNumberOfLocals();
        size_t const max_stack = method->GetMaximumNumberOfStackElements();
        DebugDump("%s<%d locals, %d stack, %d bc_count>\n", indent, locals,
                  max_stack, method->GetNumberOfBytecodes());

#ifdef _DEBUG
        Print("bytecodes: ");
        for (size_t i = 0; i < numberOfBytecodes; ++i) {
            Print(to_string((int)(*method)[i]) + " ");
        }
        Print("\n");
#endif
    }

    // output bytecodes
    for (size_t bc_idx = 0; bc_idx < numberOfBytecodes;
         bc_idx += Bytecode::GetBytecodeLength(bytecodes[bc_idx])) {
        // the bytecode.
        uint8_t const bytecode = bytecodes[bc_idx];
        // indent, bytecode index, bytecode mnemonic
        DebugDump("%s%4d:%s  ", indent, bc_idx,
                  Bytecode::GetBytecodeName(bytecode));
        // parameters (if any)
        if (Bytecode::GetBytecodeLength(bytecode) == 1) {
            DebugPrint("\n");
            continue;
        }

        switch (bytecode) {
            case BC_PUSH_0:
            case BC_PUSH_1:
            case BC_PUSH_NIL:
            case BC_RETURN_SELF: {
                // no more details to be printed
                break;
            }
            case BC_PUSH_LOCAL_0: {
                DebugPrint("local: 0, context: 0\n");
                break;
            }
            case BC_PUSH_LOCAL_1: {
                DebugPrint("local: 1, context: 0\n");
                break;
            }
            case BC_PUSH_LOCAL_2: {
                DebugPrint("local: 2, context: 0\n");
                break;
            }
            case BC_PUSH_LOCAL: {
                DebugPrint("local: %d, context: %d\n", bytecodes[bc_idx + 1],
                           bytecodes[bc_idx + 2]);
                break;
            }
            case BC_PUSH_ARGUMENT: {
                DebugPrint("argument: %d, context %d\n", bytecodes[bc_idx + 1],
                           bytecodes[bc_idx + 2]);
                break;
            }

            case BC_PUSH_BLOCK: {
                size_t const indent_size = strlen(indent) + 1 + 1;
                char* nindent = new char[indent_size];
                DebugPrint("block: (index: %d) ", bytecodes[bc_idx + 1]);

                if (method != nullptr && printObjects) {
                    if (snprintf(nindent, indent_size, "%s\t", indent) < 0) {
                        ErrorExit("snprintf failed");
                    }
                    Disassembler::DumpMethod(
                        static_cast<VMMethod*>(method->GetConstant(bc_idx)),
                        nindent);
                } else {
                    DebugPrint("\n");
                }
                delete[] nindent;
                break;
            }
            case BC_PUSH_CONSTANT: {
                if (method != nullptr && printObjects) {
                    vm_oop_t constant = method->GetConstant(bc_idx);
                    VMClass* cl = CLASS_OF(constant);
                    if (cl == nullptr) {
                        DebugPrint("(index: %d) value: (%s) ",
                                   bytecodes[bc_idx + 1],
                                   "class==nullptr");
                    } else {
                        VMSymbol* cname = cl->GetName();
                        DebugPrint("(index: %d) value: (%s) ",
                                   bytecodes[bc_idx + 1],
                                   cname->GetStdString().c_str());
                    }
                    dispatch(constant);
                } else {
                    DebugPrint("(index: %d)", bytecodes[bc_idx + 1]);
                }
                DebugPrint("\n");
                break;
            }
            case BC_PUSH_GLOBAL: {
                if (method != nullptr && printObjects) {
                    vm_oop_t cst = method->GetConstant(bc_idx);
                    if (cst != nullptr) {
                        auto* name = static_cast<VMSymbol*>(cst);
                        if (name != nullptr) {
                            DebugPrint("(index: %d) value: %s\n",
                                       bytecodes[bc_idx + 1],
                                       name->GetStdString().c_str());
                        } else {
                            DebugPrint("(index: %d) value: !nullptr!: error!\n",
                                       bytecodes[bc_idx + 1]);
                        }
                        break;
                    }
                }

                DebugPrint("(index: %d)\n", bytecodes[bc_idx + 1]);
                break;
            }
            case BC_POP_LOCAL:
                DebugPrint("local: %d, context: %d\n", bytecodes[bc_idx + 1],
                           bytecodes[bc_idx + 2]);
                break;
            case BC_POP_LOCAL_0:
                DebugPrint("local: 0, context: 0\n");
                break;
            case BC_POP_LOCAL_1:
                DebugPrint("local: 1, context: 0\n");
                break;
            case BC_POP_LOCAL_2:
                DebugPrint("local: 2, context: 0\n");
                break;
            case BC_POP_ARGUMENT:
                DebugPrint("argument: %d, context: %d\n", bytecodes[bc_idx + 1],
                           bytecodes[bc_idx + 2]);
                break;
            case BC_INC_FIELD:
            case BC_INC_FIELD_PUSH:
            case BC_POP_FIELD:
            case BC_PUSH_FIELD: {
                uint8_t const fieldIdx = bytecodes[bc_idx + 1];
                PrintFieldAccess(method, printObjects, fieldIdx);
                break;
            }
            case BC_POP_FIELD_1:
            case BC_PUSH_FIELD_1: {
                PrintFieldAccess(method, printObjects, 1);
                break;
            }
            case BC_SEND:
            case BC_SEND_1: {
                if (method != nullptr && printObjects) {
                    auto* name =
                        static_cast<VMSymbol*>(method->GetConstant(bc_idx));
                    DebugPrint("(index: %d) signature: %s\n",
                               bytecodes[bc_idx + 1],
                               name->GetStdString().c_str());
                } else {
                    DebugPrint("(index: %d)\n", bytecodes[bc_idx + 1]);
                }
                break;
            }
            case BC_SUPER_SEND: {
                if (method != nullptr && printObjects) {
                    auto* name =
                        static_cast<VMSymbol*>(method->GetConstant(bc_idx));
                    DebugPrint("(index: %d) signature: %s\n",
                               bytecodes[bc_idx + 1],
                               name->GetStdString().c_str());
                } else {
                    DebugPrint("(index: %d)\n", bytecodes[bc_idx + 1]);
                }
                break;
            }
            case BC_JUMP:
            case BC_JUMP_ON_FALSE_POP:
            case BC_JUMP_ON_TRUE_POP:
            case BC_JUMP_ON_FALSE_TOP_NIL:
            case BC_JUMP_ON_TRUE_TOP_NIL:
            case BC_JUMP_ON_NOT_NIL_POP:
            case BC_JUMP_ON_NIL_POP:
            case BC_JUMP_ON_NOT_NIL_TOP_TOP:
            case BC_JUMP_ON_NIL_TOP_TOP:
            case BC_JUMP_IF_GREATER:
            case BC_JUMP_BACKWARD:
            case BC_JUMP2:
            case BC_JUMP2_ON_FALSE_POP:
            case BC_JUMP2_ON_TRUE_POP:
            case BC_JUMP2_ON_FALSE_TOP_NIL:
            case BC_JUMP2_ON_TRUE_TOP_NIL:
            case BC_JUMP2_ON_NOT_NIL_POP:
            case BC_JUMP2_ON_NIL_POP:
            case BC_JUMP2_ON_NOT_NIL_TOP_TOP:
            case BC_JUMP2_ON_NIL_TOP_TOP:
            case BC_JUMP2_IF_GREATER:
            case BC_JUMP2_BACKWARD: {
                uint16_t const offset =
                    ComputeOffset(bytecodes[bc_idx + 1], bytecodes[bc_idx + 2]);

                int32_t target = 0;
                if (bytecode == BC_JUMP_BACKWARD ||
                    bytecode == BC_JUMP2_BACKWARD) {
                    target = ((int32_t)bc_idx) - offset;
                } else {
                    target = ((int32_t)bc_idx) + offset;
                }
                DebugPrint("(jump offset: %d -> jump target: %d)\n", offset,
                           target);
                break;
            }
            default: {
                DebugPrint("<incorrect bytecode>\n");
            }
        }
    }
    DebugDump("%s)\n", indent);
}

/**
 * Bytecode Index Accessor macros
 */
#define BC_0 method->GetBytecode(bc_idx)
#define BC_1 method->GetBytecode(bc_idx + 1)
#define BC_2 method->GetBytecode(bc_idx + 2)

void Disassembler::printArgument(uint8_t idx, uint8_t ctx, VMClass* cl,
                                 VMFrame* frame) {
    vm_oop_t o = frame->GetArgument(idx, ctx);
    DebugPrint("argument: %d, context: %d", idx, ctx);

    if (cl != nullptr) {
        VMClass* c = CLASS_OF(o);
        VMSymbol* cname = c->GetName();

        DebugPrint("<(%s) ", cname->GetStdString().c_str());
        dispatch(o);
        DebugPrint(">");
    }
    DebugPrint("\n");
}

void Disassembler::printPopLocal(uint8_t idx, uint8_t ctx, VMFrame* frame) {
    vm_oop_t o = frame->GetStackElement(0);
    VMClass* c = CLASS_OF(o);
    VMSymbol* cname = c->GetName();

    DebugPrint("popped local: %d, context: %d <(%s) ", idx, ctx,
               cname->GetStdString().c_str());
    dispatch(o);
    DebugPrint(">\n");
}

void Disassembler::printNth(uint8_t idx, VMFrame* frame, const char* op) {
    vm_oop_t o = frame->GetStackElement(idx);
    if (o != nullptr) {
        VMClass* c = CLASS_OF(o);
        VMSymbol* cname = c->GetName();

        DebugPrint("<to %s: (%s) ", op, cname->GetStdString().c_str());
        dispatch(o);
    } else {
        DebugPrint("<to %s: address: %p", op, (void*)o);
    }
    DebugPrint(">\n");
}

void Disassembler::printConstantAccess(vm_oop_t constant, uint8_t index) {
    VMClass* c = CLASS_OF(constant);
    VMSymbol* cname = c->GetName();

    DebugPrint("(index: %d) value: (%s) ", index,
               cname->GetStdString().c_str());
    dispatch(constant);
    DebugPrint("\n");
}

/**
 * Dump bytecode from the frame running
 */
void Disassembler::DumpBytecode(VMFrame* frame, VMMethod* method,
                                size_t bc_idx) {
    static int64_t indentc = 0;
    static char ikind = '@';
    uint8_t const bc = BC_0;
    VMClass* cl = method->GetHolder();

    // Determine Context: Class or Block?
    if (cl != nullptr) {
        VMSymbol* cname = cl->GetName();
        VMSymbol* sig = method->GetSignature();

        DebugTrace("%20s>>%-20s% 10lld %c %04d: %s\t",
                   cname->GetStdString().c_str(), sig->GetStdString().c_str(),
                   indentc, ikind, bc_idx, Bytecode::GetBytecodeName(bc));
    } else {
        VMSymbol* sig = method->GetSignature();

        DebugTrace("%-42s% 10lld %c %04d: %s\t", sig->GetStdString().c_str(),
                   indentc, ikind, bc_idx, Bytecode::GetBytecodeName(bc));
    }
    // reset send indicator
    if (ikind != '@') {
        ikind = '@';
    }

    switch (bc) {
        case BC_PUSH_0:
        case BC_PUSH_1:
        case BC_PUSH_NIL: {
            DebugPrint("\n");
            break;
        }
        case BC_HALT: {
            DebugPrint("<halting>\n\n\n");
            break;
        }
        case BC_DUP: {
            printNth(0, frame, "dup");
            break;
        }
        case BC_DUP_SECOND: {
            printNth(1, frame, "dup-second");
            break;
        }
        case BC_INC: {
            printNth(0, frame, "inc");
            break;
        }
        case BC_DEC: {
            printNth(0, frame, "dec");
            break;
        }
        case BC_PUSH_LOCAL: {
            uint8_t const bc1 = BC_1;
            uint8_t const bc2 = BC_2;
            vm_oop_t o = frame->GetLocal(bc1, bc2);
            VMClass* c = CLASS_OF(o);
            VMSymbol* cname = c->GetName();

            DebugPrint("local: %d, context: %d <(%s) ", BC_1, BC_2,
                       cname->GetStdString().c_str());
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_PUSH_LOCAL_0:
        case BC_PUSH_LOCAL_1:
        case BC_PUSH_LOCAL_2: {
            uint8_t bc1 = 0;
            uint8_t bc2 = 0;
            switch (bc) {
                case BC_PUSH_LOCAL_0:
                    bc1 = 0;
                    bc2 = 0;
                    break;
                case BC_PUSH_LOCAL_1:
                    bc1 = 1;
                    bc2 = 0;
                    break;
                case BC_PUSH_LOCAL_2:
                    bc1 = 2;
                    bc2 = 0;
                    break;
                default:
                    DebugPrint("Unexpected bytecode");
                    return;
            }

            vm_oop_t o = frame->GetLocal(bc1, bc2);
            VMClass* c = CLASS_OF(o);
            VMSymbol* cname = c->GetName();

            DebugPrint("local: %d, context: %d <(%s) ", BC_1, BC_2,
                       cname->GetStdString().c_str());
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_PUSH_SELF: {
            printArgument(0, 0, cl, frame);
            break;
        }
        case BC_PUSH_ARG_1: {
            printArgument(1, 0, cl, frame);
            break;
        }
        case BC_PUSH_ARG_2: {
            printArgument(2, 0, cl, frame);
            break;
        }
        case BC_PUSH_ARGUMENT: {
            uint8_t const bc1 = BC_1;
            uint8_t const bc2 = BC_2;
            printArgument(bc1, bc2, cl, frame);
            break;
        }
        case BC_PUSH_BLOCK: {
            DebugPrint("block: (index: %d) ", BC_1);
            auto* meth = dynamic_cast<VMMethod*>(
                (AbstractVMObject*)method->GetConstant(bc_idx));
            DumpMethod(meth, "$");
            break;
        }
        case BC_PUSH_CONSTANT: {
            vm_oop_t constant = method->GetConstant(bc_idx);
            printConstantAccess(constant, BC_1);
            break;
        }
        case BC_PUSH_CONSTANT_0: {
            vm_oop_t constant = method->GetIndexableField(0);
            printConstantAccess(constant, 0);
            break;
        }
        case BC_PUSH_CONSTANT_1: {
            vm_oop_t constant = method->GetIndexableField(1);
            printConstantAccess(constant, 1);
            break;
        }
        case BC_PUSH_CONSTANT_2: {
            vm_oop_t constant = method->GetIndexableField(2);
            printConstantAccess(constant, 2);
            break;
        }
        case BC_PUSH_GLOBAL: {
            auto* name = static_cast<VMSymbol*>(method->GetConstant(bc_idx));
            vm_oop_t o = Universe::GetGlobal(name);
            VMSymbol* cname = nullptr;

            const char* c_cname = nullptr;
            std::string c_cname_str;
            if (o != nullptr) {
                VMClass* c = CLASS_OF(o);
                cname = c->GetName();
                c_cname_str = cname->GetStdString();
                c_cname = c_cname_str.c_str();
            } else {
                c_cname = "nullptr";
            }

            DebugPrint("(index: %d)value: %s <(%s) ", BC_1,
                       name->GetStdString().c_str(), c_cname);
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_POP: {
            vm_oop_t o = frame->GetStackElement(0);
            VMClass* c = CLASS_OF(o);
            VMSymbol* cname = c->GetName();

            DebugPrint("popped <(%s) ", cname->GetStdString().c_str());
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_POP_LOCAL: {
            printPopLocal(BC_1, BC_2, frame);
            break;
        }
        case BC_POP_LOCAL_0: {
            printPopLocal(0, 0, frame);
            break;
        }
        case BC_POP_LOCAL_1: {
            printPopLocal(1, 0, frame);
            break;
        }
        case BC_POP_LOCAL_2: {
            printPopLocal(2, 0, frame);
            break;
        }
        case BC_POP_ARGUMENT: {
            vm_oop_t o = frame->GetStackElement(0);
            VMClass* c = CLASS_OF(o);
            VMSymbol* cname = c->GetName();
            DebugPrint("argument: %d, context: %d <(%s) ", BC_1, BC_2,
                       cname->GetStdString().c_str());
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_INC_FIELD:
        case BC_INC_FIELD_PUSH:
        case BC_PUSH_FIELD:
        case BC_POP_FIELD: {
            VMFrame* ctxt = frame->GetOuterContext();
            vm_oop_t arg = ctxt->GetArgumentInCurrentContext(0);
            uint8_t const fieldIndex = BC_1;

            vm_oop_t o = ((VMObject*)arg)->GetField(fieldIndex);
            VMClass* c = CLASS_OF(o);
            VMSymbol* cname = c->GetName();
            uint8_t const fieldIdx = BC_1;
            VMSymbol* name =
                method->GetHolder()->GetInstanceFieldName(fieldIdx);
            DebugPrint("(index: %d) field: %s <(%s) ", BC_1,
                       name->GetStdString().c_str(),
                       cname->GetStdString().c_str());
            dispatch(o);
            DebugPrint(">\n");
            break;
        }
        case BC_SUPER_SEND:
        case BC_SEND:
        case BC_SEND_1: {
            auto* sel = static_cast<VMSymbol*>(method->GetConstant(bc_idx));

            DebugPrint("(index: %d) signature: %s (", BC_1,
                       sel->GetStdString().c_str());
            // handle primitives, they don't increase call-depth
            vm_oop_t elem = frame->GetStackElement(
                Signature::GetNumberOfArguments(sel) - 1);
            VMClass* elemClass = CLASS_OF(elem);
            auto* inv = elemClass->LookupInvokable(sel);

            if (inv != nullptr && inv->IsPrimitive()) {
                DebugPrint("*)\n");
            } else {
                DebugPrint("\n");
                indentc++;
                ikind = '>';  // visual
            }
            break;
        }
        case BC_RETURN_SELF:
        case BC_RETURN_LOCAL:
        case BC_RETURN_NON_LOCAL: {
            DebugPrint(")\n");
            indentc--;
            ikind = '<';  // visual
            break;
        }
        case BC_JUMP:
        case BC_JUMP_ON_FALSE_POP:
        case BC_JUMP_ON_TRUE_POP:
        case BC_JUMP_ON_FALSE_TOP_NIL:
        case BC_JUMP_ON_TRUE_TOP_NIL:
        case BC_JUMP_ON_NOT_NIL_POP:
        case BC_JUMP_ON_NIL_POP:
        case BC_JUMP_ON_NOT_NIL_TOP_TOP:
        case BC_JUMP_ON_NIL_TOP_TOP:
        case BC_JUMP_IF_GREATER:
        case BC_JUMP_BACKWARD:
        case BC_JUMP2:
        case BC_JUMP2_ON_FALSE_POP:
        case BC_JUMP2_ON_TRUE_POP:
        case BC_JUMP2_ON_FALSE_TOP_NIL:
        case BC_JUMP2_ON_TRUE_TOP_NIL:
        case BC_JUMP2_ON_NOT_NIL_POP:
        case BC_JUMP2_ON_NIL_POP:
        case BC_JUMP2_ON_NOT_NIL_TOP_TOP:
        case BC_JUMP2_ON_NIL_TOP_TOP:
        case BC_JUMP2_IF_GREATER:
        case BC_JUMP2_BACKWARD: {
            uint16_t const offset =
                ComputeOffset(method->GetBytecode(bc_idx + 1),
                              method->GetBytecode(bc_idx + 2));

            int32_t target = 0;
            if (bc == BC_JUMP_BACKWARD || bc == BC_JUMP2_BACKWARD) {
                target = ((int32_t)bc_idx) - offset;
            } else {
                target = ((int32_t)bc_idx) + offset;
            }
            DebugPrint("(jump offset: %d -> jump target: %d)", offset, target);
            break;
        }
        default:
            DebugPrint("<incorrect bytecode>\n");
            break;
    }
}
