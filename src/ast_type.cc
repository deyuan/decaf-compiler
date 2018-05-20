/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
#include "errors.h"
#include "string.h"


/* Class constants
 * ---------------
 * These are public constants for the built-in base types (int, double, etc.)
 * They can be accessed with the syntax Type::intType. This allows you to
 * directly access them and share the built-in types where needed rather that
 * creates lots of copies.
 */

Type *Type::intType    = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType   = new Type("void");
Type *Type::boolType   = new Type("bool");
Type *Type::nullType   = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType  = new Type("error");

Type::Type(const char *n) {
    Assert(n);
    typeName = strdup(n);
}

void Type::PrintChildren(int indentLevel) {
    printf("%s", typeName);
}

void Type::Check() {
}

bool Type::IsEquivalentTo(Type *other) {
    if (!other->IsBasicType()) {
        return false;
    }
    if (strcmp(typeName, other->GetTypeName())) {
        return false;
    }
    return true;
}

NamedType::NamedType(Identifier *i) : Type(*i->GetLocation()) {
    Assert(i != NULL);
    (id=i)->SetParent(this);
}

void NamedType::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
}

void NamedType::Check() {
    Decl *d = symtab->Lookup(this->GetId());
    if (d == NULL || (!d->IsClassDecl() && !d->IsInterfaceDecl())) {
        ReportError::IdentifierNotDeclared(this->GetId(), LookingForType);
    }
}

bool NamedType::IsEquivalentTo(Type *other) {
    if (!other->IsNamedType()) {
        return false;
    }
    NamedType * nt = dynamic_cast<NamedType*>(other);
    return (id->IsEquivalentTo(nt->GetId()));
}

ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}
void ArrayType::PrintChildren(int indentLevel) {
    elemType->Print(indentLevel+1);
}

void ArrayType::Check() {
    elemType->Check();
}

bool ArrayType::IsEquivalentTo(Type *other) {
    if (!other->IsArrayType()) {
        return false;
    }
    ArrayType * nt = dynamic_cast<ArrayType*>(other);
    return (elemType->IsEquivalentTo(nt->GetElemType()));
}

