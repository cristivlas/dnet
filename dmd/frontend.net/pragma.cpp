//
// $Id$
// Copyright (c) 2009 Cristian L. Vlasceanu
//
#include <cassert>
#include <string>
#include "attrib.h"
#include "expression.h"
#include "lexer.h"
#include "module.h"
#include "backend.net/array.h"
#include "backend.net/backend.h"
#include "backend.net/utils.h"
#include "frontend.net/externassembly.h"
#include "frontend.net/pragma.h"

using namespace std;


PragmaScope::PragmaScope(PragmaDeclaration* decl, Dsymbol* parent, StringExp* exp)
    : module(NULL)
{
    this->loc = decl->loc;
    members = decl->decl;

    const size_t len = exp->length();
    string s(len, ' ');
    for (size_t i = 0; i != len; ++i)
    {
        s[i] = exp->charAt(i);
    }
    if (decl->ident == Lexer::idPool("assembly"))
    {
        whatKind = pragma_assembly;
        if (!parent || !parent->isModule())
        {
            error(loc, "nested assembly pragma not allowed");
        }
        BackEnd::instance().addExtern(ExternAssembly(s));
        assert(!c_ident);
        c_ident = Lexer::idPool(s.c_str());

        module = parent->isModule();

        string tmp;
        for (Dsymbol* sym = parent; sym; sym = sym->parent)
        {
            if (!tmp.empty())
                tmp = '.' + tmp;
            tmp = sym->toChars() + tmp;
        }
        s = "[" + s + "]" + tmp;
    }
    ident = Lexer::idPool(s.c_str());
}


void PragmaScope::toObjFile(int multiobj)
{
}


void PragmaScope::setScope(Scope* sc)
{
    ArrayAdapter<Dsymbol*> syms(members);
    for (ArrayAdapter<Dsymbol*>::iterator i = syms.begin(); i != syms.end(); ++i)
    {
        DEREF(*i).setScope(sc);
    }
}


static void semantic(PragmaScope* pragma, Scope* sc, Array* dsymbols)
{
    ArrayAdapter<Dsymbol*> syms(dsymbols);

    for (ArrayAdapter<Dsymbol*>::iterator i = syms.begin(); i != syms.end(); ++i)
    {
        Dsymbol& dsym = DEREF(*i);
        dsym.parent = pragma;
        if (AttribDeclaration* attr = dsym.isAttribDeclaration())
        {
            semantic(pragma, sc, attr->decl);
        }
        else 
        {
            dsym.semantic(sc);
        }
    }
}


void PragmaScope::semantic(Scope* sc)
{
    ::semantic(this, sc, members);
}


PragmaScope* inPragmaAssembly(Dsymbol* sym)
{
    PragmaScope* pragmaScope = NULL;
    if (sym)
    {
        pragmaScope = sym->isPragmaScope();
        if (!pragmaScope)
        {
            return inPragmaAssembly(sym->toParent());
        }
        if (pragmaScope->kind() != PragmaScope::pragma_assembly)
        {
            pragmaScope = NULL;
        }
    }
    return pragmaScope;
}
