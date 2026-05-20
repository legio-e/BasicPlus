package edu.bpgenvm;

import edu.bpgenvm.vm.debug.DebugController;
import edu.bpgenvm.vm.debug.DebugEvent;
import edu.bpgenvm.vm.debug.PausedEvent;
import edu.bpgenvm.vm.debug.ResumedEvent;
import edu.bpgenvm.vm.debug.StepCommand;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Smoke test del API del DebugController. No incluye el hook real (que
 * requiere una DebugContext live de la VM); eso lo cubre el test e2e
 * a través del IDE.
 */
class DebugControllerTest {

    @Test
    void breakpointsAddRemoveContainsByBasename() {
        DebugController c = new DebugController();
        c.setBreakpoint("C:/path/to/foo.bp", 10, true);
        assertTrue(c.isBreakpointAt("foo.bp", 10),
                "basename match: foo.bp:10 debería estar");
        assertTrue(c.isBreakpointAt("D:/otherDir/foo.bp", 10),
                "basename match: cualquier dir + foo.bp:10 debería estar");
        assertFalse(c.isBreakpointAt("foo.bp", 11),
                "linea distinta no debería estar");
        assertFalse(c.isBreakpointAt("bar.bp", 10),
                "fichero distinto no debería estar");

        c.setBreakpoint("foo.bp", 10, false);
        assertFalse(c.isBreakpointAt("foo.bp", 10),
                "tras quitar, no debería estar");
    }

    @Test
    void listBreakpointsReturnsSortedSnapshot() {
        DebugController c = new DebugController();
        c.setBreakpoint("z.bp", 3, true);
        c.setBreakpoint("a.bp", 50, true);
        c.setBreakpoint("a.bp", 10, true);

        List<String> snap = c.listBreakpoints();
        // Lexicográfico: "a.bp:10" < "a.bp:50" < "z.bp:3" (en string).
        assertEquals(3, snap.size());
        assertEquals("a.bp:10", snap.get(0));
        assertEquals("a.bp:50", snap.get(1));
        assertEquals("z.bp:3",  snap.get(2));
    }

    @Test
    void clearAllBreakpointsLeavesEmpty() {
        DebugController c = new DebugController();
        c.setBreakpoint("a.bp", 1, true);
        c.setBreakpoint("a.bp", 2, true);
        c.clearAllBreakpoints();
        assertTrue(c.listBreakpoints().isEmpty());
    }

    @Test
    void sendCommandWhileNotPausedIsNoOp() {
        DebugController c = new DebugController();
        // No está pausado (ningún hook ha corrido) → debe ser silencioso.
        c.sendCommand(StepCommand.CONTINUE);
        c.sendCommand(StepCommand.STEP_INTO);
        // Si esto bloqueara colgaría el test; el contrato es que retorna inmediato.
        assertFalse(c.isPaused());
    }

    @Test
    void listenerRecibeEventosEmitidosManualmente() throws Exception {
        // Forzamos a emit a través de la reflexión: probamos la canalización
        // de listeners sin necesidad de un hook completo.
        DebugController c = new DebugController();
        List<DebugEvent> received = new ArrayList<>();
        c.addListener(received::add);

        // emit es private; lo invocamos con reflection para validar el
        // contrato "los listeners reciben todo en orden".
        java.lang.reflect.Method emit =
                DebugController.class.getDeclaredMethod("emit", DebugEvent.class);
        emit.setAccessible(true);
        emit.invoke(c, new PausedEvent(0, 100, 5, "x.bp", 100, 200, 300, 50));
        emit.invoke(c, new ResumedEvent(0));

        assertEquals(2, received.size());
        assertTrue(received.get(0) instanceof PausedEvent);
        assertTrue(received.get(1) instanceof ResumedEvent);
        PausedEvent pe = (PausedEvent) received.get(0);
        assertEquals(5, pe.line);
        assertEquals("x.bp", pe.sourceFile);
    }
}
