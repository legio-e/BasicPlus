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
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;


public class Main 
{
    public static void main(String[] args) {

        GenCode gen = new GenCode();

        gen.beginModule("test");

        // function main()
        Function main = gen.beginFunction(
                "main",
                Type.VOID
        );

        // local int x
        Variable x = main.addLocal("x", Type.I32);

        // local int[4] arr
        Variable arr = main.addLocal(
                "arr",
                new StaticArrayType(Type.I32, 4)
        );

        // x = 10
        gen.iconst(10);
        gen.store(x);

        // print x
        gen.load(x);
        gen.printInt();
        gen.printNL();

        // arr[0] = 100
        gen.loadLocalAddress(arr);
        gen.iconst(0);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.iconst(100);
        gen.storeIndirectI32();

        // arr[1] = 200
        gen.loadLocalAddress(arr);
        gen.iconst(1);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.iconst(200);
        gen.storeIndirectI32();

        System.out.println("Static array values:");

        // print arr[0]
        gen.loadLocalAddress(arr);
        gen.iconst(0);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.loadIndirectI32();
        gen.printInt();
        gen.printNL();

        // print arr[1]
        gen.loadLocalAddress(arr);
        gen.iconst(1);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.loadIndirectI32();
        gen.printInt();
        gen.printNL();

        System.out.println("If statement:");

        // if (x < 20)
        Label elseLabel = gen.createLabel();
        Label endIf = gen.createLabel();

        gen.load(x);
        gen.iconst(20);
        gen.icmplt();

        gen.jz(elseLabel);

        gen.iconst(111);
        gen.printInt();
        gen.printNL();

        gen.jmp(endIf);

        gen.markLabel(elseLabel);

        gen.iconst(222);
        gen.printInt();
        gen.printNL();

        gen.markLabel(endIf);

        System.out.println("While loop:");

        // while(x < 15)
        Label whileStart = gen.createLabel();
        Label whileEnd = gen.createLabel();

        gen.markLabel(whileStart);

        gen.load(x);
        gen.iconst(15);
        gen.icmplt();

        gen.jz(whileEnd);

        gen.load(x);
        gen.printInt();
        gen.printNL();

        // x = x + 1
        gen.load(x);
        gen.iconst(1);
        gen.iadd();
        gen.store(x);

        gen.jmp(whileStart);

        gen.markLabel(whileEnd);

        gen.retVoid();

        gen.endFunction();

        byte[] program = gen.build();

        System.out.println("=== BYTECODE ===");

        for(byte b : program) {
            System.out.printf("%02X ", b);
        }

        System.out.println();
        System.out.println("=== VM OUTPUT ===");

        System.out.println("Variable x:");

        VirtualMachine vm = new VirtualMachine(program);

        vm.run();
    }

}

/*
    public static void main(String[] args) 
    {

        GenCode gen = new GenCode();

        gen.beginModule("test");

        // function main()
        Function main = gen.beginFunction(
                "main",
                Type.VOID
        );

        // local int x
        Variable x = main.addLocal("x", Type.I32);

        // local int[4] arr
        Variable arr = main.addLocal(
                "arr",
                new StaticArrayType(Type.I32, 4)
        );

        // x = 10
        gen.iconst(10);
        gen.store(x);

        // print x
        gen.load(x);
        gen.printInt();
        gen.printNL();

        // arr[0] = 100
        gen.loadLocalAddress(arr);
        gen.iconst(0);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.iconst(100);
        gen.storeIndirectI32();

        // arr[1] = 200
        gen.loadLocalAddress(arr);
        gen.iconst(1);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.iconst(200);
        gen.storeIndirectI32();

        // print arr[0]
        gen.loadLocalAddress(arr);
        gen.iconst(0);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.loadIndirectI32();
        gen.printInt();
        gen.printNL();

        // print arr[1]
        gen.loadLocalAddress(arr);
        gen.iconst(1);
        gen.iconst(4);
        gen.imul();
        gen.iadd();
        gen.loadIndirectI32();
        gen.printInt();
        gen.printNL();

        // if (x < 20)
        Label elseLabel = gen.createLabel();
        Label endIf = gen.createLabel();

        gen.load(x);
        gen.iconst(20);
        gen.icmplt();

        gen.jz(elseLabel);

        gen.iconst(111);
        gen.printInt();
        gen.printNL();

        gen.jmp(endIf);

        gen.markLabel(elseLabel);

        gen.iconst(222);
        gen.printInt();
        gen.printNL();

        gen.markLabel(endIf);

        // while(x < 15)
        Label whileStart = gen.createLabel();
        Label whileEnd = gen.createLabel();

        gen.markLabel(whileStart);

        gen.load(x);
        gen.iconst(15);
        gen.icmplt();

        gen.jz(whileEnd);

        gen.load(x);
        gen.printInt();
        gen.printNL();

        // x = x + 1
        gen.load(x);
        gen.iconst(1);
        gen.iadd();
        gen.store(x);

        gen.jmp(whileStart);

        gen.markLabel(whileEnd);

        gen.retVoid();

        gen.endFunction();

        byte[] program = gen.build();

        System.out.println("=== BYTECODE ===");

        for(byte b : program) {
            System.out.printf("%02X ", b);
        }

        System.out.println();
        System.out.println("=== VM OUTPUT ===");

        VirtualMachine vm = new VirtualMachine(program);

        vm.run();
    }
    
*    
}
*/

