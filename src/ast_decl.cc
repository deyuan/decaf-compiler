/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "errors.h"


Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this);
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void VarDecl::PrintChildren(int indentLevel) {
   type->Print(indentLevel+1);
   id->Print(indentLevel+1);
}

void VarDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        symtab->InsertSymbol(this);
    }
}

void VarDecl::Check() {
    type->Check();
    id->Check();
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

void ClassDecl::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
    if (extends) extends->Print(indentLevel+1, "(extends) ");
    implements->PrintAll(indentLevel+1, "(implements) ");
    members->PrintAll(indentLevel+1);
}

void ClassDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        symtab->InsertSymbol(this);
    }
    // set the owner of the sub scope.
    symtab->BuildScope(this->GetId()->GetIdName());
    if (extends) {
        // set parent of the current class.
        symtab->SetScopeParent(extends->GetId()->GetIdName());
    }
    // need to deal with implements.
    for (int i = 0; i < implements->NumElements(); i++) {
        symtab->SetInterface(implements->Nth(i)->GetId()->GetIdName());
    }

    members->BuildSTAll();
    symtab->ExitScope();
}

void ClassDecl::Check() {
    id->Check();

    // Check extends.
    if (extends) {
        Decl *d = symtab->Lookup(extends->GetId());
        if (d == NULL || !d->IsClassDecl()) {
            ReportError::IdentifierNotDeclared(extends->GetId(),
                    LookingForClass);
        }
    }
    // Check interface.
    for (int i = 0; i < implements->NumElements(); i++) {
        Decl *d = symtab->Lookup(implements->Nth(i)->GetId());
        if (d == NULL || !d->IsInterfaceDecl()) {
            ReportError::IdentifierNotDeclared(implements->Nth(i)->GetId(),
                    LookingForInterface);
        }
    }

    symtab->EnterScope();

    // Check Interface.
    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        Assert(d != NULL); // members must be all inserted into symbol table.

        // Check Interface error.
        Decl *t = symtab->LookupInterface(d->GetId());
        if (t != NULL) {
            if (d->IsFnDecl()) {
                // compare the function signature.
                FnDecl *fn1 = dynamic_cast<FnDecl*>(d);
                FnDecl *fn2 = dynamic_cast<FnDecl*>(t);
                if (!fn1->IsEquivalentTo(fn2)) {
                    ReportError::OverrideMismatch(d);
                }
            } else {
                // some other types conflict with interface method names.
                ReportError::DeclConflict(d, t);
            }
        }
    }

    // Check class inheritance.
    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        Assert(d != NULL); // members must be all inserted into symbol table.

        //printf("Check: '%s'\n", d->GetPrintNameForNode());
        if (d->IsVarDecl()) {

            // Check ID override error.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                ReportError::DeclConflict(d, t);
            }
            // Check the type and id for the VarDecl.
            d->Check();

        } else if (d->IsFnDecl()) {

            // Check Function overload error.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                // compare the function signature.
                if (!t->IsFnDecl()) {
                    ReportError::DeclConflict(d, t);
                } else {
                    FnDecl *fn1 = dynamic_cast<FnDecl*>(d);
                    FnDecl *fn2 = dynamic_cast<FnDecl*>(t);
                    if (!fn1->IsEquivalentTo(fn2)) {
                        ReportError::OverrideMismatch(d);
                    }
                }
            }

            // Check the scopes within the function. keep scopes right.
            d->Check();

        } else {
            Assert(false);
        }
    }
    symtab->ExitScope();
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
    members->PrintAll(indentLevel+1);
}

void InterfaceDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        symtab->InsertSymbol(this);
    }
    symtab->BuildScope(this->GetId()->GetIdName());
    members->BuildSTAll();
    symtab->ExitScope();
}

void InterfaceDecl::Check() {
    id->Check();
    symtab->EnterScope();
    members->CheckAll();
    symtab->ExitScope();
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) {
    (body=b)->SetParent(this);
}

void FnDecl::PrintChildren(int indentLevel) {
    returnType->Print(indentLevel+1, "(return type) ");
    id->Print(indentLevel+1);
    formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}

void FnDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        symtab->InsertSymbol(this);
    }
    symtab->BuildScope();
    formals->BuildSTAll();
    if (body) body->BuildST(); // function body must be a StmtBlock.
    symtab->ExitScope();
}

void FnDecl::Check() {
    returnType->Check();
    id->Check();
    symtab->EnterScope();
    formals->CheckAll();
    if (body) body->Check();
    symtab->ExitScope();
}

bool FnDecl::IsEquivalentTo(Decl *other) {
    if (!other->IsFnDecl()) {
        return false;
    }
    FnDecl *fn = dynamic_cast<FnDecl*>(other);
    if (!returnType->IsEquivalentTo(fn->GetReturnType())) {
        return false;
    }
    if (formals->NumElements() != fn->GetFormals()->NumElements()) {
        return false;
    }
    for (int i = 0; i < formals->NumElements(); i++) {
        // must be VarDecls.
        Type *var_type1 =
            (dynamic_cast<VarDecl*>(formals->Nth(i)))->GetType();
        Type *var_type2 =
            (dynamic_cast<VarDecl*>(fn->GetFormals()->Nth(i)))->GetType();
        if (!var_type1->IsEquivalentTo(var_type2)) {
            return false;
        }
    }
    return true;
}

