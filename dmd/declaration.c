
// Compiler implementation of the D programming language
// Copyright (c) 1999-2009 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <assert.h>

#include "init.h"
#include "declaration.h"
#include "attrib.h"
#include "mtype.h"
#include "template.h"
#include "scope.h"
#include "aggregate.h"
#include "module.h"
#include "id.h"
#include "expression.h"
#include "hdrgen.h"

/********************************* Declaration ****************************/

Declaration::Declaration(Identifier *id)
    : Dsymbol(id)
{
    type = NULL;
    originalType = NULL;
    storage_class = STCundefined;
    protection = PROTundefined;
    linkage = LINKdefault;
    inuse = 0;
}

void Declaration::semantic(Scope *sc)
{
}

const char *Declaration::kind()
{
    return "declaration";
}

unsigned Declaration::size(Loc loc)
{
    assert(type);
    return type->size();
}

int Declaration::isStaticConstructor()
{
    return FALSE;
}

int Declaration::isStaticDestructor()
{
    return FALSE;
}

int Declaration::isDelete()
{
    return FALSE;
}

int Declaration::isDataseg()
{
    return FALSE;
}

int Declaration::isThreadlocal()
{
    return FALSE;
}

int Declaration::isCodeseg()
{
    return FALSE;
}

enum PROT Declaration::prot()
{
    return protection;
}

/*************************************
 * Check to see if declaration can be modified in this context (sc).
 * Issue error if not.
 */

#if DMDV2
void Declaration::checkModify(Loc loc, Scope *sc, Type *t)
{
    if (sc->incontract && isParameter())
	error(loc, "cannot modify parameter '%s' in contract", toChars());

    if (isCtorinit())
    {	// It's only modifiable if inside the right constructor
	Dsymbol *s = sc->func;
	while (1)
	{
	    FuncDeclaration *fd = NULL;
	    if (s)
		fd = s->isFuncDeclaration();
	    if (fd &&
		((fd->isCtorDeclaration() && storage_class & STCfield) ||
		 (fd->isStaticCtorDeclaration() && !(storage_class & STCfield))) &&
		fd->toParent() == toParent()
	       )
	    {
		VarDeclaration *v = isVarDeclaration();
		assert(v);
		v->ctorinit = 1;
		//printf("setting ctorinit\n");
	    }
	    else
	    {
		if (s)
		{   s = s->toParent2();
		    continue;
		}
		else
		{
		    const char *p = isStatic() ? "static " : "";
		    error(loc, "can only initialize %sconst %s inside %sconstructor",
			p, toChars(), p);
		}
	    }
	    break;
	}
    }
    else
    {
	VarDeclaration *v = isVarDeclaration();
	if (v && v->canassign == 0)
	{
	    const char *p = NULL;
	    if (isConst())
		p = "const";
	    else if (isInvariant())
		p = "mutable";
	    else if (storage_class & STCmanifest)
		p = "enum";
	    else if (!t->isAssignable())
		p = "struct with immutable members";
	    if (p)
	    {	error(loc, "cannot modify %s", p);
	    }
	}
    }
}
#endif


/********************************* TupleDeclaration ****************************/

TupleDeclaration::TupleDeclaration(Loc loc, Identifier *id, Objects *objects)
    : Declaration(id)
{
    this->type = NULL;
    this->objects = objects;
    this->isexp = 0;
    this->tupletype = NULL;
}

Dsymbol *TupleDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);
    return NULL;
}

const char *TupleDeclaration::kind()
{
    return "tuple";
}

Type *TupleDeclaration::getType()
{
    /* If this tuple represents a type, return that type
     */

    //printf("TupleDeclaration::getType() %s\n", toChars());
    if (isexp)
	return NULL;
    if (!tupletype)
    {
	/* It's only a type tuple if all the Object's are types
	 */
	for (size_t i = 0; i < objects->dim; i++)
	{   Object *o = (Object *)objects->data[i];

	    if (o->dyncast() != DYNCAST_TYPE)
	    {
		//printf("\tnot[%d], %p, %d\n", i, o, o->dyncast());
		return NULL;
	    }
	}

	/* We know it's a type tuple, so build the TypeTuple
	 */
	Arguments *args = new Arguments();
	args->setDim(objects->dim);
	OutBuffer buf;
	for (size_t i = 0; i < objects->dim; i++)
	{   Type *t = (Type *)objects->data[i];

	    //printf("type = %s\n", t->toChars());
#if 0
	    buf.printf("_%s_%d", ident->toChars(), i);
	    char *name = (char *)buf.extractData();
	    Identifier *id = new Identifier(name, TOKidentifier);
	    Argument *arg = new Argument(STCin, t, id, NULL);
#else
	    Argument *arg = new Argument(0, t, NULL, NULL);
#endif
	    args->data[i] = (void *)arg;
	}

	tupletype = new TypeTuple(args);
    }

    return tupletype;
}

int TupleDeclaration::needThis()
{
    //printf("TupleDeclaration::needThis(%s)\n", toChars());
    for (size_t i = 0; i < objects->dim; i++)
    {   Object *o = (Object *)objects->data[i];
	if (o->dyncast() == DYNCAST_EXPRESSION)
	{   Expression *e = (Expression *)o;
	    if (e->op == TOKdsymbol)
	    {	DsymbolExp *ve = (DsymbolExp *)e;
		Declaration *d = ve->s->isDeclaration();
		if (d && d->needThis())
		{
		    return 1;
		}
	    }
	}
    }
    return 0;
}

/********************************* TypedefDeclaration ****************************/

TypedefDeclaration::TypedefDeclaration(Loc loc, Identifier *id, Type *basetype, Initializer *init)
    : Declaration(id)
{
    this->type = new TypeTypedef(this);
    this->basetype = basetype->toBasetype();
    this->init = init;
#ifdef _DH
    this->htype = NULL;
    this->hbasetype = NULL;
#endif
    this->sem = 0;
    this->loc = loc;
    this->sinit = NULL;
}

Dsymbol *TypedefDeclaration::syntaxCopy(Dsymbol *s)
{
    Type *basetype = this->basetype->syntaxCopy();

    Initializer *init = NULL;
    if (this->init)
	init = this->init->syntaxCopy();

    assert(!s);
    TypedefDeclaration *st;
    st = new TypedefDeclaration(loc, ident, basetype, init);
#ifdef _DH
    // Syntax copy for header file
    if (!htype)      // Don't overwrite original
    {	if (type)    // Make copy for both old and new instances
	{   htype = type->syntaxCopy();
	    st->htype = type->syntaxCopy();
	}
    }
    else            // Make copy of original for new instance
        st->htype = htype->syntaxCopy();
    if (!hbasetype)
    {	if (basetype)
	{   hbasetype = basetype->syntaxCopy();
	    st->hbasetype = basetype->syntaxCopy();
	}
    }
    else
        st->hbasetype = hbasetype->syntaxCopy();
#endif
    return st;
}

void TypedefDeclaration::semantic(Scope *sc)
{
    //printf("TypedefDeclaration::semantic(%s) sem = %d\n", toChars(), sem);
    if (sem == 0)
    {	sem = 1;
	basetype = basetype->semantic(loc, sc);
	sem = 2;
	type = type->semantic(loc, sc);
	if (sc->parent->isFuncDeclaration() && init)
	    semantic2(sc);
	storage_class |= sc->stc & STCdeprecated;
    }
    else if (sem == 1)
    {
	error("circular definition");
    }
}

void TypedefDeclaration::semantic2(Scope *sc)
{
    //printf("TypedefDeclaration::semantic2(%s) sem = %d\n", toChars(), sem);
    if (sem == 2)
    {	sem = 3;
	if (init)
	{
	    init = init->semantic(sc, basetype);

	    ExpInitializer *ie = init->isExpInitializer();
	    if (ie)
	    {
		if (ie->exp->type == basetype)
		    ie->exp->type = type;
	    }
	}
    }
}

const char *TypedefDeclaration::kind()
{
    return "typedef";
}

Type *TypedefDeclaration::getType()
{
    return type;
}

void TypedefDeclaration::toCBuffer(OutBuffer *buf, HdrGenState *hgs)
{
    buf->writestring("typedef ");
    basetype->toCBuffer(buf, ident, hgs);
    if (init)
    {
	buf->writestring(" = ");
	init->toCBuffer(buf, hgs);
    }
    buf->writeByte(';');
    buf->writenl();
}

/********************************* AliasDeclaration ****************************/

AliasDeclaration::AliasDeclaration(Loc loc, Identifier *id, Type *type)
    : Declaration(id)
{
    //printf("AliasDeclaration(id = '%s', type = %p)\n", id->toChars(), type);
    //printf("type = '%s'\n", type->toChars());
    this->loc = loc;
    this->type = type;
    this->aliassym = NULL;
#ifdef _DH
    this->htype = NULL;
    this->haliassym = NULL;
#endif
    this->overnext = NULL;
    this->inSemantic = 0;
    assert(type);
}

AliasDeclaration::AliasDeclaration(Loc loc, Identifier *id, Dsymbol *s)
    : Declaration(id)
{
    //printf("AliasDeclaration(id = '%s', s = %p)\n", id->toChars(), s);
    assert(s != this);
    this->loc = loc;
    this->type = NULL;
    this->aliassym = s;
#ifdef _DH
    this->htype = NULL;
    this->haliassym = NULL;
#endif
    this->overnext = NULL;
    this->inSemantic = 0;
    assert(s);
}

Dsymbol *AliasDeclaration::syntaxCopy(Dsymbol *s)
{
    //printf("AliasDeclaration::syntaxCopy()\n");
    assert(!s);
    AliasDeclaration *sa;
    if (type)
	sa = new AliasDeclaration(loc, ident, type->syntaxCopy());
    else
	sa = new AliasDeclaration(loc, ident, aliassym->syntaxCopy(NULL));
#ifdef _DH
    // Syntax copy for header file
    if (!htype)	    // Don't overwrite original
    {	if (type)	// Make copy for both old and new instances
	{   htype = type->syntaxCopy();
	    sa->htype = type->syntaxCopy();
	}
    }
    else			// Make copy of original for new instance
	sa->htype = htype->syntaxCopy();
    if (!haliassym)
    {	if (aliassym)
	{   haliassym = aliassym->syntaxCopy(s);
	    sa->haliassym = aliassym->syntaxCopy(s);
	}
    }
    else
	sa->haliassym = haliassym->syntaxCopy(s);
#endif
    return sa;
}

void AliasDeclaration::semantic(Scope *sc)
{
    //printf("AliasDeclaration::semantic() %s\n", toChars());
    if (aliassym)
    {
	if (aliassym->isTemplateInstance())
	    aliassym->semantic(sc);
	return;
    }
    this->inSemantic = 1;

    if (storage_class & STCconst)
	error("cannot be const");

    storage_class |= sc->stc & STCdeprecated;

    // Given:
    //	alias foo.bar.abc def;
    // it is not knowable from the syntax whether this is an alias
    // for a type or an alias for a symbol. It is up to the semantic()
    // pass to distinguish.
    // If it is a type, then type is set and getType() will return that
    // type. If it is a symbol, then aliassym is set and type is NULL -
    // toAlias() will return aliasssym.

    Dsymbol *s;
    Type *t;
    Expression *e;

    /* This section is needed because resolve() will:
     *   const x = 3;
     *   alias x y;
     * try to alias y to 3.
     */
    s = type->toDsymbol(sc);
    if (s && ((s->getType() && type->equals(s->getType())) || s->isEnumMember()))
	goto L2;			// it's a symbolic alias

#if DMDV2
    if (storage_class & STCref)
    {	// For 'ref' to be attached to function types, and picked
	// up by Type::resolve(), it has to go into sc.
	sc = sc->push();
	sc->stc |= STCref;
	type->resolve(loc, sc, &e, &t, &s);
	sc = sc->pop();
    }
    else
#endif
	type->resolve(loc, sc, &e, &t, &s);
    if (s)
    {
	goto L2;
    }
    else if (e)
    {
	// Try to convert Expression to Dsymbol
	s = getDsymbol(e);
	if (s)
	    goto L2;

	error("cannot alias an expression %s", e->toChars());
	t = e->type;
    }
    else if (t)
    {
	type = t;
    }
    if (overnext)
	ScopeDsymbol::multiplyDefined(0, this, overnext);
    this->inSemantic = 0;
    return;

  L2:
    //printf("alias is a symbol %s %s\n", s->kind(), s->toChars());
    type = NULL;
    VarDeclaration *v = s->isVarDeclaration();
    if (v && v->linkage == LINKdefault)
    {
	error("forward reference of %s", v->toChars());
	s = NULL;
    }
    else
    {
	FuncDeclaration *f = s->toAlias()->isFuncDeclaration();
	if (f)
	{
	    if (overnext)
	    {
		FuncAliasDeclaration *fa = new FuncAliasDeclaration(f);
		if (!fa->overloadInsert(overnext))
		    ScopeDsymbol::multiplyDefined(0, f, overnext);
		overnext = NULL;
		s = fa;
		s->parent = sc->parent;
	    }
	}
	if (overnext)
	    ScopeDsymbol::multiplyDefined(0, s, overnext);
	if (s == this)
	{
	    assert(global.errors);
	    s = NULL;
	}
    }
    //printf("setting aliassym %p to %p\n", this, s);
    aliassym = s;
    this->inSemantic = 0;
}

int AliasDeclaration::overloadInsert(Dsymbol *s)
{
    /* Don't know yet what the aliased symbol is, so assume it can
     * be overloaded and check later for correctness.
     */

    //printf("AliasDeclaration::overloadInsert('%s')\n", s->toChars());
    if (overnext == NULL)
    {	overnext = s;
	return TRUE;
    }
    else
    {
	return overnext->overloadInsert(s);
    }
}

const char *AliasDeclaration::kind()
{
    return "alias";
}

Type *AliasDeclaration::getType()
{
    return type;
}

Dsymbol *AliasDeclaration::toAlias()
{
    //printf("AliasDeclaration::toAlias('%s', this = %p, aliassym = %p, kind = '%s')\n", toChars(), this, aliassym, aliassym ? aliassym->kind() : "");
    assert(this != aliassym);
    //static int count; if (++count == 10) *(char*)0=0;
    if (inSemantic)
    {	error("recursive alias declaration");
	aliassym = new TypedefDeclaration(loc, ident, Type::terror, NULL);
    }
    Dsymbol *s = aliassym ? aliassym->toAlias() : this;
    return s;
}

void AliasDeclaration::toCBuffer(OutBuffer *buf, HdrGenState *hgs)
{
    buf->writestring("alias ");
#if 0 && _DH
    if (hgs->hdrgen)
    {
	if (haliassym)
	{
	    haliassym->toCBuffer(buf, hgs);
	    buf->writeByte(' ');
	    buf->writestring(ident->toChars());
	}
	else
	    htype->toCBuffer(buf, ident, hgs);
    }
    else
#endif
    {
	if (aliassym)
	{
	    aliassym->toCBuffer(buf, hgs);
	    buf->writeByte(' ');
	    buf->writestring(ident->toChars());
	}
	else
	    type->toCBuffer(buf, ident, hgs);
    }
    buf->writeByte(';');
    buf->writenl();
}

/********************************* VarDeclaration ****************************/

VarDeclaration::VarDeclaration(Loc loc, Type *type, Identifier *id, Initializer *init)
    : Declaration(id)
{
    //printf("VarDeclaration('%s')\n", id->toChars());
#ifdef DEBUG
    if (!type && !init)
    {	printf("VarDeclaration('%s')\n", id->toChars());
	//*(char*)0=0;
    }
#endif
    assert(type || init);
    this->type = type;
    this->init = init;
#ifdef _DH
    this->htype = NULL;
    this->hinit = NULL;
#endif
    this->loc = loc;
    offset = 0;
    noauto = 0;
#if DMDV1
    nestedref = 0;
#endif
    ctorinit = 0;
    aliassym = NULL;
    onstack = 0;
    canassign = 0;
    value = NULL;
}

Dsymbol *VarDeclaration::syntaxCopy(Dsymbol *s)
{
    //printf("VarDeclaration::syntaxCopy(%s)\n", toChars());

    VarDeclaration *sv;
    if (s)
    {	sv = (VarDeclaration *)s;
    }
    else
    {
	Initializer *init = NULL;
	if (this->init)
	{   init = this->init->syntaxCopy();
	    //init->isExpInitializer()->exp->print();
	    //init->isExpInitializer()->exp->dump(0);
	}

	sv = new VarDeclaration(loc, type ? type->syntaxCopy() : NULL, ident, init);
	sv->storage_class = storage_class;
    }
#ifdef _DH
    // Syntax copy for header file
    if (!htype)      // Don't overwrite original
    {	if (type)    // Make copy for both old and new instances
	{   htype = type->syntaxCopy();
	    sv->htype = type->syntaxCopy();
	}
    }
    else            // Make copy of original for new instance
        sv->htype = htype->syntaxCopy();
    if (!hinit)
    {	if (init)
	{   hinit = init->syntaxCopy();
	    sv->hinit = init->syntaxCopy();
	}
    }
    else
        sv->hinit = hinit->syntaxCopy();
#endif
    return sv;
}

void VarDeclaration::semantic(Scope *sc)
{
#if 0
    printf("VarDeclaration::semantic('%s', parent = '%s')\n", toChars(), sc->parent->toChars());
    printf(" type = %s\n", type ? type->toChars() : "null");
    printf(" stc = x%x\n", sc->stc);
    printf(" storage_class = x%x\n", storage_class);
    printf("linkage = %d\n", sc->linkage);
    //if (strcmp(toChars(), "mul") == 0) halt();
#endif

    storage_class |= sc->stc;
    if (storage_class & STCextern && init)
	error("extern symbols cannot have initializers");

    /* If auto type inference, do the inference
     */
    int inferred = 0;
    if (!type)
    {	inuse++;
	type = init->inferType(sc);
	inuse--;
	inferred = 1;

	/* This is a kludge to support the existing syntax for RAII
	 * declarations.
	 */
	storage_class &= ~STCauto;
	originalType = type;
    }
    else
    {	if (!originalType)
	    originalType = type;
	type = type->semantic(loc, sc);
    }
    //printf(" semantic type = %s\n", type ? type->toChars() : "null");

    type->checkDeprecated(loc, sc);
    linkage = sc->linkage;
    this->parent = sc->parent;
    //printf("this = %p, parent = %p, '%s'\n", this, parent, parent->toChars());
    protection = sc->protection;
    //printf("sc->stc = %x\n", sc->stc);
    //printf("storage_class = x%x\n", storage_class);

#if DMDV2
    if (storage_class & STCgshared && global.params.safe && !sc->module->safe)
    {
	error("__gshared not allowed in safe mode; use shared");
    }
#endif

    Dsymbol *parent = toParent();
    FuncDeclaration *fd = parent->isFuncDeclaration();

    Type *tb = type->toBasetype();
    if (tb->ty == Tvoid && !(storage_class & STClazy))
    {	error("voids have no value");
	type = Type::terror;
	tb = type;
    }
    if (tb->ty == Tfunction)
    {	error("cannot be declared to be a function");
	type = Type::terror;
	tb = type;
    }
    if (tb->ty == Tstruct)
    {	TypeStruct *ts = (TypeStruct *)tb;

	if (!ts->sym->members)
	{
	    error("no definition of struct %s", ts->toChars());
	}
    }

    if (tb->ty == Ttuple)
    {   /* Instead, declare variables for each of the tuple elements
	 * and add those.
	 */
	TypeTuple *tt = (TypeTuple *)tb;
	size_t nelems = Argument::dim(tt->arguments);
	Objects *exps = new Objects();
	exps->setDim(nelems);
	Expression *ie = init ? init->toExpression() : NULL;

	for (size_t i = 0; i < nelems; i++)
	{   Argument *arg = Argument::getNth(tt->arguments, i);

	    OutBuffer buf;
	    buf.printf("_%s_field_%zu", ident->toChars(), i);
	    buf.writeByte(0);
	    char *name = (char *)buf.extractData();
	    Identifier *id = new Identifier(name, TOKidentifier);

	    Expression *einit = ie;
	    if (ie && ie->op == TOKtuple)
	    {	einit = (Expression *)((TupleExp *)ie)->exps->data[i];
	    }
	    Initializer *ti = init;
	    if (einit)
	    {	ti = new ExpInitializer(einit->loc, einit);
	    }

	    VarDeclaration *v = new VarDeclaration(loc, arg->type, id, ti);
	    //printf("declaring field %s of type %s\n", v->toChars(), v->type->toChars());
	    v->semantic(sc);

	    if (sc->scopesym)
	    {	//printf("adding %s to %s\n", v->toChars(), sc->scopesym->toChars());
		if (sc->scopesym->members)
		    sc->scopesym->members->push(v);
	    }

	    Expression *e = new DsymbolExp(loc, v);
	    exps->data[i] = e;
	}
	TupleDeclaration *v2 = new TupleDeclaration(loc, ident, exps);
	v2->isexp = 1;
	aliassym = v2;
	return;
    }

Lagain:
    /* Storage class can modify the type
     */
    type = type->addStorageClass(storage_class);

    /* Adjust storage class to reflect type
     */
    if (type->isConst())
    {	storage_class |= STCconst;
	if (type->isShared())
	    storage_class |= STCshared;
    }
    else if (type->isInvariant())
	storage_class |= STCimmutable;
    else if (type->isShared())
	storage_class |= STCshared;

    if (isSynchronized())
    {
	error("variable %s cannot be synchronized", toChars());
    }
    else if (isOverride())
    {
	error("override cannot be applied to variable");
    }
    else if (isAbstract())
    {
	error("abstract cannot be applied to variable");
    }
    else if (storage_class & STCfinal)
    {
	error("final cannot be applied to variable");
    }

    if (storage_class & (STCstatic | STCextern | STCmanifest | STCtemplateparameter | STCtls | STCgshared))
    {
    }
    else
    {
	AggregateDeclaration *aad = sc->anonAgg;
	if (!aad)
	    aad = parent->isAggregateDeclaration();
	if (aad)
	{
#if DMDV2
	    assert(!(storage_class & (STCextern | STCstatic | STCtls | STCgshared)));

	    if (storage_class & (STCconst | STCimmutable) && init)
	    {
		if (!type->toBasetype()->isTypeBasic())
		    storage_class |= STCstatic;
	    }
	    else
#endif
		aad->addField(sc, this);
	}

	InterfaceDeclaration *id = parent->isInterfaceDeclaration();
	if (id)
	{
	    error("field not allowed in interface");
	}

	/* Templates cannot add fields to aggregates
	 */
	TemplateInstance *ti = parent->isTemplateInstance();
	if (ti)
	{
	    // Take care of nested templates
	    while (1)
	    {
		TemplateInstance *ti2 = ti->tempdecl->parent->isTemplateInstance();
		if (!ti2)
		    break;
		ti = ti2;
	    }

	    // If it's a member template
	    AggregateDeclaration *ad = ti->tempdecl->isMember();
	    if (ad && storage_class != STCundefined)
	    {
		error("cannot use template to add field to aggregate '%s'", ad->toChars());
	    }
	}
    }

#if DMDV2
    if ((storage_class & (STCref | STCparameter | STCforeach)) == STCref &&
	ident != Id::This)
    {
	error("only parameters or foreach declarations can be ref");
    }
#endif

    if (type->isauto() && !noauto)
    {
	if (storage_class & (STCfield | STCout | STCref | STCstatic | STCmanifest | STCtls | STCgshared) || !fd)
	{
	    error("globals, statics, fields, manifest constants, ref and out parameters cannot be scope");
	}

	if (!(storage_class & (STCauto | STCscope)))
	{
	    if (!(storage_class & STCparameter) && ident != Id::withSym)
		error("reference to scope class must be scope");
	}
    }

    if ((isConst() || isInvariant()) && !init && !fd)
    {	// Initialize by constructor only
	storage_class |= STCctorinit;
    }

    if (init)
	storage_class |= STCinit;     // remember we had an explicit initializer
    else if (storage_class & STCmanifest)
	error("manifest constants must have initializers");

    enum TOK op = TOKconstruct;
    if (!init && !sc->inunion && !isStatic() && fd &&
	(!(storage_class & (STCfield | STCin | STCforeach | STCparameter)) || (storage_class & STCout)) &&
	type->size() != 0)
    {
	// Provide a default initializer
	//printf("Providing default initializer for '%s'\n", toChars());
	if (type->ty == Tstruct &&
	    ((TypeStruct *)type)->sym->zeroInit == 1)
	{   /* If a struct is all zeros, as a special case
	     * set it's initializer to the integer 0.
	     * In AssignExp::toElem(), we check for this and issue
	     * a memset() to initialize the struct.
	     * Must do same check in interpreter.
	     */
	    Expression *e = new IntegerExp(loc, 0, Type::tint32);
	    Expression *e1;
	    e1 = new VarExp(loc, this);
	    e = new AssignExp(loc, e1, e);
#if DMDV2
	    e->op = TOKconstruct;
#endif
	    e->type = e1->type;		// don't type check this, it would fail
	    init = new ExpInitializer(loc, e);
	    return;
	}
	else if (type->ty == Ttypedef)
	{   TypeTypedef *td = (TypeTypedef *)type;
	    if (td->sym->init)
	    {	init = td->sym->init;
		ExpInitializer *ie = init->isExpInitializer();
		if (ie)
		    // Make copy so we can modify it
		    init = new ExpInitializer(ie->loc, ie->exp);
	    }
	    else
		init = getExpInitializer();
	}
	else
	{
	    init = getExpInitializer();
	}
#if DMDV2
	// Default initializer is always a blit
	op = TOKblit;
#endif
    }

    if (init)
    {
	sc = sc->push();
	sc->stc &= ~(STC_TYPECTOR | STCpure | STCnothrow | STCref);

	ArrayInitializer *ai = init->isArrayInitializer();
	if (ai && tb->ty == Taarray)
	{
	    init = ai->toAssocArrayInitializer();
	}

	StructInitializer *si = init->isStructInitializer();
	ExpInitializer *ei = init->isExpInitializer();

	// See if initializer is a NewExp that can be allocated on the stack
	if (ei && isScope() && ei->exp->op == TOKnew)
	{   NewExp *ne = (NewExp *)ei->exp;
	    if (!(ne->newargs && ne->newargs->dim))
	    {	ne->onstack = 1;
		onstack = 1;
		if (type->isBaseOf(ne->newtype->semantic(loc, sc), NULL))
		    onstack = 2;
	    }
	}

	// If inside function, there is no semantic3() call
	if (sc->func)
	{
	    // If local variable, use AssignExp to handle all the various
	    // possibilities.
	    if (fd &&
		!(storage_class & (STCmanifest | STCstatic | STCtls | STCgshared | STCextern)) &&
		!init->isVoidInitializer())
	    {
		//printf("fd = '%s', var = '%s'\n", fd->toChars(), toChars());
		if (!ei)
		{
		    Expression *e = init->toExpression();
		    if (!e)
		    {
			init = init->semantic(sc, type);
			e = init->toExpression();
			if (!e)
			{   error("is not a static and cannot have static initializer");
			    return;
			}
		    }
		    ei = new ExpInitializer(init->loc, e);
		    init = ei;
		}

		Expression *e1 = new VarExp(loc, this);

		Type *t = type->toBasetype();
		if (t->ty == Tsarray)
		{
		    ei->exp = ei->exp->semantic(sc);
		    if (!ei->exp->implicitConvTo(type))
		    {
			int dim = ((TypeSArray *)t)->dim->toInteger();
			// If multidimensional static array, treat as one large array
			while (1)
			{
			    t = t->nextOf()->toBasetype();
			    if (t->ty != Tsarray)
				break;
			    dim *= ((TypeSArray *)t)->dim->toInteger();
			    e1->type = new TypeSArray(t->nextOf(), new IntegerExp(0, dim, Type::tindex));
			}
		    }
		    e1 = new SliceExp(loc, e1, NULL, NULL);
		}
		else if (t->ty == Tstruct)
		{
		    ei->exp = ei->exp->semantic(sc);
#if DMDV2
		    /* Look to see if initializer is a call to the constructor
		     */
		    StructDeclaration *sd = ((TypeStruct *)t)->sym;
		    if (sd->ctor &&		// there are constructors
			ei->exp->type->ty == Tstruct &&	// rvalue is the same struct
			((TypeStruct *)ei->exp->type)->sym == sd &&
			ei->exp->op == TOKstar)
		    {
			/* Look for form of constructor call which is:
			 *    *__ctmp.ctor(arguments...)
			 */
			PtrExp *pe = (PtrExp *)ei->exp;
			if (pe->e1->op == TOKcall)
			{   CallExp *ce = (CallExp *)pe->e1;
			    if (ce->e1->op == TOKdotvar)
			    {	DotVarExp *dve = (DotVarExp *)ce->e1;
				if (dve->var->isCtorDeclaration())
				{   /* It's a constructor call, currently constructing
				     * a temporary __ctmp.
				     */
				    /* Before calling the constructor, initialize
				     * variable with a bit copy of the default
				     * initializer
				     */
				    Expression *e = new AssignExp(loc, new VarExp(loc, this), t->defaultInit(loc));
				    e->op = TOKblit;
				    e->type = t;
				    ei->exp = new CommaExp(loc, e, ei->exp);

				    /* Replace __ctmp being constructed with e1
				     */
				    dve->e1 = e1;
				    return;
				}
			    }
			}
		    }
#endif
		    if (!ei->exp->implicitConvTo(type))
		    {	Type *ti = ei->exp->type->toBasetype();
			// Don't cast away invariant or mutability in initializer
			if (!(ti->ty == Tstruct && t->toDsymbol(sc) == ti->toDsymbol(sc)))
			    ei->exp = new CastExp(loc, ei->exp, type);
		    }
		}
		ei->exp = new AssignExp(loc, e1, ei->exp);
		ei->exp->op = op;
		canassign++;
		ei->exp = ei->exp->semantic(sc);
		canassign--;
		ei->exp->optimize(WANTvalue);
	    }
	    else
	    {
		init = init->semantic(sc, type);
	    }
	}
	else if (storage_class & (STCconst | STCimmutable | STCmanifest) ||
		 type->isConst() || type->isInvariant())
	{
	    /* Because we may need the results of a const declaration in a
	     * subsequent type, such as an array dimension, before semantic2()
	     * gets ordinarily run, try to run semantic2() now.
	     * Ignore failure.
	     */

	    if (!global.errors && !inferred)
	    {
		unsigned errors = global.errors;
		global.gag++;
		//printf("+gag\n");
		Expression *e;
		Initializer *i2 = init;
		inuse++;
		if (ei)
		{
		    e = ei->exp->syntaxCopy();
		    e = e->semantic(sc);
		    e = e->implicitCastTo(sc, type);
		}
		else if (si || ai)
		{   i2 = init->syntaxCopy();
		    i2 = i2->semantic(sc, type);
		}
		inuse--;
		global.gag--;
		//printf("-gag\n");
		if (errors != global.errors)	// if errors happened
		{
		    if (global.gag == 0)
			global.errors = errors;	// act as if nothing happened
#if DMDV2
		    /* Save scope for later use, to try again
		     */
		    scope = new Scope(*sc);
		    scope->setNoFree();
#endif
		}
		else if (ei)
		{
		    if (isDataseg())
			/* static const/invariant does CTFE
			 */
			e = e->optimize(WANTvalue | WANTinterpret);
		    else
			e = e->optimize(WANTvalue);
		    if (e->op == TOKint64 || e->op == TOKstring)
		    {
			ei->exp = e;		// no errors, keep result
		    }
#if DMDV2
		    else
		    {
			/* Save scope for later use, to try again
			 */
			scope = new Scope(*sc);
			scope->setNoFree();
		    }
#endif
		}
		else
		    init = i2;		// no errors, keep result
	    }
	}
	sc = sc->pop();
    }
}

void VarDeclaration::semantic2(Scope *sc)
{
    //printf("VarDeclaration::semantic2('%s')\n", toChars());
    if (init && !toParent()->isFuncDeclaration())
    {	inuse++;
#if 0
	ExpInitializer *ei = init->isExpInitializer();
	if (ei)
	{
	    ei->exp->dump(0);
	    printf("type = %p\n", ei->exp->type);
	}
#endif
	init = init->semantic(sc, type);
	inuse--;
    }
}

const char *VarDeclaration::kind()
{
    return "variable";
}

Dsymbol *VarDeclaration::toAlias()
{
    //printf("VarDeclaration::toAlias('%s', this = %p, aliassym = %p)\n", toChars(), this, aliassym);
    assert(this != aliassym);
    Dsymbol *s = aliassym ? aliassym->toAlias() : this;
    return s;
}

void VarDeclaration::toCBuffer(OutBuffer *buf, HdrGenState *hgs)
{
    StorageClassDeclaration::stcToCBuffer(buf, storage_class);

    /* If changing, be sure and fix CompoundDeclarationStatement::toCBuffer()
     * too.
     */
    if (type)
	type->toCBuffer(buf, ident, hgs);
    else
	buf->writestring(ident->toChars());
    if (init)
    {	buf->writestring(" = ");
#if DMDV2
	ExpInitializer *ie = init->isExpInitializer();
	if (ie && (ie->exp->op == TOKconstruct || ie->exp->op == TOKblit))
	    ((AssignExp *)ie->exp)->e2->toCBuffer(buf, hgs);
	else
#endif
	    init->toCBuffer(buf, hgs);
    }
    buf->writeByte(';');
    buf->writenl();
}

int VarDeclaration::needThis()
{
    //printf("VarDeclaration::needThis(%s, x%x)\n", toChars(), storage_class);
    return storage_class & STCfield;
}

int VarDeclaration::isImportedSymbol()
{
    if (protection == PROTexport && !init &&
	(storage_class & STCstatic || parent->isModule()))
	return TRUE;
    return FALSE;
}

void VarDeclaration::checkCtorConstInit()
{
#if 0 /* doesn't work if more than one static ctor */
    if (ctorinit == 0 && isCtorinit() && !(storage_class & STCfield))
	error("missing initializer in static constructor for const variable");
#endif
}

/************************************
 * Check to see if this variable is actually in an enclosing function
 * rather than the current one.
 */

void VarDeclaration::checkNestedReference(Scope *sc, Loc loc)
{
    if (parent && !isDataseg() && parent != sc->parent &&
	!(storage_class & STCmanifest))
    {
	// The function that this variable is in
	FuncDeclaration *fdv = toParent()->isFuncDeclaration();
	// The current function
	FuncDeclaration *fdthis = sc->parent->isFuncDeclaration();

	if (fdv && fdthis && fdv != fdthis)
	{
	    if (loc.filename)
		fdthis->getLevel(loc, fdv);

	    for (int i = 0; i < nestedrefs.dim; i++)
	    {	FuncDeclaration *f = (FuncDeclaration *)nestedrefs.data[i];
		if (f == fdthis)
		    goto L1;
	    }
	    nestedrefs.push(fdthis);
	  L1: ;


	    for (int i = 0; i < fdv->closureVars.dim; i++)
	    {	Dsymbol *s = (Dsymbol *)fdv->closureVars.data[i];
		if (s == this)
		    goto L2;
	    }

	    fdv->closureVars.push(this);
	  L2: ;

	    //printf("fdthis is %s\n", fdthis->toChars());
	    //printf("var %s in function %s is nested ref\n", toChars(), fdv->toChars());
	}
    }
}

/****************************
 * Get ExpInitializer for a variable, if there is one.
 */

ExpInitializer *VarDeclaration::getExpInitializer()
{
    ExpInitializer *ei;

    if (init)
	ei = init->isExpInitializer();
    else
    {
	Expression *e = type->defaultInit(loc);
	if (e)
	    ei = new ExpInitializer(loc, e);
	else
	    ei = NULL;
    }
    return ei;
}

/*******************************************
 * If variable has a constant expression initializer, get it.
 * Otherwise, return NULL.
 */

Expression *VarDeclaration::getConstInitializer()
{
    if ((isConst() || isInvariant() || storage_class & STCmanifest) &&
	storage_class & STCinit)
    {
	ExpInitializer *ei = getExpInitializer();
	if (ei)
	    return ei->exp;
    }

    return NULL;
}

/*************************************
 * Return !=0 if we can take the address of this variable.
 */

int VarDeclaration::canTakeAddressOf()
{
#if 0
    /* Global variables and struct/class fields of the form:
     *	const int x = 3;
     * are not stored and hence cannot have their address taken.
     */
    if ((isConst() || isInvariant()) &&
	storage_class & STCinit &&
	(!(storage_class & (STCstatic | STCextern)) || (storage_class & STCfield)) &&
	(!parent || toParent()->isModule() || toParent()->isTemplateInstance()) &&
	type->toBasetype()->isTypeBasic()
       )
    {
	return 0;
    }
#else
    if (storage_class & STCmanifest)
	return 0;
#endif
    return 1;
}

/*******************************
 * Does symbol go into data segment?
 * Includes extern variables.
 */

int VarDeclaration::isDataseg()
{
#if 0
    printf("VarDeclaration::isDataseg(%p, '%s')\n", this, toChars());
    printf("%x, %p, %p\n", storage_class & (STCstatic | STCconst), parent->isModule(), parent->isTemplateInstance());
    printf("parent = '%s'\n", parent->toChars());
#endif
    if (storage_class & STCmanifest)
	return 0;
    Dsymbol *parent = this->toParent();
    if (!parent && !(storage_class & STCstatic))
    {	error("forward referenced");
	type = Type::terror;
	return 0;
    }
    return canTakeAddressOf() &&
	(storage_class & (STCstatic | STCextern | STCtls | STCgshared) ||
	 toParent()->isModule() ||
	 toParent()->isTemplateInstance());
}

/************************************
 * Does symbol go into thread local storage?
 */

int VarDeclaration::isThreadlocal()
{
    //printf("VarDeclaration::isThreadlocal(%p, '%s')\n", this, toChars());
#if 0 || TARGET_OSX
    /* To be thread-local, must use the __thread storage class.
     * BUG: OSX doesn't support thread local yet.
     */
    return isDataseg() &&
	(storage_class & (STCtls | STCconst | STCimmutable | STCshared | STCgshared)) == STCtls;
#else
    /* Data defaults to being thread-local. It is not thread-local
     * if it is immutable, const or shared.
     */
    int i = isDataseg() &&
	!(storage_class & (STCimmutable | STCconst | STCshared | STCgshared));
    //printf("\treturn %d\n", i);
    return i;
#endif
}

int VarDeclaration::hasPointers()
{
    //printf("VarDeclaration::hasPointers() %s, ty = %d\n", toChars(), type->ty);
    return (!isDataseg() && type->hasPointers());
}

/******************************************
 * Return TRUE if variable needs to call the destructor.
 */

int VarDeclaration::needsAutoDtor()
{
    //printf("VarDeclaration::needsAutoDtor() %s\n", toChars());

    if (noauto || storage_class & STCnodtor)
	return FALSE;

    // Destructors for structs and arrays of structs
    Type *tv = type->toBasetype();
    while (tv->ty == Tsarray)
    {   TypeSArray *ta = (TypeSArray *)tv;
	tv = tv->nextOf()->toBasetype();
    }
    if (tv->ty == Tstruct)
    {   TypeStruct *ts = (TypeStruct *)tv;
	StructDeclaration *sd = ts->sym;
	if (sd->dtor)
	    return TRUE;
    }

    // Destructors for classes
    if (storage_class & (STCauto | STCscope))
    {
	if (type->isClassHandle())
	    return TRUE;
    }
    return FALSE;
}


/******************************************
 * If a variable has an auto destructor call, return call for it.
 * Otherwise, return NULL.
 */

Expression *VarDeclaration::callAutoDtor(Scope *sc)
{   Expression *e = NULL;

    //printf("VarDeclaration::callAutoDtor() %s\n", toChars());

    if (noauto || storage_class & STCnodtor)
	return NULL;

    // Destructors for structs and arrays of structs
    bool array = false;
    Type *tv = type->toBasetype();
    while (tv->ty == Tsarray)
    {   TypeSArray *ta = (TypeSArray *)tv;
	array = true;
	tv = tv->nextOf()->toBasetype();
    }
    if (tv->ty == Tstruct)
    {   TypeStruct *ts = (TypeStruct *)tv;
	StructDeclaration *sd = ts->sym;
	if (sd->dtor)
	{
	    if (array)
	    {
		// Typeinfo.destroy(cast(void*)&v);
		Expression *ea = new SymOffExp(loc, this, 0, 0);
		ea = new CastExp(loc, ea, Type::tvoid->pointerTo());
		Expressions *args = new Expressions();
		args->push(ea);

		Expression *et = type->getTypeInfo(sc);
		et = new DotIdExp(loc, et, Id::destroy);

		e = new CallExp(loc, et, args);
	    }
	    else
	    {
		e = new VarExp(loc, this);
		e = new DotVarExp(loc, e, sd->dtor, 0);
		e = new CallExp(loc, e);
	    }
	    return e;
	}
    }

    // Destructors for classes
    if (storage_class & (STCauto | STCscope))
    {
	for (ClassDeclaration *cd = type->isClassHandle();
	     cd;
	     cd = cd->baseClass)
	{
	    /* We can do better if there's a way with onstack
	     * classes to determine if there's no way the monitor
	     * could be set.
	     */
	    //if (cd->isInterfaceDeclaration())
		//error("interface %s cannot be scope", cd->toChars());
	    if (1 || onstack || cd->dtors.dim)	// if any destructors
	    {
		// delete this;
		Expression *ec;

		ec = new VarExp(loc, this);
		e = new DeleteExp(loc, ec);
		e->type = Type::tvoid;
		break;
	    }
	}
    }
    return e;
}


/********************************* ClassInfoDeclaration ****************************/

ClassInfoDeclaration::ClassInfoDeclaration(ClassDeclaration *cd)
    : VarDeclaration(0, ClassDeclaration::classinfo->type, cd->ident, NULL)
{
    this->cd = cd;
    storage_class = STCstatic | STCgshared;
}

Dsymbol *ClassInfoDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}

void ClassInfoDeclaration::semantic(Scope *sc)
{
}

/********************************* ModuleInfoDeclaration ****************************/

ModuleInfoDeclaration::ModuleInfoDeclaration(Module *mod)
    : VarDeclaration(0, Module::moduleinfo->type, mod->ident, NULL)
{
    this->mod = mod;
    storage_class = STCstatic | STCgshared;
}

Dsymbol *ModuleInfoDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}

void ModuleInfoDeclaration::semantic(Scope *sc)
{
}

/********************************* TypeInfoDeclaration ****************************/

TypeInfoDeclaration::TypeInfoDeclaration(Type *tinfo, int internal)
    : VarDeclaration(0, Type::typeinfo->type, tinfo->getTypeInfoIdent(internal), NULL)
{
    this->tinfo = tinfo;
    storage_class = STCstatic | STCgshared;
    protection = PROTpublic;
    linkage = LINKc;
}

Dsymbol *TypeInfoDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}

void TypeInfoDeclaration::semantic(Scope *sc)
{
    assert(linkage == LINKc);
}

/***************************** TypeInfoConstDeclaration **********************/

#if DMDV2
TypeInfoConstDeclaration::TypeInfoConstDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}
#endif

/***************************** TypeInfoInvariantDeclaration **********************/

#if DMDV2
TypeInfoInvariantDeclaration::TypeInfoInvariantDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}
#endif

/***************************** TypeInfoSharedDeclaration **********************/

#if DMDV2
TypeInfoSharedDeclaration::TypeInfoSharedDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}
#endif

/***************************** TypeInfoStructDeclaration **********************/

TypeInfoStructDeclaration::TypeInfoStructDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoClassDeclaration ***********************/

TypeInfoClassDeclaration::TypeInfoClassDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoInterfaceDeclaration *******************/

TypeInfoInterfaceDeclaration::TypeInfoInterfaceDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoTypedefDeclaration *********************/

TypeInfoTypedefDeclaration::TypeInfoTypedefDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoPointerDeclaration *********************/

TypeInfoPointerDeclaration::TypeInfoPointerDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoArrayDeclaration ***********************/

TypeInfoArrayDeclaration::TypeInfoArrayDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoStaticArrayDeclaration *****************/

TypeInfoStaticArrayDeclaration::TypeInfoStaticArrayDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoAssociativeArrayDeclaration ************/

TypeInfoAssociativeArrayDeclaration::TypeInfoAssociativeArrayDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoEnumDeclaration ***********************/

TypeInfoEnumDeclaration::TypeInfoEnumDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoFunctionDeclaration ********************/

TypeInfoFunctionDeclaration::TypeInfoFunctionDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoDelegateDeclaration ********************/

TypeInfoDelegateDeclaration::TypeInfoDelegateDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/***************************** TypeInfoTupleDeclaration **********************/

TypeInfoTupleDeclaration::TypeInfoTupleDeclaration(Type *tinfo)
    : TypeInfoDeclaration(tinfo, 0)
{
}

/********************************* ThisDeclaration ****************************/

// For the "this" parameter to member functions

ThisDeclaration::ThisDeclaration(Loc loc, Type *t)
   : VarDeclaration(loc, t, Id::This, NULL)
{
    noauto = 1;
}

Dsymbol *ThisDeclaration::syntaxCopy(Dsymbol *s)
{
    assert(0);		// should never be produced by syntax
    return NULL;
}

