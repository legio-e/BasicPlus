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
        readonly HashSet<string> renombrados = new HashSet<string>();
        readonly HashSet<string> imports = new HashSet<string>();
        /// El sitio de cada clase en el módulo, para `#751` (no se construye una clase declarada más abajo).
        readonly Dictionary<ISymbol, int> orden = new Dictionary<ISymbol, int>(SymbolEqualityComparer.Default);

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
            // Los nombres: las anidadas, con el de su clase delante (A.B es A_B); dos que se llamen igual, no se traducen.
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
            for (int i = 0; i < ordenados.Count; i++) orden[ordenados[i]] = i;

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
                case TypeKind.Class:
                case TypeKind.Interface: EmiteClase(s); break;     // una interfaz simple es una clase base (#745)
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
                else if (EsExcepcionNet(b)) base_ = "Exception";
                else Rechaza(lugarBase, Porque.DeNet("hereda de " + NombreCompleto(b)));
            }
            // Las interfaces (#745, decidido por Eduardo): una del programa, en una clase sin otra base, es su clase base;
            // dos, o una y una clase, son herencia múltiple. IDisposable no cuenta: basta con su Dispose.
            var propias = s.Interfaces.Where(i => DelPrograma(i)).ToList();
            foreach (var i in s.Interfaces.Where(i => !DelPrograma(i) && NombreCompleto(i) != "System.IDisposable"))
                Rechaza(lugarBase, PorInterfazAjena(i));
            if (propias.Count > 1 || (propias.Count == 1 && base_ != null))
                Rechaza(lugarBase, Porque.Fuera(Porque.HerenciaMultiple));
            else if (propias.Count == 1)
                base_ = Tipo(propias[0], lugarBase);

            Linea((s.DeclaredAccessibility == Accessibility.Public ? "public " : "") + "class " + NombreTipo(s)
                  + (base_ != null ? " : " + base_ : ""));
            Linea("{");
            sangria++;
            foreach (var p in partes)
                foreach (var miembro in p.Members)
                    EmiteMiembro(miembro);
            // Una excepción sin constructor: el implícito de C# llama a Exception(), que en BP no existe (la de BP pide
            // su mensaje); se escribe con el mensaje que pondría .NET.
            if (EsExcepcionNet(b) && s.InstanceConstructors.All(c => c.IsImplicitlyDeclared))
                Linea("public function " + NombreTipo(s) + "() : base(" + Cadena(MensajePorDefecto(s)) + ") { }");
            sangria--;
            Linea("}");
            Linea("");
            claseActual = null;
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
            string acceso = Acceso(primero.DeclaredAccessibility);
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
                // la definición sin cuerpo: se escribe la implementación; sin ella, C# quita las llamadas
                if (s.PartialImplementationPart == null) Rechaza(me, Porque.Pendiente("métodos partial sin cuerpo"));
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
            }
            string cabecera = (esMain || explicita ? "public" : Acceso(s.DeclaredAccessibility)) + (s.IsStatic ? " static" : "")
                + " function " + Tipo(s.ReturnType, me.ReturnType) + " " + nombre
                + "(" + Parametros(me.ParameterList) + ")";
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

        /// ¿Tiene la clase otro miembro que se llame así (y que no sea una implementación explícita)?
        bool ChocaConOtro(ISymbol s, string nombre) =>
            claseActual.GetMembers(nombre).Any(o => !SymbolEqualityComparer.Default.Equals(o, s)
                && !(o is IMethodSymbol om && om.ExplicitInterfaceImplementations.Length > 0)
                && !(o is IPropertySymbol op && op.ExplicitInterfaceImplementations.Length > 0));

        void EmiteConstructor(ConstructorDeclarationSyntax c)
        {
            var s = m.GetDeclaredSymbol(c);
            if (s.IsStatic) { Rechaza(c, Porque.H7(Porque.CtorEstatico)); return; }
            string cabecera = Acceso(s.DeclaredAccessibility) + " function " + NombreTipo(claseActual)
                + "(" + Parametros(c.ParameterList) + ")";
            var ini = c.Initializer;
            var b = claseActual.BaseType;
            // `: base()` hacia object: vale (Eduardo, 31-oct: «Todas las clases tienen un padre salvo Object así que
            // base.metodo() debería ser válido»), y se copia; pero JC no lo compila (#753)
            if (ini != null && ini.IsKind(SyntaxKind.BaseConstructorInitializer) && b?.SpecialType == SpecialType.System_Object)
                Rechaza(ini, Porque.Fallo("#753", Porque.SuperObject));
            if (ini != null)
            {
                var objetivo = m.GetSymbolInfo(ini).Symbol as IMethodSymbol;
                if (ini.IsKind(SyntaxKind.BaseConstructorInitializer) && EsExcepcionNet(b))
                    cabecera += " : base(" + ArgsExcepcion(ini.ArgumentList, claseActual) + ")";
                else
                    cabecera += (ini.IsKind(SyntaxKind.BaseConstructorInitializer) ? " : base(" : " : this(")
                        + Argumentos(ini.ArgumentList, objetivo) + ")";
            }
            else if (EsExcepcionNet(b))
                cabecera += " : base(" + Cadena(MensajePorDefecto(claseActual)) + ")";
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
            }
            string cabecera = (explicita ? "public" : Acceso(s.DeclaredAccessibility)) + (s.IsStatic ? " static" : "") + " "
                + Tipo(s.Type, p.Type) + " " + nombre;
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
                partes.Add(Tipo(ps.Type, p.Type) + " " + Ident(ps.Name) + (p.Default != null ? " = " + ValorPorDefecto(ps, p) : ""));
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
            if (!ReservadasJc.Contains(nombre)) return nombre;
            if (renombrados.Add(nombre)) t.Avisos.Add("renombrado: " + nombre + " → " + nombre + "_ (palabra reservada de JC)");
            return nombre + "_";
        }

        string NombreTipo(INamedTypeSymbol s)
        {
            var o = s.OriginalDefinition;
            if (nombresDeTipo.TryGetValue(o, out var n)) return n;
            n = (o.ContainingType != null ? NombreTipo(o.ContainingType) + "_" : "") + Ident(o.Name);
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

        /// `#751`: una clase no se construye, ni se pregunta con `is`, ni se convierte a ella, ni se atrapa, más arriba
        /// de donde se declara en el módulo (el emisor revienta); la propia sí (#447).
        void CompruebaOrden(ITypeSymbol t, SyntaxNode donde)
        {
            if (claseActual == null || !(t is INamedTypeSymbol nt) || !DelPrograma(nt)) return;
            var o = nt.OriginalDefinition;
            if (SymbolEqualityComparer.Default.Equals(o, claseActual.OriginalDefinition)) return;
            if (orden.TryGetValue(o, out int i) && orden.TryGetValue(claseActual.OriginalDefinition, out int j) && i > j)
                Rechaza(donde, Porque.Fallo("#751", Porque.ClaseDespues));
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

        static bool EsExcepcionNet(ITypeSymbol t) => t != null && t.ToDisplayString() == "System.Exception";

        /// El mensaje que pone .NET a una excepción creada sin él: «Exception of type 'N.A+B' was thrown.»
        static string MensajePorDefecto(INamedTypeSymbol s) => "Exception of type '" + NombreNet(s) + "' was thrown.";

        static string NombreNet(INamedTypeSymbol s)
        {
            if (s.ContainingType != null) return NombreNet(s.ContainingType) + "+" + s.Name;
            var ns = s.ContainingNamespace;
            return ns == null || ns.IsGlobalNamespace ? s.Name : ns.ToDisplayString() + "." + s.Name;
        }

        /// Los argumentos de `: base(…)` hacia System.Exception: sin ninguno, el mensaje de .NET; con uno, ése.
        string ArgsExcepcion(ArgumentListSyntax args, INamedTypeSymbol clase)
        {
            if (args.Arguments.Count == 0) return Cadena(MensajePorDefecto(clase));
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
