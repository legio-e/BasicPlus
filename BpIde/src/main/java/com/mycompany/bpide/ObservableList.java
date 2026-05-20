package com.mycompany.bpide;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Lista observable mínima: cada mutación notifica a sus listeners en orden
 * de subscripción. Pensada para que un componente UI (un TableModel, una
 * lista, etc.) refleje cambios sin acoplarse a la fuente de datos.
 *
 * <p>No es thread-safe: las mutaciones deben hacerse desde un único hilo
 * (típicamente el EDT en una app Swing). Los productores externos
 * (workers) deben envolver con {@code SwingUtilities.invokeLater}.</p>
 */
public final class ObservableList<T> {

    /**
     * Listener notificado en cada mutación. Cualquiera de los tres métodos
     * puede tener un default que no haga nada en implementaciones parciales.
     */
    public interface Listener<T> {
        void onAdded(int index, T item);
        void onRemoved(int index, T item);
        void onCleared();
    }

    private final List<T> items = new ArrayList<>();
    private final List<Listener<T>> listeners = new ArrayList<>();

    // ---- Mutación ----

    public void add(T item) {
        int idx = items.size();
        items.add(item);
        for (Listener<T> l : listeners) l.onAdded(idx, item);
    }

    public T remove(int index) {
        T removed = items.remove(index);
        for (Listener<T> l : listeners) l.onRemoved(index, removed);
        return removed;
    }

    public void clear() {
        if (items.isEmpty()) return;
        items.clear();
        for (Listener<T> l : listeners) l.onCleared();
    }

    // ---- Acceso ----

    public T get(int index)   { return items.get(index); }
    public int size()         { return items.size(); }
    public boolean isEmpty()  { return items.isEmpty(); }
    public List<T> snapshot() { return Collections.unmodifiableList(new ArrayList<>(items)); }

    // ---- Subscripción ----

    public void addListener(Listener<T> listener)    { listeners.add(listener); }
    public void removeListener(Listener<T> listener) { listeners.remove(listener); }
}
