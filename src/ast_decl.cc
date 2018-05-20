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
    idx = -1;
    expr_type = NULL;
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void VarDecl::PrintChildren(int indentLevel) {
   if (expr_type) std::cout << " <" << expr_type << ">";
   type->Print(indentLevel+1);
   id->Print(indentLevel+1);
   if (id->GetDecl()) printf(" ........ {def}");
}

void VarDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
}

void VarDecl::CheckDecl() {
    type->Check(E_CheckDecl);
    id->Check(E_CheckDecl);

    expr_type = type->GetType();
}

void VarDecl::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        default:
            type->Check(c);
            id->Check(c);
    }
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
    if (expr_type) std::cout << " <" << expr_type << ">";
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    if (extends) extends->Print(indentLevel+1, "(extends) ");
    implements->PrintAll(indentLevel+1, "(implements) ");
    members->PrintAll(indentLevel+1);
}

void ClassDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        // if two local symbols have the same name, then report an error.
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    // record the owner of the current class scope.
    symtab->BuildScope(this->GetId()->GetIdName());
    if (extends) {
        // record the parent of the current class.
        symtab->SetScopeParent(extends->GetId()->GetIdName());
    }
    // record the implements of the current class.
    for (int i = 0; i < implements->NumElements(); i++) {
        symtab->SetInterface(implements->Nth(i)->GetId()->GetIdName());
    }
    members->BuildSTAll();
    symtab->ExitScope();
}

void ClassDecl::CheckDecl() {
    id->Check(E_CheckDecl);
    // check extends.
    if (extends) {
        extends->Check(E_CheckDecl, LookingForClass);
    }
    // check interface.
    for (int i = 0; i < implements->NumElements(); i++) {
        implements->Nth(i)->Check(E_CheckDecl, LookingForInterface);
    }
    symtab->EnterScope();
    members->CheckAll(E_CheckDecl);
    symtab->ExitScope();

    expr_type = new NamedType(id);
    expr_type->SetSelfType();
}

void ClassDecl::CheckInherit() {
    symtab->EnterScope();

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        Assert(d != NULL); // already inserted into symbol table.

        if (d->IsVarDecl()) {
            // check class inheritance of variables.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                // subclass cannot override inherited variables.
                ReportError::DeclConflict(d, t);
            }
            // check interface inheritance of variables.
            t = symtab->LookupInterface(d->GetId());
            if (t != NULL) {
                // variable names conflict with interface method names.
                ReportError::DeclConflict(d, t);
            }

        } else if (d->IsFnDecl()) {
            // check class inheritance of functions.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                if (!t->IsFnDecl()) {
                    ReportError::DeclConflict(d, t);
                } else {
                    // compare the function signature.
                    FnDecl *fn1 = dynamic_cast<FnDecl*>(d);
                    FnDecl *fn2 = dynamic_cast<FnDecl*>(t);
                    if (fn1->GetType() && fn2->GetType() // correct return type
                            && !fn1->IsEquivalentTo(fn2)) {
                        // subclass cannot overload inherited functions.
                        ReportError::OverrideMismatch(d);
                    }
                }
            }
            // check interface inheritance of functions.
            t = symtab->LookupInterface(d->GetId());
            if (t != NULL) {
                // compare the function signature.
                FnDecl *fn1 = dynamic_cast<FnDecl*>(d);
                FnDecl *fn2 = dynamic_cast<FnDecl*>(t);
                if (fn1->GetType() && fn2->GetType() // correct return type
                        && !fn1->IsEquivalentTo(fn2)) {
                    // subclass cannot overload inherited functions.
                    ReportError::OverrideMismatch(d);
                }
            }
            // check scopes within the function and keep active scope correct.
            d->Check(E_CheckInherit);
        }
    }

    // Check the full implementation of interface methods.
    for (int i = 0; i < implements->NumElements(); i++) {
        Decl *d = implements->Nth(i)->GetId()->GetDecl();
        if (d != NULL) {
            List<Decl*> *m = dynamic_cast<InterfaceDecl*>(d)->GetMembers();
            // check the members of each interface.
            for (int j = 0; j < m->NumElements(); j++) {
                Identifier *mid = m->Nth(j)->GetId();
                Decl *t = symtab->LookupField(this->id, mid);
                if (t == NULL) {
                    ReportError::InterfaceNotImplemented(this,
                            implements->Nth(i));
                    break;
                } else {
                    // check the function signature.
                    FnDecl *fn1 = dynamic_cast<FnDecl*>(m->Nth(j));
                    FnDecl *fn2 = dynamic_cast<FnDecl*>(t);
                    if (!fn1 || !fn2 || !fn1->GetType() || !fn2->GetType()
                            || !fn1->IsEquivalentTo(fn2)) {
                        ReportError::InterfaceNotImplemented(this,
                                implements->Nth(i));
                        break;
                    }
                }
            }
        }
    }
    symtab->ExitScope();
}

void ClassDecl::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckInherit:
            this->CheckInherit(); break;
        default:
            id->Check(c);
            if (extends) extends->Check(c);
            implements->CheckAll(c);
            symtab->EnterScope();
            members->CheckAll(c);
            symtab->ExitScope();
    }
}

bool ClassDecl::IsChildOf(Decl *other) {
    if (other->IsClassDecl()) {
        if (id->IsEquivalentTo(other->GetId())) {
            // self.
            return true;
        } else if (!extends) {
            return false;
        } else {
            // look up all the parent classes.
            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl*>(d)->IsChildOf(other);
        }
    } else if (other->IsInterfaceDecl()) {
        for (int i = 0; i < implements->NumElements(); i++) {
            Identifier *iid = implements->Nth(i)->GetId();
            if (iid->IsEquivalentTo(other->GetId())) {
                return true;
            }
        }
        if (!extends) {
            return false;
        } else {
            // look up all the parent classes' interfaces.
            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl*>(d)->IsChildOf(other);
        }
    } else {
        return false;
    }
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    members->PrintAll(indentLevel+1);
}

void InterfaceDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    symtab->BuildScope(this->GetId()->GetIdName());
    members->BuildSTAll();
    symtab->ExitScope();
}

void InterfaceDecl::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            expr_type = new NamedType(id);
            expr_type->SetSelfType();
            // fall through.
        default:
            id->Check(c);
            symtab->EnterScope();
            members->CheckAll(c);
            symtab->ExitScope();
    }
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
    if (expr_type) std::cout << " <" << expr_type << ">";
    returnType->Print(indentLevel+1, "(return type) ");
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}

void FnDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    symtab->BuildScope();
    formals->BuildSTAll();
    if (body) body->BuildST(); // function body must be a StmtBlock.
    symtab->ExitScope();
}

void FnDecl::CheckDecl() {
    returnType->Check(E_CheckDecl);
    id->Check(E_CheckDecl);
    symtab->EnterScope();
    formals->CheckAll(E_CheckDecl);
    if (body) body->Check(E_CheckDecl);
    symtab->ExitScope();

    expr_type = returnType->GetType();
}

void FnDecl::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        default:
            returnType->Check(c);
            id->Check(c);
            symtab->EnterScope();
            formals->CheckAll(c);
            if (body) body->Check(c);
            symtab->ExitScope();
    }
}

bool FnDecl::IsEquivalentTo(Decl *other) {
    Assert(this->GetType() && other->GetType());

    if (!other->IsFnDecl()) {
        return false;
    }
    FnDecl *fn = dynamic_cast<FnDecl*>(other);
    if (!returnType->IsEquivalentTo(fn->GetType())) {
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

