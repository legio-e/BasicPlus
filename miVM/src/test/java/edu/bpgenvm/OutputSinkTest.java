package edu.bpgenvm;

import edu.bpgenvm.vm.OutputSink;
import edu.bpgenvm.vm.StdoutSink;
import edu.bpgenvm.vm.VirtualMachine;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Valida A1.2 — el contrato del OutputSink inyectable.
 *
 * No ejecuta un .mod real (eso lo hacen las demás tests); aquí miramos el
 * contrato del sink propiamente dicho: que la VM lo expone, que el default
 * es StdoutSink, y que un sink customizado captura caracteres exactamente
 * como los recibe. Cuando lleguemos a A1.4 (servidor en la VM), el sink que
 * inyectará el IDE seguirá ESTE contrato.
 */
class OutputSinkTest {

    /** Sink de captura — el IDE hará algo equivalente para mandar al socket. */
    private static final class CaptureSink implements OutputSink {
        final StringBuilder buf = new StringBuilder();
        int flushes = 0;
        @Override public synchronized void writeText(String s) { buf.append(s); }
        @Override public synchronized void writeChar(char c)   { buf.append(c); }
        @Override public synchronized void newline()           { buf.append('\n'); }
        @Override public synchronized void flush()             { flushes++; }
    }

    @Test
    void defaultSinkIsStdout() {
        VirtualMachine vm = new VirtualMachine();
        assertNotNull(vm.getProgramOut(), "el sink no puede ser null");
        assertTrue(vm.getProgramOut() instanceof StdoutSink,
                "default debería ser StdoutSink, fue " + vm.getProgramOut().getClass().getName());
    }

    @Test
    void setNullRestoresStdoutSink() {
        VirtualMachine vm = new VirtualMachine();
        CaptureSink custom = new CaptureSink();
        vm.setProgramOut(custom);
        assertSame(custom, vm.getProgramOut());
        vm.setProgramOut(null);
        assertTrue(vm.getProgramOut() instanceof StdoutSink,
                "set(null) debería volver al default StdoutSink");
    }

    @Test
    void captureSinkRecibeChunksLiteralmente() {
        VirtualMachine vm = new VirtualMachine();
        CaptureSink sink = new CaptureSink();
        vm.setProgramOut(sink);

        // Simulamos las invocaciones que harían los opcodes PRINT_*.
        OutputSink s = vm.getProgramOut();
        s.writeText("hola");
        s.writeChar(' ');
        s.writeText("mundo");
        s.newline();
        s.writeText("linea 2");
        s.flush();

        assertEquals("hola mundo\nlinea 2", sink.buf.toString());
        assertEquals(1, sink.flushes);
    }
}
