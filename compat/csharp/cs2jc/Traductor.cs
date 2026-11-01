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
                if (acc != Accessibility.Private && Hereda(desde.OriginalDefinition, dueno)) continue;   // una hija lo ve en JC
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
                case TypeKind.Struct: Rechaza(donde, Porque.H7(Porque.Struct)); break;
                case TypeKind.Enum: EmiteEnum(s); break;
                default: Rechaza(donde, Porque.Pendiente("tipo " + s.TypeKind)); break;
            }
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
            if (b != null && b.SpecialType != SpecialType.System_Object)
            {
                if (DelPrograma(b)) base_ = Tipo(b, donde);
                else if (ExcepcionJc(b) is string ex) base_ = ex;     // una excepción de .NET (CsSystem, el escalón 1)
                else Rechaza(lugarBase, Porque.DeNet("hereda de " + NombreCompleto(b)));
            }
            // Las interfaces (H7.3, #756; antes, con #745, una sola y como clase base): las del programa, en la cabecera
            // detrás del padre, como en C# (`class C : B, I, J`); las de .NET, la biblioteca. IDisposable no cuenta:
            // basta con su Dispose.
            var supertipos = new List<string>();
            if (base_ != null) supertipos.Add(base_);
            foreach (var i in s.Interfaces.Where(i => DelPrograma(i))) supertipos.Add(Tipo(i, lugarBase));
            foreach (var i in s.Interfaces.Where(i => !DelPrograma(i) && NombreCompleto(i) != "System.IDisposable"))
                Rechaza(lugarBase, PorInterfazAjena(i));

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
                if (DelPrograma(i)) padres.Add(Tipo(i, lugar));
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
                    return;
                }
                case PropertyDeclarationSyntax p:
                {
                    var s = m.GetDeclaredSymbol(p);
                    if (s.IsStatic || !s.IsAbstract) { Rechaza(p, Porque.Pendiente("una propiedad de interfaz con cuerpo o estática (C# 8)")); return; }
                    Linea(Tipo(s.Type, p.Type) + " " + Ident(s.Name) + " { get; set; }");
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
            foreach (var i in claseActual.AllInterfaces.Where(DelPrograma))
                foreach (var mi in i.GetMembers().Where(x => x is IPropertySymbol || (x is IMethodSymbol xm && xm.MethodKind == MethodKind.Ordinary)))
                {
                    var impl = claseActual.FindImplementationForInterfaceMember(mi);
                    if (SymbolEqualityComparer.Default.Equals(impl, s) && !ifaces.Contains(NombreTipo(i))) ifaces.Add(NombreTipo(i));
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

        /// Los atributos se quitan; `[Conditional]` no, porque quita las llamadas al método.
        void Atributos(SyntaxList<AttributeListSyntax> listas)
        {
            foreach (var a in listas.SelectMany(l => l.Attributes))
            {
                var tipo = m.GetTypeInfo(a).Type ?? (m.GetSymbolInfo(a).Symbol as IMethodSymbol)?.ContainingType;
                if (tipo != null && tipo.ToDisplayString() == "System.Diagnostics.ConditionalAttribute")
                    Rechaza(a, Porque.Fuera(Porque.Reflexion));
            }
        }

        void EmiteCampo(FieldDeclarationSyntax f)
        {
            var mods = f.Modifiers;
            if (mods.Any(SyntaxKind.FixedKeyword) || mods.Any(SyntaxKind.UnsafeKeyword)) { Rechaza(f, Porque.Fuera(Porque.Unsafe)); return; }
            if (mods.Any(SyntaxKind.NewKeyword)) { Rechaza(f, Porque.H7(Porque.NoVirtuales)); return; }
            var vars = f.Declaration.Variables;
            var primero = (IFieldSymbol) m.GetDeclaredSymbol(vars[0]);
            string tipo = Tipo(primero.Type, f.Declaration.Type);
            string acceso = AccesoDe(primero);
            if (primero.IsConst)
            {
                foreach (var v in vars)                                        // JC: una constante por declaración
                    Linea(acceso + " const " + tipo + " " + Ident(v.Identifier.ValueText) + " = " + Expr(v.Initializer.Value) + ";");
                return;
            }
            string estatico = primero.IsStatic ? "static " : "";
            if (acceso == "public")
            {
                // BP no deja campos públicos («el estado sale por getters y setters»): cada uno, una propiedad.
                foreach (var v in vars)
                    Linea("public " + estatico + tipo + " " + Ident(v.Identifier.ValueText) + " { get; set; }"
                          + (v.Initializer != null ? " = " + Expr(v.Initializer.Value) + ";" : ""));
                return;
            }
            Linea(acceso + " " + estatico + tipo + " " + string.Join(", ", vars.Select(v => Ident(v.Identifier.ValueText)
                  + (v.Initializer != null ? " = " + Expr(v.Initializer.Value) : ""))) + ";");
        }

        void EmiteMetodo(MethodDeclarationSyntax me)
        {
            var s = m.GetDeclaredSymbol(me);
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
            if (me.Modifiers.Any(SyntaxKind.NewKeyword) || Oculta(s)) { Rechaza(me, Porque.H7(Porque.NoVirtuales)); return; }
            bool esMain = main != null && (SymbolEqualityComparer.Default.Equals(s, main)
                                           || SymbolEqualityComparer.Default.Equals(s.PartialDefinitionPart, main)
                                           || SymbolEqualityComparer.Default.Equals(s, main.PartialImplementationPart));
            // `int IForma.Area()`: la implementación explícita de la interfaz que en JC es la clase base, con su nombre
            string nombre = NombreMetodo(s);
            bool explicita = s.ExplicitInterfaceImplementations.Length > 0;
            if (explicita)
            {
                nombre = Ident(s.ExplicitInterfaceImplementations[0].Name);
                if (ChocaConOtro(s, s.ExplicitInterfaceImplementations[0].Name))
                {
                    Rechaza(me, Porque.Pendiente("una implementación explícita y otro miembro con su nombre"));
                    return;
                }
                if (ChocaConOtraExplicita(s)) { Rechaza(me, Porque.H7(Porque.ExplicitaDoble)); return; }
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

        /// H7.3 — ¿otra implementación explícita de la clase se escribiría igual en JC: el mismo nombre y los mismos
        /// parámetros (`void IA.Draw()` y `void IB.Draw()`)? Es la implementación explícita de verdad, dos cuerpos para un
        /// mismo método, uno por interfaz: JC no la tiene. Con otra firma, son dos sobrecargas y valen (Mono test-457).
        bool ChocaConOtraExplicita(ISymbol s)
        {
            string n = (s as IMethodSymbol)?.ExplicitInterfaceImplementations[0].Name ?? ((IPropertySymbol) s).ExplicitInterfaceImplementations[0].Name;
            var ps = (s as IMethodSymbol)?.Parameters ?? ((IPropertySymbol) s).Parameters;
            return claseActual.GetMembers().Any(o => !SymbolEqualityComparer.Default.Equals(o, s) && o.Kind == s.Kind
                && ((o as IMethodSymbol)?.ExplicitInterfaceImplementations.FirstOrDefault()?.Name
                    ?? (o as IPropertySymbol)?.ExplicitInterfaceImplementations.FirstOrDefault()?.Name) == n
                && MismosParametros((o as IMethodSymbol)?.Parameters ?? ((IPropertySymbol) o).Parameters, ps));
        }

        static bool MismosParametros(System.Collections.Immutable.ImmutableArray<IParameterSymbol> a,
                                     System.Collections.Immutable.ImmutableArray<IParameterSymbol> b) =>
            a.Length == b.Length && a.Zip(b, (x, y) => SymbolEqualityComparer.Default.Equals(x.Type, y.Type)).All(v => v);

        /// ¿Tiene la clase otro miembro que se llame así (y que no sea una implementación explícita)?
        bool ChocaConOtro(ISymbol s, string nombre) =>
            claseActual.GetMembers(nombre).Any(o => !SymbolEqualityComparer.Default.Equals(o, s)
                && !(o is IMethodSymbol om && om.ExplicitInterfaceImplementations.Length > 0)
                && !(o is IPropertySymbol op && op.ExplicitInterfaceImplementations.Length > 0));

        void EmiteConstructor(ConstructorDeclarationSyntax c)
        {
            var s = m.GetDeclaredSymbol(c);
            if (s.IsStatic) { Rechaza(c, Porque.H7(Porque.CtorEstatico)); return; }
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
            if (p.Modifiers.Any(SyntaxKind.NewKeyword) || OcultaPropiedad(s)) { Rechaza(p, Porque.H7(Porque.NoVirtuales)); return; }
            string nombre = Ident(s.Name);
            bool explicita = s.ExplicitInterfaceImplementations.Length > 0;
            if (explicita)
            {
                nombre = Ident(s.ExplicitInterfaceImplementations[0].Name);
                if (ChocaConOtro(s, s.ExplicitInterfaceImplementations[0].Name))
                {
                    Rechaza(p, Porque.Pendiente("una implementación explícita y otro miembro con su nombre"));
                    return;
                }
                if (ChocaConOtraExplicita(s)) { Rechaza(p, Porque.H7(Porque.ExplicitaDoble)); return; }
            }
            string cabecera = (explicita ? "public" : AccesoDe(s)) + (s.IsStatic ? " static" : "") + " "
                + Tipo(s.Type, p.Type) + " " + nombre;
            if (!s.IsStatic) Anotaciones(s);
            if (s.IsAbstract)
            {
                // una propiedad abstracta, o la de una interfaz: sus accesores lanzan
                Linea(cabecera);
                Linea("{");
                sangria++;
                foreach (var a in p.AccessorList.Accessors)
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
            var acc = p.AccessorList.Accessors;
            if (acc.All(a => a.Body == null && a.ExpressionBody == null))
            {
                // automática; `{ get; }` e `{ get; init; }` también: C# ya comprobó que nadie más la escribe
                Linea(cabecera + " { get; set; }" + (p.Initializer != null ? " = " + Expr(p.Initializer.Value) + ";" : ""));
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

        string Parametros(ParameterListSyntax pl)
        {
            var partes = new List<string>();
            foreach (var p in pl.Parameters)
            {
                var ps = m.GetDeclaredSymbol(p);
                if (ps == null) { partes.Add(Rechaza(p, Porque.Fuera(Porque.Interop))); continue; }     // __arglist
                if (ps.RefKind != RefKind.None) Rechaza(p, Porque.H7(Porque.RefOut));
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
                if (sinValor) Linea(ExprSentencia(flecha.Expression) + ";");
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
            nombresDeTipo[o] = n;
            return n;
        }

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
            // un enum (`Dia d = Dia.Martes`): JC sólo admite un literal, y un miembro de un enum no lo es
            if (ps.Type.TypeKind == TypeKind.Enum) return Rechaza(donde, Porque.H7(Porque.EnumPorDefecto));
            var ci = System.Globalization.CultureInfo.InvariantCulture;
            switch (v)
            {
                case null: return Rechaza(donde, EsCadena(ps.Type) ? Porque.H7(Porque.NullCadena) : Porque.H7(Porque.NullPorDefecto));
                case bool b: return b ? "true" : "false";
                case string s: return Cadena(s);
                case char ch: return ((int) ch).ToString(ci);           // un char, su código: en JC es un entero
                case int i: return i == int.MinValue ? "(-2147483647 - 1)" : i.ToString(ci);
                case long l: return l == long.MinValue ? "(-9223372036854775807L - 1L)" : l.ToString(ci) + "L";
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

        string NombreMetodo(IMethodSymbol s) => EsToString(s) ? "toString" : Ident(s.Name);

        /// `ToString` es el `toString` de BP (la ranura 0 de Object): quien lo sobrescribe en C# lo sobrescribe en JC.
        static bool EsToString(IMethodSymbol s)
        {
            for (var o = s; o != null; o = o.OverriddenMethod)
                if (o.Name == "ToString" && o.Parameters.Length == 0 && o.ContainingType.SpecialType == SpecialType.System_Object)
                    return true;
            return false;
        }

        /// ¿Un método que oculta otro de su base, en vez de sobrescribirlo? En BP todo método es virtual: sobrescribiría.
        static bool Oculta(IMethodSymbol s)
        {
            if (s.IsStatic || s.IsOverride) return false;
            for (var b = s.ContainingType.BaseType; b != null; b = b.BaseType)
                foreach (var o in b.GetMembers(s.Name).OfType<IMethodSymbol>())
                    if (!o.IsStatic && o.DeclaredAccessibility != Accessibility.Private && MismosParametros(o, s))
                        return true;
            return false;
        }

        static bool OcultaPropiedad(IPropertySymbol s)
        {
            if (s.IsStatic || s.IsOverride) return false;
            for (var b = s.ContainingType.BaseType; b != null; b = b.BaseType)
                if (b.GetMembers(s.Name).Any(o => !o.IsStatic && o.DeclaredAccessibility != Accessibility.Private))
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
