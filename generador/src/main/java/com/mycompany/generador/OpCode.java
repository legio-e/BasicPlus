/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.mycompany.generador;

/**
 *
 * @author Eduardo
 */
public final class OpCode 
{

    public static final int ICONST = 1;

    public static final int IADD = 2;
    public static final int ISUB = 3;
    public static final int IMUL = 4;
    public static final int IDIV = 5;

    public static final int ILOAD = 6;
    public static final int ISTORE = 7;

    public static final int PRINT_INT = 8;
    public static final int PRINT_NL = 9;

    public static final int RET = 10;

    public static final int LOAD_LOCAL_ADDR = 11;

    public static final int LOAD_I32 = 12;
    public static final int STORE_I32 = 13;

    public static final int ICMPLT = 14;

    public static final int JMP = 15;
    public static final int JZ = 16;
}
