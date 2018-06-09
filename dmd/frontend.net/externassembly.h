#pragma once
//
// $Id$
//
// Copyright (c) 2009 Cristian L. Vlasceanu
//
#include <cassert>
#include <string>

class ExternAssembly
{
    std::string name_;
    std::string version_;

public:
    explicit ExternAssembly(const std::string& name) : name_(name)
    {
    }
    explicit ExternAssembly(const char* name)
    {
        assert(name);
        name_.assign(name);
    }
    const std::string& name() const
    {
        return name_;
    }
    void setVersion(const std::string& ver)
    {
        version_ = ver;
    }
    const std::string& version() const
    {
        return version_;
    }
};


bool operator<(const ExternAssembly&, const ExternAssembly&);

