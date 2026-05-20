/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.mycompany.generador;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 *
 * @author Eduardo
 */
public class GenCode 
{

    private Module currentModule;

    private Function currentFunction;

    public void beginModule(String name) {
        currentModule = new Module(name);
    }

    public Function beginFunction(
            String name,
            Type returnType) {

        currentFunction = currentModule.addFunction(
                name,
                returnType
        );

        return currentFunction;
    }

    public void endFunction() {
        currentFunction.finalizeFunction();
        currentFunction = null;
    }

    public byte[] build() {
        return currentModule.build();
    }

    public Label createLabel() {
        return currentFunction.code.createLabel();
    }

    public void markLabel(Label label) {
        currentFunction.code.markLabel(label);
    }

    public void iconst(int value) {
        currentFunction.code.emitByte(OpCode.ICONST);
        currentFunction.code.emitInt(value);
    }

    public void iadd() {
        currentFunction.code.emitByte(OpCode.IADD);
    }

    public void isub() {
        currentFunction.code.emitByte(OpCode.ISUB);
    }

    public void imul() {
        currentFunction.code.emitByte(OpCode.IMUL);
    }

    public void idiv() {
        currentFunction.code.emitByte(OpCode.IDIV);
    }

    public void load(Variable v) {
        currentFunction.code.emitByte(OpCode.ILOAD);
        currentFunction.code.emitInt(v.offset);
    }

    public void store(Variable v) {
        currentFunction.code.emitByte(OpCode.ISTORE);
        currentFunction.code.emitInt(v.offset);
    }

    public void loadLocalAddress(Variable v) {
        currentFunction.code.emitByte(OpCode.LOAD_LOCAL_ADDR);
        currentFunction.code.emitInt(v.offset);
    }

    public void loadIndirectI32() {
        currentFunction.code.emitByte(OpCode.LOAD_I32);
    }

    public void storeIndirectI32() {
        currentFunction.code.emitByte(OpCode.STORE_I32);
    }

    public void printInt() {
        currentFunction.code.emitByte(OpCode.PRINT_INT);
    }

    public void printNL() {
        currentFunction.code.emitByte(OpCode.PRINT_NL);
    }

    public void icmplt() {
        currentFunction.code.emitByte(OpCode.ICMPLT);
    }

    public void jmp(Label label) {
        currentFunction.code.emitJump(OpCode.JMP, label);
    }

    public void jz(Label label) {
        currentFunction.code.emitJump(OpCode.JZ, label);
    }

    public void retVoid() {
        currentFunction.code.emitByte(OpCode.RET);
    }
}

class Module {

    private final String name;

    private final List<Function> functions =
            new ArrayList<>();

    public Module(String name) {
        this.name = name;
    }

    public Function addFunction(
            String name,
            Type returnType) {

        Function fn = new Function(name, returnType);

        functions.add(fn);

        return fn;
    }

    public byte[] build() {

        ByteArrayOutputStream out =
                new ByteArrayOutputStream();

        try {

            for(Function fn : functions) {

                byte[] code = fn.code.toByteArray();

                out.write(code);
            }

        } catch(Exception ex) {
            throw new RuntimeException(ex);
        }

        return out.toByteArray();
    }
}

class Function {

    public final String name;

    public final Type returnType;

    public final CodeBuffer code =
            new CodeBuffer();

    private int nextLocalOffset = 0;

    public Function(
            String name,
            Type returnType) {

        this.name = name;
        this.returnType = returnType;
    }

    public Variable addLocal(
            String name,
            Type type) {

        Variable v = new Variable(
                name,
                type,
                nextLocalOffset
        );

        nextLocalOffset += type.stackSize;

        return v;
    }

    public void finalizeFunction() {
        code.patchLabels();
    }
}

class Variable {

    public final String name;

    public final Type type;

    public final int offset;

    public Variable(
            String name,
            Type type,
            int offset) {

        this.name = name;
        this.type = type;
        this.offset = offset;
    }
}

class Type {

    public final String name;

    public final int size;

    public final int stackSize;

    public Type(
            String name,
            int size,
            int stackSize) {

        this.name = name;
        this.size = size;
        this.stackSize = stackSize;
    }

    public static final Type VOID =
            new Type("void", 0, 0);

    public static final Type I32 =
            new Type("i32", 4, 4);

    public static final Type I8 =
            new Type("i8", 1, 4);

    public static final Type I16 =
            new Type("i16", 2, 4);

    public static final Type F32 =
            new Type("f32", 4, 4);

    public static final Type F64 =
            new Type("f64", 8, 8);

    public static final Type BOOL =
            new Type("bool", 4, 4);
}

class StaticArrayType extends Type {

    public final Type elementType;

    public final int length;

    public StaticArrayType(
            Type elementType,
            int length) {

        super(
                elementType.name + "[]",
                elementType.size * length,
                align(
                        elementType.stackSize * length,
                        4
                )
        );

        this.elementType = elementType;
        this.length = length;
    }

    private static int align(int value, int align) {
        return ((value + align - 1) / align) * align;
    }
}
