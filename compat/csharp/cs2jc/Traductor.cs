// El traductor de C# a JC (V8/H6, docs/V8_IDEAS.md §E9). Recorre el árbol de Roslyn una vez, y cada nodo se copia, se
// reescribe o no se traduce (Rechazos.cs). Lo que no conoce es «no se traduce», con su nombre: nada se salta en
// silencio. Las dos reglas de fondo: se reescribe sólo lo que JC escribe de otra forma; y una reescritura no cambia lo
// que se calcula ni su orden.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Operations;

namespace Cs2Jc
{
    sealed class Traduccion
    {
        public string Modulo;
        public string Jc;                                  // null si algo no se traduce
        public readonly List<Rechazo> Rechazos = new List<Rechazo>();
        public readonly List<string> Avisos = new List<string>();
        public bool MainDevuelveEntero;
    }

    sealed partial class Traductor
    {
        /// Las palabras reservadas de JC (JcLexer.RESERVADAS): un nombre de C# que lo sea lleva un `_` detrás.
        static readonly HashSet<string> ReservadasJc = new HashSet<string>(
            ("module library interface implements extends import from const function class enum public private " +
             "protected final native sync owner event static void var new is base field if else while do for foreach " +
             "in break continue return switch case default parallel try catch finally throw raise print println " +
             "inline this super true false null int long float double bool string byte sbyte short ushort").Split(' '));

        /// Las clases de Core, que JC no deja redeclarar («es una clase del lenguaje (Core)»): una de la prueba que se
        /// llame así lleva un `_` detrás, como un nombre reservado.
        static readonly HashSet<string> ClasesDeCore = new HashSet<string>(
            "Object Thread Mutex StringBuilder Exception RuntimeError EncodingError Comparable Integer Long Double Float Boolean List"
                .Split(' '));

        /// Lo que se pone en el sitio de lo que no se traduce, para seguir recorriendo y recogerlo todo.
        const string Hueco = "/*?*/";

        readonly SemanticModel m;
        readonly SyntaxTree arbol;
        readonly IMethodSymbol main;
        readonly Traduccion t = new Traduccion();
        readonly StringBuilder sb = new StringBuilder();
        int sangria;
        INamedTypeSymbol claseActual;
        readonly Stack<string> catchs = new Stack<string>();     // la excepción de cada catch abierto, para `throw;`
        int nombresNuevos;
        readonly Dictionary<ISymbol, string> nombresDeTipo = new Dictionary<ISymbol, string>(SymbolEqualityComparer.Default);
        /// Los tipos de fuera (no anidados) que llevan su namespace delante, porque su nombre chocaría con otro.
        readonly HashSet<ISymbol> conEspacio = new HashSet<ISymbol>(SymbolEqualityComparer.Default);
        /// Lo privado o protegido que usa una clase anidada (Traduce): en JC, public.
        readonly HashSet<ISymbol> abiertos = new HashSet<ISymbol>(SymbolEqualityComparer.Default);

        /// La visibilidad de un miembro en JC: la suya, o public si lo usa una anidada (abiertos).
        string AccesoDe(ISymbol s) => abiertos.Contains(s.OriginalDefinition) ? "public" : Acceso(s.DeclaredAccessibility);

        /// #760 — en JC, lo protegido lo usa una hija sobre `this` o sobre un objeto de su clase o de una hija suya (la regla
        /// de Java); sobre uno del padre, no. Lo que C# deja ahí es por ser una anidada (test-241), y se abre.
        bool SobreUnoSuyo(SyntaxNode nombre, INamedTypeSymbol desde)
        {
            if (!(nombre.Parent is MemberAccessExpressionSyntax ma && ma.Name == nombre)) return true;   // sin receptor: this
            if (ma.Expression is ThisExpressionSyntax || ma.Expression is BaseExpressionSyntax) return true;
            return m.GetTypeInfo(ma.Expression).Type is INamedTypeSymbol t && Hereda(t.OriginalDefinition, desde.OriginalDefinition);
        }
        readonly HashSet<string> renombrados = new HashSet<string>();
        readonly HashSet<string> imports = new HashSet<string>();

        public Traductor(CSharpCompilation comp, SyntaxTree arbol)
        {
            this.arbol = arbol;
            m = comp.GetSemanticModel(arbol);
            main = comp.GetEntryPoint(default);
        }

        // ------------------------------------------------------------------------------------------------ el módulo

        public Traduccion Traduce(string modulo)
        {
            var raiz = (CompilationUnitSyntax) arbol.GetRoot();
            foreach (var g in raiz.Members.OfType<GlobalStatementSyntax>())
                Rechaza(g, Porque.Pendiente("instrucciones fuera de una clase"));
            foreach (var d in raiz.DescendantNodes().OfType<DelegateDeclarationSyntax>())
                Rechaza(d, Porque.H7(Porque.Delegados));
            foreach (var u in raiz.DescendantNodes().OfType<ExternAliasDirectiveSyntax>())
                Rechaza(u, Porque.Fuera(Porque.Interop));

            // El «Color Color» de C# (H8, tema 1): un tipo que se llama como una variable, un campo, una propiedad o un
            // parámetro del programa lleva un `_` detrás (NombreTipo); en JC, como en Java, la variable taparía al tipo.
            foreach (var n in raiz.DescendantNodes())
            {
                if (n is VariableDeclaratorSyntax v) nombresDeValor.Add(v.Identifier.ValueText);
                else if (n is PropertyDeclarationSyntax p) nombresDeValor.Add(p.Identifier.ValueText);
                else if (n is ParameterSyntax ps) nombresDeValor.Add(ps.Identifier.ValueText);
            }

            var tipos = new List<INamedTypeSymbol>();
            foreach (var d in raiz.DescendantNodes().OfType<BaseTypeDeclarationSyntax>())
                if (m.GetDeclaredSymbol(d) is INamedTypeSymbol s && !tipos.Contains(s, SymbolEqualityComparer.Default))
                    tipos.Add(s);
            // Los nombres: las anidadas, con el de su clase delante (A.B es A_B); y dos que se llamarían igual, con el
            // namespace delante (el escalón 1; Eduardo, 1-nov: «los nombres que chocan se antepone el namespace»): N1.X y
            // N2.X son N1_X y N2_X, y sus anidadas, N1_X_B. Si aún chocan (C y C<T> en el mismo namespace), no se traducen.
            var chocan = tipos.GroupBy(s => NombreTipo(s)).Where(g => g.Count() > 1).SelectMany(g => g).ToList();
            if (chocan.Count > 0)
            {
                foreach (var s in chocan)
                {
                    var fuera = s.OriginalDefinition;
                    while (fuera.ContainingType != null) fuera = fuera.ContainingType.OriginalDefinition;
                    if (!fuera.ContainingNamespace.IsGlobalNamespace) conEspacio.Add(fuera);
                }
                nombresDeTipo.Clear();          // se recalculan, con el namespace delante de los que chocan
            }
            // Lo privado (o protegido) de una clase que usa otra anidada en ella: en JC las anidadas son clases aparte (A_B),
            // y lo verían desde fuera; se escribe public (el escalón 1). Sólo cambia quién puede verlo, no lo que se calcula.
            foreach (var nodo in raiz.DescendantNodes().Where(x => x is SimpleNameSyntax || x is ObjectCreationExpressionSyntax))
            {
                var s = m.GetSymbolInfo(nodo).Symbol;
                if (s == null || s.ContainingType == null || s is INamedTypeSymbol || s is ILocalSymbol || s is IParameterSymbol
                    || !DelPrograma(s)) continue;
                var acc = s.DeclaredAccessibility;
                if (acc != Accessibility.Private && acc != Accessibility.Protected && acc != Accessibility.ProtectedAndInternal) continue;
                var tipoDelNodo = nodo.Ancestors().OfType<BaseTypeDeclarationSyntax>().FirstOrDefault();
                if (!(tipoDelNodo != null && m.GetDeclaredSymbol(tipoDelNodo) is INamedTypeSymbol desde)) continue;
                var dueno = s.ContainingType.OriginalDefinition;
                if (SymbolEqualityComparer.Default.Equals(desde.OriginalDefinition, dueno)) continue;
                if (acc != Accessibility.Private && Hereda(desde.OriginalDefinition, dueno) && SobreUnoSuyo(nodo, desde))
                    continue;   // una hija lo ve en JC, sobre un objeto suyo (#760)
                abiertos.Add(s.OriginalDefinition);
            }
            var usados = new Dictionary<string, INamedTypeSymbol>();
            foreach (var s in tipos)
            {
                string n = NombreTipo(s);
                if (usados.ContainsKey(n)) Rechaza(Declaracion(s), Porque.Pendiente("dos clases se llamarían " + n));
                else usados[n] = s;
            }
            while (usados.ContainsKey(modulo)) modulo += "M";
            t.Modulo = modulo;
            BuscaChoques(tipos);                 // las implementaciones explícitas que chocan (Explicitas.cs)
            tiposDelPrograma = tipos;            // el tipo de un objeto (Tipos.cs)

            // JC quiere la clase padre declarada antes que la hija («padre local … no es una clase abierta declarada
            // antes»); a C# le da igual el orden: cada clase, detrás de su base (y si no, en el orden de C#).
            var ordenados = new List<INamedTypeSymbol>();
            var puestos = new HashSet<INamedTypeSymbol>(SymbolEqualityComparer.Default);
            void Pon(INamedTypeSymbol s)
            {
                if (!puestos.Add(s)) return;
                var b = s.BaseType?.OriginalDefinition;
                if (b != null && tipos.Contains(b, SymbolEqualityComparer.Default)) Pon(b);
                foreach (var i in s.Interfaces)          // la interfaz simple, que en JC es su clase base
                    if (tipos.Contains(i.OriginalDefinition, SymbolEqualityComparer.Default)) Pon(i.OriginalDefinition);
                ordenados.Add(s);
            }
            foreach (var s in tipos) Pon(s);

            foreach (var s in ordenados) EmiteTipo(s);
            EmiteMain();
            if (main == null) Rechaza(raiz, Porque.Pendiente("no hay Main"));
            if (usaTipoDe) EmiteTipoDe();

            if (t.Rechazos.Count == 0)
            {
                var jc = new StringBuilder();
                jc.Append("module ").Append(modulo).Append('\n').Append("{\n");
                foreach (var i in imports.OrderBy(x => x, StringComparer.Ordinal)) jc.Append("   import ").Append(i).Append(";\n");
                if (imports.Count > 0) jc.Append('\n');
                jc.Append(sb).Append("}\n");
                t.Jc = jc.ToString();
            }
            var ordenadosPorLinea = t.Rechazos.OrderBy(r => r.Linea).ToList();      // estable: en una línea, el de C#
            t.Rechazos.Clear();
            t.Rechazos.AddRange(ordenadosPorLinea);
            return t;
        }

        void EmiteTipo(INamedTypeSymbol s)
        {
            var donde = Declaracion(s);
            if (s.IsRecord) { Rechaza(donde, Porque.Pendiente("record")); return; }
            switch (s.TypeKind)
            {
                case TypeKind.Class: EmiteClase(s); break;
                case TypeKind.Interface: EmiteInterfaz(s); break;   // H7.3 (#756): la interfaz de JC
                case TypeKind.Struct:
                    if (EsStructSinEstado(s)) EmiteClase(s);
                    else Rechaza(donde, Porque.H7(Porque.Struct));
                    break;
                case TypeKind.Enum: EmiteEnum(s); break;
                default: Rechaza(donde, Porque.Pendiente("tipo " + s.TypeKind)); break;
            }
        }

        /// H8, tema 2 — un `struct` sin estado: sin campos de instancia, ni propiedades automáticas o eventos, que los
        /// llevan. Se escribe como una clase: sin estado, copiarlo no se nota, y un objeto vale tanto como otro. Lo que sí
        /// hay que escribir es su valor por defecto, que en C# existe y en una clase sería null.
        bool EsStructSinEstado(ITypeSymbol t)
        {
            if (!(t is INamedTypeSymbol s) || s.TypeKind != TypeKind.Struct || !DelPrograma(s) || s.IsGenericType || s.IsRecord) return false;
            foreach (var x in s.GetMembers())
            {
                if (x.IsStatic) continue;
                if (x is IFieldSymbol || x is IEventSymbol) return false;
                if (x is IPropertySymbol && x.DeclaringSyntaxReferences.Select(r => r.GetSyntax()).OfType<PropertyDeclarationSyntax>()
                        .Any(d => d.AccessorList != null && d.AccessorList.Accessors.All(a => a.Body == null && a.ExpressionBody == null)))
                    return false;
            }
            return true;
        }

        /// Un enum de C#, el de JC: los valores, los que calcula C#, todos escritos (JC sólo admite un número sin signo;
        /// C#, cualquier constante). El tipo de debajo (`: byte`) no se escribe: sólo lo nota quien lo convierte, y
        /// convertir un enum no se traduce (#522).
        void EmiteEnum(INamedTypeSymbol s)
        {
            var donde = (EnumDeclarationSyntax) Declaracion(s);
            Atributos(donde.AttributeLists);
            var miembros = new List<string>();
            foreach (var v in donde.Members)
            {
                var f = m.GetDeclaredSymbol(v);
                // un `enum : ulong` puede pasarse de un long (test-40)
                long valor = f.ConstantValue is ulong grande && grande > long.MaxValue ? -1 : Convert.ToInt64(f.ConstantValue);
                if (valor < 0 || valor > int.MaxValue) Rechaza(v, Porque.H7(Porque.EnumNegativo));
                miembros.Add(Ident(f.Name) + " = " + valor);
            }
            Linea((s.DeclaredAccessibility == Accessibility.Public ? "public " : "") + "enum " + NombreTipo(s));
            Linea("{");
            sangria++;
            for (int i = 0; i < miembros.Count; i++) Linea(miembros[i] + (i < miembros.Count - 1 ? "," : ""));
            sangria--;
            Linea("}");
            Linea("");
        }

        /// El `main` de JC: llama al `Main` de la prueba y, si devuelve otra cosa que 0, lanza «Main devuelve N»; así la
        /// traducción acaba con 0 si la prueba pasa y con 1 si no (#481), y el mensaje dice por qué.
        void EmiteMain()
        {
            if (main == null) return;
            var donde = main.DeclaringSyntaxReferences.First().GetSyntax();
            // `Main(string[] args)`: sin argumentos, como cuando la prueba se ejecuta sola
            string llamada = NombreTipo(main.ContainingType) + "." + Ident(main.Name) + "(" + (main.Parameters.Length == 1 ? "argumentos" : "") + ")";
            Linea("public function void main()");
            Linea("{");
            sangria++;
            if (main.Parameters.Length == 1) Linea("string[] argumentos = [];");
            if (main.ReturnType.SpecialType == SpecialType.System_Int32)
            {
                t.MainDevuelveEntero = true;
                Linea("int r = " + llamada + ";");
                Linea("if (r != 0)");
                Linea("{");
                Linea("   throw new RuntimeError(\"Main devuelve \" + r);");
                Linea("}");
            }
            else if (main.ReturnsVoid) Linea(llamada + ";");
            else Rechaza(donde, Porque.Fuera(Porque.Async));
            sangria--;
            Linea("}");
        }

        // ------------------------------------------------------------------------------------------------ las clases

        void EmiteClase(INamedTypeSymbol s)
        {
            claseActual = s;
            var partes = s.DeclaringSyntaxReferences.Select(r => (TypeDeclarationSyntax) r.GetSyntax())
                .OrderBy(p => p.SpanStart).ToList();
            var donde = partes[0];
            if (s.IsGenericType) { Rechaza(donde.TypeParameterList ?? (SyntaxNode) donde, Porque.H7(Porque.Genericos)); return; }
            if (partes.Any(p => p.ParameterList != null))
                Rechaza(partes.First(p => p.ParameterList != null).ParameterList, Porque.Pendiente("constructor primario (C# 12)"));
            foreach (var p in partes) Atributos(p.AttributeLists);
            string base_ = null;
            var b = s.BaseType;
            var lugarBase = donde.BaseList ?? (SyntaxNode) donde;
            if (b != null && b.SpecialType != SpecialType.System_Object && b.SpecialType != SpecialType.System_ValueType)
            {
                if (DelPrograma(b)) base_ = Tipo(b, donde);
                else if (ExcepcionJc(b) is string ex) base_ = ex;     // una excepción de .NET (CsSystem, el escalón 1)
                // una clase de atributo (H8, tema 1; 2-nov): en BP, una clase más, sin padre; sus usos, [X(…)], se quitan
                else if (NombreCompleto(b) == "System.Attribute") { }
                else Rechaza(lugarBase, Porque.DeNet("hereda de " + NombreCompleto(b)));
            }
            // Las interfaces (H7.3, #756; antes, con #745, una sola y como clase base): las del programa y las de CsSystem
            // (IDisposable), en la cabecera detrás del padre, como en C# (`class C : B, I, J`); las demás de .NET, la biblioteca.
            var supertipos = new List<string>();
            if (base_ != null) supertipos.Add(base_);
            foreach (var i in s.Interfaces)
            {
                if (DelPrograma(i) || EsDeCsSystem(i)) supertipos.Add(Tipo(i, lugarBase));
                else Rechaza(lugarBase, PorInterfazAjena(i));
            }

            Linea((s.DeclaredAccessibility == Accessibility.Public ? "public " : "") + "class " + NombreTipo(s)
                  + (supertipos.Count > 0 ? " : " + string.Join(", ", supertipos) : ""));
            Linea("{");
            sangria++;
            foreach (var p in partes)
                foreach (var miembro in p.Members)
                    EmiteMiembro(miembro);
            // Una excepción sin constructor: el implícito de C# llama a Exception() o a SystemException(), que en BP no
            // existen (la Exception y el RuntimeError de Core piden su mensaje); se escribe con el mensaje que pondría
            // .NET. Las de la librería tienen el suyo sin argumentos, y lo llama el implícito de JC.
            if (MensajeSinArgumentos(b, s) is string porDefecto && s.InstanceConstructors.All(c => c.IsImplicitlyDeclared))
                Linea("public function " + NombreTipo(s) + "() : base(" + Cadena(porDefecto) + ") { }");
            // un struct siempre se puede crear sin argumentos; una clase de JC con un constructor, no: se escribe
            if (s.TypeKind == TypeKind.Struct && s.InstanceConstructors.Any(c => !c.IsImplicitlyDeclared)
                && s.InstanceConstructors.Any(c => c.IsImplicitlyDeclared && c.Parameters.Length == 0))
                Linea("public function " + NombreTipo(s) + "() { }");
            Reenvia(s);                          // lo que implementa con otro nombre (Explicitas.cs)
            CtorEstatico(s);
            sangria--;
            Linea("}");
            Linea("");
            claseActual = null;
        }

        /// Una interfaz de C# (H7.3, `#756`): la de JC, `interface IForma : IBase { function double Area(); string Nombre
        /// { get; set; } }`. Sus métodos, sólo la firma; sus propiedades, con los dos accesores (en JC una propiedad siempre
        /// los tiene, y una de sólo lectura de C# nadie la escribe por la interfaz: la prueba compila en C#); de las que
        /// hereda, las del programa. Lo demás —genérica, sus eventos e indexadores, un miembro con cuerpo (C# 8) o
        /// estático, una de .NET de padre—, rechazado con su motivo.
        void EmiteInterfaz(INamedTypeSymbol s)
        {
            claseActual = s;
            var partes = s.DeclaringSyntaxReferences.Select(r => (TypeDeclarationSyntax) r.GetSyntax())
                .OrderBy(p => p.SpanStart).ToList();
            var donde = partes[0];
            if (s.IsGenericType) { Rechaza(donde.TypeParameterList ?? (SyntaxNode) donde, Porque.H7(Porque.Genericos)); claseActual = null; return; }
            foreach (var p in partes) Atributos(p.AttributeLists);
            var lugar = donde.BaseList ?? (SyntaxNode) donde;
            var padres = new List<string>();
            foreach (var i in s.Interfaces)
            {
                if (DelPrograma(i) || EsDeCsSystem(i)) padres.Add(Tipo(i, lugar));
                else Rechaza(lugar, PorInterfazAjena(i));
            }
            Linea((s.DeclaredAccessibility == Accessibility.Public ? "public " : "") + "interface " + NombreTipo(s)
                  + (padres.Count > 0 ? " : " + string.Join(", ", padres) : ""));
            Linea("{");
            sangria++;
            foreach (var p in partes)
                foreach (var miembro in p.Members)
                    EmiteMiembroDeInterfaz(miembro);
            sangria--;
            Linea("}");
            Linea("");
            claseActual = null;
        }

        void EmiteMiembroDeInterfaz(MemberDeclarationSyntax d)
        {
            Atributos(d.AttributeLists);
            switch (d)
            {
                case MethodDeclarationSyntax me:
                {
                    var s = m.GetDeclaredSymbol(me);
                    if (me.TypeParameterList != null) { Rechaza(me.TypeParameterList, Porque.H7(Porque.Genericos)); return; }
                    if (s.IsStatic || !s.IsAbstract) { Rechaza(me, Porque.Pendiente("un método de interfaz con cuerpo o estático (C# 8)")); return; }
                    Linea("function " + Tipo(s.ReturnType, me.ReturnType) + " " + NombreMetodo(s) + "(" + Parametros(me.ParameterList) + ");");
                    return;   // su nombre, el que lleve si chocaba (Explicitas.cs)
                }
                case PropertyDeclarationSyntax p:
                {
                    var s = m.GetDeclaredSymbol(p);
                    if (s.IsStatic || !s.IsAbstract) { Rechaza(p, Porque.Pendiente("una propiedad de interfaz con cuerpo o estática (C# 8)")); return; }
                    Linea(Tipo(s.Type, p.Type) + " " + NombreDe(s) + " { get; set; }");
                    return;
                }
                default: RechazaNodo(d); return;
            }
        }

        /// H7.3 (`#756`) — lo que JC pide delante de un método o una propiedad: `@implement{ A, B }`, con las interfaces del
        /// programa que tienen un miembro que éste implementa (lo dice Roslyn; obligatoria en JC si una es de la cabecera,
        /// y valida con cualquiera de las de la clase); y `@override{}` si redefine uno de una clase del programa
        /// (opcional en JC, y así el compilador comprueba que de verdad redefine algo).
        void Anotaciones(ISymbol s)
        {
            var ifaces = new List<string>();
            foreach (var i in claseActual.AllInterfaces.Where(i => DelPrograma(i) || EsDeCsSystem(i)))
                foreach (var mi in i.GetMembers().Where(x => x is IPropertySymbol || (x is IMethodSymbol xm && xm.MethodKind == MethodKind.Ordinary)))
                {
                    var impl = claseActual.FindImplementationForInterfaceMember(mi);
                    // uno que cambió de nombre lo implementa la explícita, o el método que le reenvía (Explicitas.cs)
                    if (deInterfaz.ContainsKey(mi.OriginalDefinition) && InterfazDe(s) == null) continue;
                    // la anotación lleva el nombre sin el módulo (manual §9.8)
                    string nombre = DelPrograma(i) ? NombreTipo(i) : i.Name;
                    if (SymbolEqualityComparer.Default.Equals(impl, s) && !ifaces.Contains(nombre)) ifaces.Add(nombre);
                }
            if (ifaces.Count > 0) Linea("@implement{ " + string.Join(", ", ifaces) + " }");
            ISymbol redef = (s as IMethodSymbol)?.OverriddenMethod ?? (ISymbol) (s as IPropertySymbol)?.OverriddenProperty;
            if (redef != null && DelPrograma(redef.ContainingType)) Linea("@override{}");
        }

        void EmiteMiembro(MemberDeclarationSyntax d)
        {
            Atributos(d.AttributeLists);
            switch (d)
            {
                case FieldDeclarationSyntax f: EmiteCampo(f); break;
                case MethodDeclarationSyntax me: EmiteMetodo(me); break;
                case ConstructorDeclarationSyntax c: EmiteConstructor(c); break;
                case DestructorDeclarationSyntax de: EmiteDestructor(de); break;
                case PropertyDeclarationSyntax p: EmitePropiedad(p); break;
                case BaseTypeDeclarationSyntax _: break;            // las anidadas, sacadas al módulo
                case DelegateDeclarationSyntax _: break;            // ya rechazados, al empezar
                default: RechazaNodo(d); break;
            }
        }

        /// Los atributos se quitan. `[Conditional("S")]` también, y lo que hace, quitar las llamadas al método si `S` no está
        /// definido, lo hace el traductor en cada llamada (CondicionalSinDefinir). Leerlos (GetCustomAttributes…) es la
        /// reflexión, que sigue fuera.
        void Atributos(SyntaxList<AttributeListSyntax> listas)
        {
        }

        /// H8, tema 1 (2-nov) — los símbolos del `[Conditional("S")]` de un método (o del que sobrescribe), entre comillas,
        /// si C# quita sus llamadas: ninguno está definido en el fichero (`#define`, `#undef`) donde se llama. null si el
        /// método no tiene o si alguno está definido.
        string CondicionalSinDefinir(IMethodSymbol ms, SyntaxNode donde)
        {
            var simbolos = new List<string>();
            for (var x = ms?.OriginalDefinition; x != null; x = x.OverriddenMethod?.OriginalDefinition)
                foreach (var a in x.GetAttributes())
                    if (a.AttributeClass?.ToDisplayString() == "System.Diagnostics.ConditionalAttribute"
                        && a.ConstructorArguments.Length == 1 && a.ConstructorArguments[0].Value is string s)
                        simbolos.Add(s);
            if (simbolos.Count == 0) return null;
            var definidos = new HashSet<string>(arbol.Options.PreprocessorSymbolNames);
            foreach (var d in arbol.GetRoot().DescendantTrivia().Where(t => t.IsDirective).Select(t => t.GetStructure()).OfType<DirectiveTriviaSyntax>())
            {
                if (d.SpanStart > donde.SpanStart) break;
                if (!d.IsActive) continue;
                if (d is DefineDirectiveTriviaSyntax df) definidos.Add(df.Name.ValueText);
                else if (d is UndefDirectiveTriviaSyntax ud) definidos.Remove(ud.Name.ValueText);
            }
            return simbolos.Any(definidos.Contains) ? null : string.Join(", ", simbolos.Select(Cadena));
        }

        void EmiteCampo(FieldDeclarationSyntax f)
        {
            var mods = f.Modifiers;
            if (mods.Any(SyntaxKind.FixedKeyword) || mods.Any(SyntaxKind.UnsafeKeyword)) { Rechaza(f, Porque.Fuera(Porque.Unsafe)); return; }
            var vars = f.Declaration.Variables;
            var primero = (IFieldSymbol) m.GetDeclaredSymbol(vars[0]);
            string tipo = Tipo(primero.Type, f.Declaration.Type);
            string acceso = AccesoDe(primero);
            // #777: una constante de C# que vale null (`const string X = null`, test-243): en JC una constante es un valor
            // y no puede serlo; se escribe como lo que hace, una variable estática que nadie cambia. Lo mismo la que no se
            // escribe con un literal (NaN, int.MinValue)
            bool comoVariable = primero.IsConst && primero.HasConstantValue
                                && (primero.ConstantValue == null || !TieneLiteral(primero.ConstantValue));
            if (primero.IsConst && !comoVariable)
            {
                foreach (var v in vars)                                        // JC: una constante por declaración
                    Linea(acceso + " const " + tipo + " " + NombreDe(m.GetDeclaredSymbol(v)) + " = " + ValorDeConstante(v) + ";");
                return;
            }
            string estatico = (primero.IsStatic || comoVariable) ? "static " : "";
            if (acceso == "public")
            {
                // BP no deja campos públicos («el estado sale por getters y setters»): cada uno, una propiedad.
                foreach (var v in vars)
                {
                    string valor = v.Initializer != null ? " = " + Expr(ValorDeCampo(v)) : NaceDeCSharp(primero.Type, v);
                    Linea("public " + estatico + tipo + " " + NombreDe(m.GetDeclaredSymbol(v)) + " { get; set; }"
                          + (valor != "" ? valor + ";" : ""));
                }
                return;
            }
            Linea(acceso + " " + estatico + tipo + " " + string.Join(", ", vars.Select(v => NombreDe(m.GetDeclaredSymbol(v))
                  + (v.Initializer != null ? " = " + Expr(ValorDeCampo(v)) : NaceDeCSharp(primero.Type, v)))) + ";");
        }

        /// ¿Se escribe este valor con un literal? No NaN, los infinitos ni el mínimo de un entero, que en JC son una cuenta.
        static bool TieneLiteral(object v) => !(v is double d && !double.IsFinite(d)) && !(v is float f && !float.IsFinite(f))
                                              && !(v is int i && i == int.MinValue) && !(v is long l && l == long.MinValue)
                                              && !(v is ulong u && u == 9223372036854775808UL);

        /// El valor de una constante de clase. JC pide un literal, y C# deja cualquier cuenta constante (`const int X = 1 + 2;`,
        /// `const bool B = !A & C;`: test-484): el valor que calcula Roslyn. Un estrecho, sin su conversión; un enum, como
        /// está escrito (el compilador sabe su valor).
        string ValorDeConstante(VariableDeclaratorSyntax v)
        {
            var f = (IFieldSymbol) m.GetDeclaredSymbol(v);
            var e = v.Initializer.Value;
            if (e is LiteralExpressionSyntax || EsEnum(f.Type) || !f.HasConstantValue) return Expr(e);
            if (EsEstrecho(f.Type)) return System.Convert.ToString(f.ConstantValue, System.Globalization.CultureInfo.InvariantCulture);
            return LiteralDeConstante(f.ConstantValue, v);
        }

        /// El valor de un campo. `static bool x = x = F();` (H8, tema 2; test-422) se da a sí mismo el valor que va a tener:
        /// lo de la derecha, sin asignar nada dentro (en el valor de un campo no hay sentencia donde sacarlo).
        ExpressionSyntax ValorDeCampo(VariableDeclaratorSyntax v)
        {
            var campo = m.GetDeclaredSymbol(v);
            var e = v.Initializer.Value;
            while (e is AssignmentExpressionSyntax a && a.IsKind(SyntaxKind.SimpleAssignmentExpression)
                   && SymbolEqualityComparer.Default.Equals(m.GetSymbolInfo(a.Left).Symbol, campo))
                e = a.Right;
            return e;
        }

        /// Un campo o una propiedad sin valor, con lo que le da C# si en JC nacería con otra cosa: un `string`, null (#774;
        /// en JC, "", lo de módulo o de clase sin valor), para que `campo == null` siga dando true; un struct sin estado,
        /// un objeto (en JC, null).
        string NaceDeCSharp(ITypeSymbol t, SyntaxNode donde) =>
            EsCadena(t) ? " = null" : EsStructSinEstado(t) ? " = new " + Tipo(t, donde) + "()" : "";

        void EmiteMetodo(MethodDeclarationSyntax me)
        {
            var s = m.GetDeclaredSymbol(me);
            // un [Conditional] sin definir: C# quita todas sus llamadas, y nadie llega a su cuerpo (test-238)
            if (CondicionalSinDefinir(s, me) != null) return;
            if (me.TypeParameterList != null) { Rechaza(me.TypeParameterList, Porque.H7(Porque.Genericos)); return; }
            if (s.IsExtern) { Rechaza(me, Porque.Fuera(Porque.Interop)); return; }
            if (s.IsAsync) { Rechaza(me, Porque.Fuera(Porque.Async)); return; }
            if (me.Modifiers.Any(SyntaxKind.UnsafeKeyword)) { Rechaza(me, Porque.Fuera(Porque.Unsafe)); return; }
            if (s.IsPartialDefinition)
            {
                // la definición sin cuerpo: se escribe la implementación; sin ella, nada, y C# quita las llamadas (en
                // Sentencias, la de una sentencia; una que no lo es, C# no la deja: void)
                return;
            }
            if (Oculta(s)) { Rechaza(me, Porque.H7(Porque.NoVirtuales)); return; }
            bool esMain = main != null && (SymbolEqualityComparer.Default.Equals(s, main)
                                           || SymbolEqualityComparer.Default.Equals(s.PartialDefinitionPart, main)
                                           || SymbolEqualityComparer.Default.Equals(s, main.PartialImplementationPart));
            // `int IForma.Area()`: la implementación explícita, con el nombre del de la interfaz (Explicitas.cs)
            string nombre = NombreMetodo(s);
            bool explicita = s.ExplicitInterfaceImplementations.Length > 0;
            if (explicita)
            {
                nombre = NombreMetodo(s.ExplicitInterfaceImplementations[0]);
                if (explicitasDeFuera.Contains(s))
                {
                    Rechaza(me, Porque.Pendiente("una implementación explícita y otro miembro con su nombre"));
                    return;
                }
            }
            string cabecera = (esMain || explicita ? "public" : AccesoDe(s)) + (s.IsStatic ? " static" : "")
                + " function " + Tipo(s.ReturnType, me.ReturnType) + " " + nombre
                + "(" + Parametros(me.ParameterList) + ")";
            if (!s.IsStatic) Anotaciones(s);
            Linea(cabecera);
            if (s.IsAbstract) { Abstracto(nombre); return; }
            Cuerpo(me.Body, me.ExpressionBody, s.ReturnsVoid);
        }

        /// El cuerpo de lo abstracto (un método `abstract`, o uno de una interfaz): lanza. Nunca se llama: C# no deja
        /// crear ni la clase abstracta ni la interfaz, y una hija que se crea lo sobrescribe.
        void Abstracto(string nombre)
        {
            Linea("{");
            Linea("   throw new RuntimeError(" + Cadena("abstracto: " + NombreTipo(claseActual) + "." + nombre) + ");");
            Linea("}");
        }

        static bool MismosParametros(System.Collections.Immutable.ImmutableArray<IParameterSymbol> a,
                                     System.Collections.Immutable.ImmutableArray<IParameterSymbol> b) =>
            a.Length == b.Length && a.Zip(b, (x, y) => SymbolEqualityComparer.Default.Equals(x.Type, y.Type)).All(v => v);

        /// El constructor estático de la clase que se está escribiendo.
        ConstructorDeclarationSyntax estatico;

        /// H8, tema 2 — un constructor estático. En C# corre la primera vez que se usa la clase; en JC, los valores de lo
        /// estático se calculan al cargar el módulo. No se nota si sólo da valores a lo estático de su clase con lo que no
        /// tiene efectos ni mira nada de fuera (SoloValores): nadie puede ver esos campos antes de que corra. Se escribe
        /// detrás de todo, porque en C# corre detrás de los valores de los campos: una función, y un campo que la llama.
        /// Vacío, nada. Con otra cosa, sigue sin traducirse.
        void CtorEstatico(INamedTypeSymbol s)
        {
            var c = estatico;
            estatico = null;
            if (c == null) return;
            var sentencias = c.Body != null ? c.Body.Statements.ToList()
                : c.ExpressionBody != null ? new List<StatementSyntax>() : null;
            var asignaciones = new List<AssignmentExpressionSyntax>();
            if (c.ExpressionBody?.Expression is AssignmentExpressionSyntax ae) asignaciones.Add(ae);
            else if (c.ExpressionBody != null) { Rechaza(c, Porque.H7(Porque.CtorEstatico)); return; }
            foreach (var x in sentencias)
            {
                if (x is EmptyStatementSyntax) continue;
                if (x is ExpressionStatementSyntax es && es.Expression is AssignmentExpressionSyntax a) asignaciones.Add(a);
                else { Rechaza(c, Porque.H7(Porque.CtorEstatico)); return; }
            }
            if (asignaciones.Count == 0) return;
            foreach (var a in asignaciones)
            {
                var d = m.GetSymbolInfo(a.Left).Symbol;
                if (!a.IsKind(SyntaxKind.SimpleAssignmentExpression) || !(d is IFieldSymbol || d is IPropertySymbol) || !d.IsStatic
                    || !SymbolEqualityComparer.Default.Equals(d.ContainingType, s) || !SoloValores(a.Right, s))
                { Rechaza(c, Porque.H7(Porque.CtorEstatico)); return; }
            }
            Linea("private static int cs_estatico = " + NombreTipo(s) + ".Cs_Estatico();");
            Linea("private static function int Cs_Estatico()");
            Linea("{");
            sangria++;
            foreach (var a in asignaciones) Linea(ExprSentencia(a) + ";");
            Linea("return 0;");
            sangria--;
            Linea("}");
        }

        /// ¿Da igual cuándo se calcule `e`? Sin llamadas, sin nada que cambie, sin leer lo estático de otra clase; un `new`,
        /// sólo de un constructor implícito.
        bool SoloValores(ExpressionSyntax e, INamedTypeSymbol clase)
        {
            foreach (var n in e.DescendantNodesAndSelf())
                switch (n)
                {
                    case InvocationExpressionSyntax _:
                    case AssignmentExpressionSyntax _:
                    case PostfixUnaryExpressionSyntax _:
                    case PrefixUnaryExpressionSyntax pu when pu.IsKind(SyntaxKind.PreIncrementExpression) || pu.IsKind(SyntaxKind.PreDecrementExpression):
                        return false;
                    case ObjectCreationExpressionSyntax oc:
                        if (!(m.GetSymbolInfo(oc).Symbol is IMethodSymbol ctor) || !ctor.IsImplicitlyDeclared || oc.Initializer != null) return false;
                        break;
                    case SimpleNameSyntax id:
                        var s = m.GetSymbolInfo(id).Symbol;
                        if (s is IPropertySymbol) return false;
                        if (s is IFieldSymbol f && !f.IsConst && !SymbolEqualityComparer.Default.Equals(f.ContainingType, clase)) return false;
                        break;
                }
            return true;
        }

        void EmiteConstructor(ConstructorDeclarationSyntax c)
        {
            var s = m.GetDeclaredSymbol(c);
            if (s.IsStatic) { estatico = c; return; }                     // detrás de los campos: CtorEstatico
            string cabecera = AccesoDe(s) + " function " + NombreTipo(claseActual)
                + "(" + Parametros(c.ParameterList) + ")";
            var ini = c.Initializer;
            var b = claseActual.BaseType;
            // `: base()` hacia object: vale (Eduardo, 31-oct: «Todas las clases tienen un padre salvo Object así que
            // base.metodo() debería ser válido»), y se copia (hasta #753, cerrada el 1-nov, JC no lo compilaba)
            bool haciaLaBase = ini != null && ini.IsKind(SyntaxKind.BaseConstructorInitializer);
            if (ini != null)
            {
                var objetivo = m.GetSymbolInfo(ini).Symbol as IMethodSymbol;
                if (haciaLaBase && MensajeSinArgumentos(b, claseActual) is string porDefecto)
                    cabecera += " : base(" + ArgsExcepcion(ini.ArgumentList, porDefecto) + ")";
                else if (haciaLaBase && !DelPrograma(b) && ExcepcionJc(b) != null && !ConstructorDeCsSystem(objetivo, b, ini))
                    return;
                else
                    cabecera += (haciaLaBase ? " : base(" : " : this(") + Argumentos(ini.ArgumentList, objetivo) + ")";
            }
            else if (MensajeSinArgumentos(b, claseActual) is string porDefecto)
                cabecera += " : base(" + Cadena(porDefecto) + ")";
            Linea(cabecera);
            Cuerpo(c.Body, c.ExpressionBody, true);
        }

        void EmiteDestructor(DestructorDeclarationSyntax d)
        {
            Linea("function ~" + NombreTipo(claseActual) + "()");
            Cuerpo(d.Body, d.ExpressionBody, true);
        }

        void EmitePropiedad(PropertyDeclarationSyntax p)
        {
            var s = m.GetDeclaredSymbol(p);
            if (OcultaPropiedad(s)) { Rechaza(p, Porque.H7(Porque.NoVirtuales)); return; }
            string nombre = NombreDe(s);
            bool explicita = s.ExplicitInterfaceImplementations.Length > 0;
            if (explicita)
            {
                nombre = NombreDe(s.ExplicitInterfaceImplementations[0]);
                if (explicitasDeFuera.Contains(s))
                {
                    Rechaza(p, Porque.Pendiente("una implementación explícita y otro miembro con su nombre"));
                    return;
                }
            }
            string cabecera = (explicita ? "public" : AccesoDe(s)) + (s.IsStatic ? " static" : "") + " "
                + Tipo(s.Type, p.Type) + " " + nombre;
            // de sólo lectura y con una hija que la redefine: su campo, a la vista (Explicitas.cs)
            bool campo = conCampo.Contains(s.OriginalDefinition);
            if (campo)
                Linea("private " + Tipo(s.Type, p.Type) + " " + nombre + "_campo" + (p.Initializer != null ? " = " + Expr(p.Initializer.Value) : NaceDeCSharp(s.Type, p)) + ";");
            if (!s.IsStatic) Anotaciones(s);
            if (campo)
            {
                Linea(cabecera);
                Linea("{");
                Linea("   get { return this." + nombre + "_campo; }");
                Linea("}");
                return;
            }
            if (s.IsAbstract)
            {
                // una propiedad abstracta, o la de una interfaz: sus accesores lanzan
                Linea(cabecera);
                Linea("{");
                sangria++;
                foreach (var a in GetPrimero(p.AccessorList.Accessors))
                {
                    Linea(a.IsKind(SyntaxKind.GetAccessorDeclaration) ? "get" : "set");
                    Abstracto(nombre);
                }
                sangria--;
                Linea("}");
                return;
            }
            if (p.ExpressionBody != null)
            {
                Linea(cabecera);
                Linea("{");
                sangria++;
                Linea("get");
                Cuerpo(null, p.ExpressionBody, false);
                sangria--;
                Linea("}");
                return;
            }
            var acc = GetPrimero(p.AccessorList.Accessors);
            if (acc.All(a => a.Body == null && a.ExpressionBody == null))
            {
                // automática; `{ get; }` e `{ get; init; }` también: C# ya comprobó que nadie más la escribe
                string valor = p.Initializer != null ? " = " + Expr(p.Initializer.Value) : NaceDeCSharp(s.Type, p);
                Linea(cabecera + " { get; set; }" + (valor != "" ? valor + ";" : ""));
                return;
            }
            Linea(cabecera);
            Linea("{");
            sangria++;
            foreach (var a in acc)
            {
                if (a.IsKind(SyntaxKind.GetAccessorDeclaration)) { Linea("get"); Cuerpo(a.Body, a.ExpressionBody, false); }
                else if (a.IsKind(SyntaxKind.SetAccessorDeclaration) || a.IsKind(SyntaxKind.InitAccessorDeclaration))
                {
                    Linea("set");
                    Cuerpo(a.Body, a.ExpressionBody, true);
                }
                else RechazaNodo(a);
            }
            sangria--;
            Linea("}");
        }

        /// Los accesores de una propiedad, con el `get` delante: JC lo quiere así, y C# deja `{ set {…} get {…} }`
        /// (test-272); el orden no cambia lo que hace cada uno.
        static List<AccessorDeclarationSyntax> GetPrimero(SyntaxList<AccessorDeclarationSyntax> acc) =>
            acc.OrderBy(a => a.IsKind(SyntaxKind.GetAccessorDeclaration) ? 0 : 1).ToList();

        string Parametros(ParameterListSyntax pl)
        {
            var partes = new List<string>();
            foreach (var p in pl.Parameters)
            {
                var ps = m.GetDeclaredSymbol(p);
                if (ps == null) { partes.Add(Rechaza(p, Porque.Fuera(Porque.Interop))); continue; }     // __arglist
                if (ps.RefKind != RefKind.None) Rechaza(p, Porque.H7(Porque.RefOut));
                // `int c = 0, params int[] oo` (gtest-optional-36): en JC, uno sin valor no va tras uno con él
                if (ps.IsParams && pl.Parameters.Any(q => q.Default != null))
                    Rechaza(p, Porque.Pendiente("un params detrás de un valor por defecto"));
                // de un método partial, el valor por defecto es el de su declaración sin cuerpo: la que lo define en C#
                var definicion = (ps.ContainingSymbol as IMethodSymbol)?.PartialDefinitionPart;
                var conValor = definicion != null ? definicion.Parameters[ps.Ordinal] : ps;
                var pValor = conValor.DeclaringSyntaxReferences.FirstOrDefault()?.GetSyntax() as ParameterSyntax ?? p;
                partes.Add(Tipo(ps.Type, p.Type) + " " + Ident(ps.Name)
                           + (pValor.Default != null ? " = " + ValorPorDefecto(conValor, pValor) : ""));
            }
            return string.Join(", ", partes);
        }

        /// El cuerpo de un método, un constructor o un accesor: el bloque, o `=> e` como `{ return e; }` (o `{ e; }`).
        void Cuerpo(BlockSyntax bloque, ArrowExpressionClauseSyntax flecha, bool sinValor)
        {
            if (bloque != null) { Bloque(bloque); return; }
            Linea("{");
            sangria++;
            if (flecha != null)
            {
                if (sinValor && flecha.Expression is InvocationExpressionSyntax llamada
                    && CondicionalSinDefinir(m.GetSymbolInfo(llamada).Symbol as IMethodSymbol, llamada) is string sc)
                    Linea("// " + ((IMethodSymbol) m.GetSymbolInfo(llamada).Symbol).Name + "(…): [Conditional(" + sc + ")], sin definir; C# quita la llamada");
                else if (sinValor) Linea(ExprSentencia(flecha.Expression) + ";");
                else Linea("return " + Expr(flecha.Expression) + ";");
            }
            sangria--;
            Linea("}");
        }

        // ------------------------------------------------------------------------------------------------ utilidades

        static string Acceso(Accessibility a)
        {
            switch (a)
            {
                case Accessibility.Public:
                case Accessibility.Internal:
                case Accessibility.ProtectedOrInternal: return "public";     // todo está en un módulo
                case Accessibility.Protected:
                case Accessibility.ProtectedAndInternal: return "protected";
                default: return "private";
            }
        }

        /// Un nombre de C# en JC: sin la `@`, y con un `_` detrás si es palabra reservada de JC.
        string Ident(string nombre)
        {
            if (nombre.StartsWith("@")) nombre = nombre.Substring(1);
            // `__delegate`: en BP lo que empieza por `__` es del sistema; se renombra como una palabra reservada, con los
            // guiones al final (`delegate__`)
            if (nombre.StartsWith("__") && nombre.TrimStart('_').Length > 0)
            {
                string limpio = nombre.TrimStart('_');
                string nuevo = limpio + new string('_', nombre.Length - limpio.Length);
                if (renombrados.Add(nombre)) t.Avisos.Add("renombrado: " + nombre + " → " + nuevo + " (en BP, lo que empieza por __ es del sistema)");
                nombre = nuevo;
            }
            if (!ReservadasJc.Contains(nombre)) return nombre;
            if (renombrados.Add(nombre)) t.Avisos.Add("renombrado: " + nombre + " → " + nombre + "_ (palabra reservada de JC)");
            return nombre + "_";
        }

        string NombreTipo(INamedTypeSymbol s)
        {
            var o = s.OriginalDefinition;
            if (nombresDeTipo.TryGetValue(o, out var n)) return n;
            n = (o.ContainingType != null ? NombreTipo(o.ContainingType) + "_"
                 : conEspacio.Contains(o) ? o.ContainingNamespace.ToDisplayString().Replace('.', '_') + "_" : "")
                + Ident(o.Name);
            if (ClasesDeCore.Contains(n))
            {
                t.Avisos.Add("renombrado: " + n + " → " + n + "_ (una clase de Core)");
                n += "_";
            }
            else if (nombresDeValor.Contains(n))
            {
                t.Avisos.Add("renombrado: " + n + " → " + n + "_ (un tipo que se llama como una variable: el «Color Color» de C#)");
                n += "_";
            }
            nombresDeTipo[o] = n;
            return n;
        }

        /// Los nombres de las variables, los campos, las propiedades y los parámetros del programa (el «Color Color»).
        readonly HashSet<string> nombresDeValor = new HashSet<string>();

        /// El nombre de una clase delante de un miembro estático (`Foo.x`). Si a la vista hay una variable, un campo o
        /// una propiedad que se llama igual (el «Color Color» de C#: `Foo Foo; … Foo.x`), JC la toma por ella.
        string Receptor(INamedTypeSymbol t, SyntaxNode donde)
        {
            string n = NombreTipo(t);
            if (n == t.Name && HayValorLlamado(t.Name, donde)) Rechaza(donde, Porque.H7(Porque.ColorColor));
            return n;
        }

        /// ¿Hay a la vista una variable, un campo o una propiedad con este nombre? Para JC, también los privados de la
        /// clase padre, que C# no ve desde la hija.
        bool HayValorLlamado(string nombre, SyntaxNode donde)
        {
            if (m.LookupSymbols(donde.SpanStart, null, nombre)
                    .Any(s => s is ILocalSymbol || s is IParameterSymbol || s is IFieldSymbol || s is IPropertySymbol))
                return true;
            for (var c = claseActual; c != null; c = c.BaseType)
                if (c.GetMembers(nombre).Any(s => s is IFieldSymbol || s is IPropertySymbol)) return true;
            return false;
        }

        /// Una interfaz de .NET: las genéricas, de H7; IEnumerable e IEnumerator, el foreach propio (H7); las demás, la
        /// biblioteca.
        static string PorInterfazAjena(INamedTypeSymbol i)
        {
            if (i.IsGenericType) return Porque.H7(Porque.Genericos);
            string n = NombreCompleto(i);
            if (n == "System.Collections.IEnumerable" || n == "System.Collections.IEnumerator") return Porque.H7(Porque.ForeachPropio);
            return Porque.DeNet("implementa " + n);
        }

        /// El valor por defecto de un parámetro: el que calcula C#, como literal (JC sólo admite un literal: «el valor
        /// por defecto del parámetro debe ser una constante literal»; C#, cualquier constante).
        string ValorPorDefecto(IParameterSymbol ps, ParameterSyntax p) => ConstanteDe(ps, p.Default);

        /// El valor por defecto de un parámetro, como literal: el de su declaración, o el de uno que se salta una llamada
        /// con argumentos con nombre.
        string ConstanteDe(IParameterSymbol ps, SyntaxNode donde)
        {
            if (!ps.HasExplicitDefaultValue) return Rechaza(donde, Porque.Pendiente("un valor por defecto sin valor"));
            string porTipo = PorTipo(ps.Type);
            if (porTipo != null) return Rechaza(donde, porTipo);
            var v = ps.ExplicitDefaultValue;
            if (v is ulong grande) v = unchecked((long) grande);       // un ulong, sus 64 bits (SinSigno.cs)
            // un T? con valor (`int? x = 5`): en JC, un objeto, y JC sólo admite un literal (null, como siempre: H7)
            if (EsNullable(ps.Type) && v != null) return Rechaza(donde, Porque.Pendiente("un valor por defecto de un T? (en JC, un objeto)"));
            // un enum (`Dia d = Dia.Martes`): JC sólo admite un literal, y un miembro de un enum no lo es
            if (ps.Type.TypeKind == TypeKind.Enum) return Rechaza(donde, Porque.H7(Porque.EnumPorDefecto));
            var ci = System.Globalization.CultureInfo.InvariantCulture;
            switch (v)
            {
                case null: return Rechaza(donde, Porque.H7(Porque.NullPorDefecto));   // también un string (#774: JC pide un literal)
                case bool b: return b ? "true" : "false";
                case string s: return Cadena(s);
                case char ch: return ((int) ch).ToString(ci);           // un char, su código: en JC es un entero
                case int i: return i == int.MinValue ? "(-2147483647 - 1)" : i.ToString(ci);
                case long l: return l == long.MinValue ? "(-9223372036854775807L - 1L)" : l.ToString(ci) + "L";
                case uint u: return u.ToString(ci) + "L";                    // un uint es un long (SinSigno.cs)
                case short _: case sbyte _: case byte _: case ushort _: return System.Convert.ToString(v, ci);
                case double d:
                {
                    if (double.IsNaN(d) || double.IsInfinity(d)) break;
                    string r = d.ToString("R", ci);
                    return r.Contains('.') || r.Contains('E') ? r : r + ".0";
                }
                case float f:
                {
                    if (float.IsNaN(f) || float.IsInfinity(f)) break;
                    string r = f.ToString("R", ci);
                    return (r.Contains('.') || r.Contains('E') ? r : r + ".0") + "f";
                }
            }
            return Rechaza(donde, Porque.Pendiente("un valor por defecto " + v));
        }

        string NombreMetodo(IMethodSymbol s) => EsToString(s) ? "toString" : NombreDe(s);

        /// `ToString` es el `toString` de BP (la ranura 0 de Object): quien lo sobrescribe en C# lo sobrescribe en JC.
        static bool EsToString(IMethodSymbol s)
        {
            for (var o = s; o != null; o = o.OverriddenMethod)
                if (o.Name == "ToString" && o.Parameters.Length == 0 && o.ContainingType.SpecialType == SpecialType.System_Object)
                    return true;
            return false;
        }

        /// ¿Un método que oculta otro de un padre de .NET? Al de un padre del programa, sí se oculta: lleva otro nombre
        /// (Explicitas.cs). Al de .NET, no se sabe si la clase de BP lo tiene.
        bool Oculta(IMethodSymbol s)
        {
            if (s.IsStatic || s.IsOverride) return false;
            for (var b = s.ContainingType.BaseType; b != null; b = b.BaseType)
                if (!DelPrograma(b))
                    foreach (var o in b.GetMembers(s.Name).OfType<IMethodSymbol>())
                        if (!o.IsStatic && o.DeclaredAccessibility != Accessibility.Private && o.MethodKind == MethodKind.Ordinary && MismosParametros(o, s))
                            return true;
            return false;
        }

        bool OcultaPropiedad(IPropertySymbol s)
        {
            if (s.IsStatic || s.IsOverride) return false;
            for (var b = s.ContainingType.BaseType; b != null; b = b.BaseType)
                if (!DelPrograma(b) && b.GetMembers(s.Name).Any(o => !o.IsStatic && !(o is INamedTypeSymbol) && o.DeclaredAccessibility != Accessibility.Private))
                    return true;
            return false;
        }

        static bool MismosParametros(IMethodSymbol a, IMethodSymbol b)
        {
            if (a.Parameters.Length != b.Parameters.Length) return false;
            for (int i = 0; i < a.Parameters.Length; i++)
                if (!SymbolEqualityComparer.Default.Equals(a.Parameters[i].Type, b.Parameters[i].Type)
                    || a.Parameters[i].RefKind != b.Parameters[i].RefKind)
                    return false;
            return true;
        }

        bool DelPrograma(ISymbol s) => s != null && s.Locations.Any(l => l.IsInSource && l.SourceTree == arbol);

        /// El mensaje que pone .NET a una excepción creada sin él: «Exception of type 'N.A+B' was thrown.»
        static string MensajePorDefecto(INamedTypeSymbol s) => "Exception of type '" + NombreNet(s) + "' was thrown.";

        static string NombreNet(INamedTypeSymbol s)
        {
            if (s.ContainingType != null) return NombreNet(s.ContainingType) + "+" + s.Name;
            var ns = s.ContainingNamespace;
            return ns == null || ns.IsGlobalNamespace ? s.Name : ns.ToDisplayString() + "." + s.Name;
        }

        /// Los argumentos hacia la Exception o el RuntimeError de BP (System.Exception y SystemException): sin ninguno, el
        /// mensaje de .NET; con uno, ése.
        string ArgsExcepcion(ArgumentListSyntax args, string porDefecto)
        {
            if (args == null || args.Arguments.Count == 0) return Cadena(porDefecto);
            if (args.Arguments.Count == 1) return Expr(args.Arguments[0].Expression);
            return Rechaza(args, Porque.Pendiente("una excepción con su causa (innerException)"));
        }

        static string Cadena(string s)
        {
            var r = new StringBuilder("\"");
            foreach (char c in s)
            {
                switch (c)
                {
                    case '"': r.Append("\\\""); break;
                    case '\\': r.Append("\\\\"); break;
                    case '\n': r.Append("\\n"); break;
                    case '\r': r.Append("\\r"); break;
                    case '\t': r.Append("\\t"); break;
                    case '\0': r.Append("\\0"); break;
                    default:
                        if (c < 32) r.Append("\\u").Append(((int) c).ToString("x4"));
                        else r.Append(c);
                        break;
                }
            }
            return r.Append('"').ToString();
        }

        SyntaxNode Declaracion(INamedTypeSymbol s) => s.DeclaringSyntaxReferences.First().GetSyntax();

        string NombreNuevo() => "excepcion_cs" + (++nombresNuevos);

        void Linea(string s)
        {
            if (s.Length > 0) sb.Append(' ', 3 * (sangria + 1)).Append(s);
            sb.Append('\n');
        }

        string Rechaza(SyntaxNode n, string porque)
        {
            int linea = n.GetLocation().GetLineSpan().StartLinePosition.Line + 1;
            if (!t.Rechazos.Any(r => r.Linea == linea && r.Porque == porque))
            {
                string que = string.Join(" ", n.ToString().Split(new[] { '\r', '\n', '\t' }, StringSplitOptions.RemoveEmptyEntries)
                    .Select(x => x.Trim()));
                if (que.Length > 60) que = que.Substring(0, 57) + "...";
                t.Rechazos.Add(new Rechazo { Linea = linea, Que = que, Porque = porque });
            }
            return Hueco;
        }

        string RechazaNodo(SyntaxNode n) =>
            Rechaza(n, Porque.PorNodo.TryGetValue(n.Kind(), out var p) ? p : Porque.Pendiente(n.Kind().ToString()));
    }
}
