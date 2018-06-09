import System;

//-------------------------------------------------------------
static string[] assemblies;

struct Namespace
{
    System.Type types[];
}
static Namespace[System.String] namespaces;
//-------------------------------------------------------------


static void parseCommandLine(string[] args)
{
    foreach(a; args)
    {
        assemblies ~= a;
    }
}


static void collectTypesFromAssembly(string filename)
{
    Reflection.Assembly assembly = Reflection.Assembly.LoadFrom(filename.sys);

    Type[] types = assembly.GetExportedTypes();
    foreach (t; types)
    {
        if (! (t.get_Namespace() in namespaces))
        {
            namespaces[t.get_Namespace()] = Namespace();
        }
        namespaces[t.get_Namespace()].types ~= t;
    }
}


/**************************************************************
 *
 */
class TypeDumper
{
    static const int shiftWidth = 4;

    int     depth_; //identation depth
    String  ident_;

    // increase identation
    void incIdent(int inc)
    {
        depth_ += inc;
        ident_ = new String(' ', depth_);
    }


    void printParams(Reflection.MethodInfo m)
    {
        size_t count = 0;
        Reflection.ParameterInfo params[] = m.GetParameters();
        foreach(p; params)
        {
            if (count) 
            {
                Console.Write(", ");
            }
            Console.Write("{0} {1}".sys, p.get_ParameterType().ToString(), p.get_Name());
            ++count;
        }
        Console.WriteLine(");");
    }


    void print(Reflection.MethodInfo m)
    {
        incIdent(shiftWidth);
        scope(exit) incIdent(-shiftWidth);

        switch (m.get_MemberType())
        {
        case Reflection.MemberTypes.Constructor:
            Console.Write("{0}this(".sys, ident_);
            printParams(m);
            break;

        case Reflection.MemberTypes.Method:
            Console.Write("{0}{1}(".sys, ident_, m.get_Name());
            printParams(m);
            break;

        default:
            ;
        }
    }


    void print(System.Type type)
    {
        incIdent(shiftWidth);
        scope(exit) incIdent(-shiftWidth);

        Console.WriteLine("{0}public class {1} {{".sys, ident_, type.get_Name());

        foreach(m; type.GetMethods())
        {
            if (m.get_IsPublic())
            {
                print(m);
            }
        }
        Console.WriteLine("{0}}}".sys, ident_); // print the closing bracket
    }


    void print(String name, ref Namespace ns)
    {
        //namespaces are translated into classes
        Console.WriteLine("class {0} {{".sys, name);

        foreach(t; ns.types)
        {
            print(t);
        }
        Console.WriteLine("}\n");
    }
}


static TypeDumper dumper = new TypeDumper;


int main(string[] args)
{
    try
    {
        parseCommandLine(args);
        foreach(a; assemblies)
        {
            collectTypesFromAssembly(a);
        }

        foreach(name, ref ns; namespaces)
        {
            dumper.print(name, ns);
        }
    }
    catch (Exception e)
    {
        Console.WriteLine(e.toString().sys);
        return -1;
    }
    return 0;
}
