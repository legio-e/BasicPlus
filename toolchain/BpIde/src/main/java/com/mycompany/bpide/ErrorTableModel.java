package com.mycompany.bpide;

import javax.swing.SwingUtilities;
import javax.swing.table.AbstractTableModel;

/**
 * TableModel para una {@link javax.swing.JTable} que muestra los errores
 * de compilación, bindeado a una {@link ObservableList} de
 * {@link CompileError}. Cada mutación de la lista se traduce a un evento
 * de tabla apropiado (insertRow/deleteRow/refreshAll) sobre el EDT.
 *
 * <p>El modelo es de sólo-lectura desde la UI ({@link #isCellEditable}
 * devuelve false); el código que añade/quita errores lo hace contra la
 * {@code ObservableList} subyacente.</p>
 */
public final class ErrorTableModel extends AbstractTableModel
        implements ObservableList.Listener<CompileError> {

    private static final String[] COLUMNS = {
            "Fichero", "Línea", "Columna", "Tipo", "Categoría", "Mensaje"
    };

    private final ObservableList<CompileError> source;

    public ErrorTableModel(ObservableList<CompileError> source) 
    {
        this.source = source;
        source.addListener(this);
    }

    /** Devuelve el error en la fila indicada (útil para handlers de selección). */
    public CompileError getErrorAt(int row) { return source.get(row); }

    // ---- Listener de ObservableList → eventos de TableModel ----

    @Override public void onAdded(int idx, CompileError item) {
        SwingUtilities.invokeLater(() -> fireTableRowsInserted(idx, idx));
    }
    @Override public void onRemoved(int idx, CompileError item) {
        SwingUtilities.invokeLater(() -> fireTableRowsDeleted(idx, idx));
    }
    @Override public void onCleared() {
        SwingUtilities.invokeLater(this::fireTableDataChanged);
    }

    // ---- API de TableModel ----

    @Override public int      getRowCount()        { return source.size(); }
    @Override public int      getColumnCount()     { return COLUMNS.length; }
    @Override public String   getColumnName(int c) { return COLUMNS[c]; }
    @Override public boolean  isCellEditable(int r, int c) { return false; }

    @Override
    public Class<?> getColumnClass(int c) 
    {
        switch (c) {
            case 1: case 2: return Integer.class;
            default:        return String.class;
        }
    }

    @Override
    public Object getValueAt(int row, int col) 
    {
        CompileError e = source.get(row);
        switch (col) {
            case 0: return e.file;
            case 1: return e.line;
            case 2: return e.column;
            case 3: return e.kind;
            case 4: return e.category;
            case 5: return e.message;
            default: return null;
        }
    }
}
