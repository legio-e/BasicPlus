/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.mycompany.generador;

import java.util.ArrayList;
import java.util.List;

/**
 *
 * @author Eduardo
 */

class Label {
    int position = -1;
}

class JumpPatch {

    public final int patchPosition;

    public final Label target;

    public JumpPatch(
            int patchPosition,
            Label target) {

        this.patchPosition = patchPosition;
        this.target = target;
    }
}

class CodeBuffer {

    private final ArrayList<Byte> data =
            new ArrayList<>();

    private final List<JumpPatch> jumps =
            new ArrayList<>();

    public int position() {
        return data.size();
    }

    public void emitByte(int value) {
        data.add((byte)value);
    }

    public void emitInt(int value) {

        emitByte(value);
        emitByte(value >> 8);
        emitByte(value >> 16);
        emitByte(value >> 24);
    }

    public void patchInt(int pos, int value) {

        data.set(pos, (byte)value);
        data.set(pos + 1, (byte)(value >> 8));
        data.set(pos + 2, (byte)(value >> 16));
        data.set(pos + 3, (byte)(value >> 24));
    }

    public Label createLabel() {
        return new Label();
    }

    public void markLabel(Label label) {
        label.position = position();
    }

    public void emitJump(
            int opcode,
            Label label) {

        emitByte(opcode);

        int patchPos = position();

        emitInt(0);

        jumps.add(new JumpPatch(
                patchPos,
                label
        ));
    }

    public void patchLabels() {

        for(JumpPatch j : jumps) {

            patchInt(
                    j.patchPosition,
                    j.target.position
            );
        }
    }

    public byte[] toByteArray() {

        byte[] result =
                new byte[data.size()];

        for(int i = 0; i < data.size(); i++) {
            result[i] = data.get(i);
        }

        return result;
    }
}

class VirtualMachine {

    private final byte[] code;

    private final byte[] memory =
            new byte[1024 * 1024];

    private int pc = 0;

    private int bp = 0;

    private int sp = 0;

    public VirtualMachine(byte[] code) {
        this.code = code;
    }

    public void run() {

        while(true) {

            int op = fetchByte();

            switch(op) {

                case OpCode.ICONST: {

                    pushInt(fetchInt());

                    break;
                }

                case OpCode.IADD: {

                    int b = popInt();
                    int a = popInt();

                    pushInt(a + b);

                    break;
                }

                case OpCode.ISUB: {

                    int b = popInt();
                    int a = popInt();

                    pushInt(a - b);

                    break;
                }

                case OpCode.IMUL: {

                    int b = popInt();
                    int a = popInt();

                    pushInt(a * b);

                    break;
                }

                case OpCode.IDIV: {

                    int b = popInt();
                    int a = popInt();

                    pushInt(a / b);

                    break;
                }

                case OpCode.ILOAD: {

                    int offset = fetchInt();

                    pushInt(
                            readInt(bp + offset)
                    );

                    break;
                }

                case OpCode.ISTORE: {

                    int offset = fetchInt();

                    int value = popInt();

                    writeInt(bp + offset, value);

                    break;
                }

                case OpCode.LOAD_LOCAL_ADDR: {

                    int offset = fetchInt();

                    pushInt(bp + offset);

                    break;
                }

                case OpCode.LOAD_I32: {

                    int addr = popInt();

                    pushInt(readInt(addr));

                    break;
                }

                case OpCode.STORE_I32: {

                    int value = popInt();
                    int addr = popInt();

                    writeInt(addr, value);

                    break;
                }

                case OpCode.PRINT_INT: {

                    System.out.print(popInt());

                    break;
                }

                case OpCode.PRINT_NL: {

                    System.out.println();

                    break;
                }

                case OpCode.ICMPLT: {

                    int b = popInt();
                    int a = popInt();

                    pushInt(a < b ? 1 : 0);

                    break;
                }

                case OpCode.JMP: {

                    pc = fetchInt();

                    break;
                }

                case OpCode.JZ: {

                    int addr = fetchInt();

                    int value = popInt();

                    if(value == 0) {
                        pc = addr;
                    }

                    break;
                }

                case OpCode.RET: {
                    return;
                }

                default:
                    throw new RuntimeException(
                            "Invalid opcode: " + op
                    );
            }
        }
    }

    private int fetchByte() {
        return code[pc++] & 0xFF;
    }

    private int fetchInt() {

        int value =
                (code[pc] & 0xFF) |
                ((code[pc + 1] & 0xFF) << 8) |
                ((code[pc + 2] & 0xFF) << 16) |
                ((code[pc + 3] & 0xFF) << 24);

        pc += 4;

        return value;
    }

    private void pushInt(int value) {

        writeInt(sp, value);

        sp += 4;
    }

    private int popInt() {

        sp -= 4;

        return readInt(sp);
    }

    private void writeInt(int addr, int value) {

        memory[addr] = (byte)value;
        memory[addr + 1] = (byte)(value >> 8);
        memory[addr + 2] = (byte)(value >> 16);
        memory[addr + 3] = (byte)(value >> 24);
    }

    private int readInt(int addr) {

        return
                (memory[addr] & 0xFF) |
                ((memory[addr + 1] & 0xFF) << 8) |
                ((memory[addr + 2] & 0xFF) << 16) |
                ((memory[addr + 3] & 0xFF) << 24);
    }
}
