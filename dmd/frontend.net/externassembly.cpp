//
// $Id$
//
// Copyright (c) 2009 Cristian L. Vlasceanu
//
#include "frontend.net/externassembly.h"


bool operator<(const ExternAssembly& lhs, const ExternAssembly& rhs)
{
    if (lhs.name() < rhs.name())
    {
        return true;
    }
    return lhs.version() < rhs.version();
}
