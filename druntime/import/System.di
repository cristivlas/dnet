// 
// Copyright (c) 2008, 2009 Cristian L. Vlasceanu
//
// This file is by no means complete: it is just to boostrap the compiler.
// Eventually all import files for mscorlib and other assemblies will be
// generated automatically.
//
pragma (assembly, "mscorlib"){

public class __object;


public class String
{
public:
    this(wchar[]);
    this(wchar, int);
    int get_Length();

    static int Compare(String, String);
    static int Compare(String, String, bool ignoreCase);
}


public class Console
{
    static public void WriteLine();
    static public void WriteLine(Object);
    static public void WriteLine(String);
    static public void WriteLine(String, ...);
    //static public void WriteLine(String, Object);
    static public void WriteLine(wstring);

    //static public void WriteLine(char);
    static public void WriteLine(bool);
    static public void WriteLine(int);
    static public void WriteLine(uint);
    static public void WriteLine(long);
    static public void WriteLine(float);
    static public void WriteLine(double);

    //static public void Write(char);
    static public void Write(String);
    static public void Write(String, ...);

    static public void Write(bool);
    static public void Write(int);
    static public void Write(long);
    static public void Write(float);
    static public void Write(double);
    static public void Write(wstring);
}


public class GC
{
    static public void Collect();
    static public void WaitForPendingFinalizers();
}


public class Reflection
{
    public enum MemberTypes
    {
        Constructor = 0x01,
        Event = 0x02,
        Field = 0x04,
        Method = 0x08,
        Property = 0x10,
        TypeInfo = 0x20,
        Custom = 0x40,
        NestedType,
        All
    }

    public enum MethodAttributes
    {
        MemberAccessMask,
        PrivateScope,
        Private,
        FamANDAssem,
        Assembly,
        Family,
        FamORAssem,
        Public,
        Static,
        Final,
        Virtual,
        HideBySig,
        CheckAccessOnOverride,
        VtableLayoutMask,
        ReuseSlot,
        NewSlot,
        Abstract,
        SpecialName,
        PinvokeImpl,
        UnmanagedExport,
        RTSpecialName,
        ReservedMask,
        HasSecurity,
        RequireSecObject
    }


    public class Assembly
    {
    public:
        static Assembly LoadFile(String);
        static Assembly LoadFrom(String);

        Type[] GetExportedTypes();
        Type[] GetTypes();
        
        Module[] GetModules(bool resourceModules);
        Module[] GetModules();
    }


    public class Module
    {
    public:
        String ToString();	
    }


    public abstract class MemberInfo
    {
    public:
        String ToString();
        String get_Name();
        MemberTypes get_MemberType();
    }


    public class MethodBase : MemberInfo
    {
    }


    public class MethodInfo : MethodBase
    {
    public:
        MethodAttributes get_Attributes();
        bool get_IsPublic();
        Type get_ReturnType();
        Reflection.ParameterInfo get_ReturnParameter();
        Reflection.ParameterInfo[] GetParameters();
    }


    public class ParameterInfo
    {
    public:
        Type GetType();
        String ToString();
        String get_Name();
        Type get_ParameterType();
    }
}


public class Type
{
public:
    String ToString();
    String get_Name();
    String get_Namespace();
    Reflection.MemberInfo[] GetMembers();
    Reflection.MethodInfo[] GetMethods();
    Type[] GetInterfaces();
}
} // pragma (assembly)
