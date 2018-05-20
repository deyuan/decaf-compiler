/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    printf("\n");
}

void Program::BuildST() {
    if (IsDebugOn("ast")) { this->Print(0); }
    symtab = new SymbolTable();

    /* Pass 1: Traverse the AST and build the symbol table. Report the
     * errors of declaration conflict in any local scopes. */
    decls->BuildSTAll();
    if (IsDebugOn("st")) { symtab->Print(); }
    PrintDebug("ast+", "BuildST finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }
}

void Program::Check() {
    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */

    /* Pass 2: Traverse the AST and report any errors of undeclared
     * identifiers except the field access and function calls. */
    symtab->ResetSymbolTable();
    decls->CheckAll(E_CheckDecl);
    PrintDebug("ast+", "CheckDecl finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }

    /* Pass 3: Traverse the AST and report errors related to the class and
     * interface inheritance. */
    symtab->ResetSymbolTable();
    decls->CheckAll(E_CheckInherit);
    PrintDebug("ast+", "CheckInherit finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }

    /* Pass 4: Traverse the AST and report errors related to types, function
     * calls and field access. Actually, check all the remaining errors. */
    symtab->ResetSymbolTable();
    decls->CheckAll(E_CheckType);
    PrintDebug("ast+", "CheckType finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

void StmtBlock::BuildST() {
    symtab->BuildScope();
    decls->BuildSTAll();
    stmts->BuildSTAll();
    symtab->ExitScope();
}

void StmtBlock::Check(checkT c) {
    symtab->EnterScope();
    decls->CheckAll(c);
    stmts->CheckAll(c);
    symtab->ExitScope();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) {
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this);
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) {
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void ForStmt::PrintChildren(int indentLevel) {
    init->Print(indentLevel+1, "(init) ");
    test->Print(indentLevel+1, "(test) ");
    step->Print(indentLevel+1, "(step) ");
    body->Print(indentLevel+1, "(body) ");
}

void ForStmt::BuildST() {
    symtab->BuildScope();
    body->BuildST();
    symtab->ExitScope();
}

void ForStmt::CheckType() {
    init->Check(E_CheckType);
    test->Check(E_CheckType);
    if (test->GetType() && test->GetType() != Type::boolType) {
        ReportError::TestNotBoolean(test);
    }
    step->Check(E_CheckType);
    symtab->EnterScope();
    body->Check(E_CheckType);
    symtab->ExitScope();
}

void ForStmt::Check(checkT c) {
    switch (c) {
        case E_CheckType:
            this->CheckType(); break;
        default:
            init->Check(c);
            test->Check(c);
            step->Check(c);
            symtab->EnterScope();
            body->Check(c);
            symtab->ExitScope();
    }
}

void WhileStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(body) ");
}

void WhileStmt::BuildST() {
    symtab->BuildScope();
    body->BuildST();
    symtab->ExitScope();
}

void WhileStmt::CheckType() {
    test->Check(E_CheckType);
    if (test->GetType() && test->GetType() != Type::boolType) {
        ReportError::TestNotBoolean(test);
    }
    symtab->EnterScope();
    body->Check(E_CheckType);
    symtab->ExitScope();
}

void WhileStmt::Check(checkT c) {
    switch (c) {
        case E_CheckType:
            this->CheckType(); break;
        default:
            test->Check(c);
            symtab->EnterScope();
            body->Check(c);
            symtab->ExitScope();
    }
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) {
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(then) ");
    if (elseBody) elseBody->Print(indentLevel+1, "(else) ");
}

void IfStmt::BuildST() {
    symtab->BuildScope();
    body->BuildST();
    symtab->ExitScope();
    if (elseBody) {
        symtab->BuildScope();
        elseBody->BuildST();
        symtab->ExitScope();
    }
}

void IfStmt::CheckType() {
    test->Check(E_CheckType);
    if (test->GetType() && test->GetType() != Type::boolType) {
        ReportError::TestNotBoolean(test);
    }
    symtab->EnterScope();
    body->Check(E_CheckType);
    symtab->ExitScope();
    if (elseBody) {
        symtab->EnterScope();
        elseBody->Check(E_CheckType);
        symtab->ExitScope();
    }
}

void IfStmt::Check(checkT c) {
    switch (c) {
        case E_CheckType:
            this->CheckType(); break;
        default:
            test->Check(c);
            symtab->EnterScope();
            body->Check(c);
            symtab->ExitScope();
            if (elseBody) {
                symtab->EnterScope();
                elseBody->Check(c);
                symtab->ExitScope();
            }
    }
}

void BreakStmt::Check(checkT c) {
    if (c == E_CheckType) {
        Node *n = this;
        while (n->GetParent()) {
            if (n->IsLoopStmt() || n->IsCaseStmt()) return;
            n = n->GetParent();
        }
        ReportError::BreakOutsideLoop(this);
    }
}

CaseStmt::CaseStmt(IntConstant *v, List<Stmt*> *s) {
    Assert(s != NULL);
    value = v;
    if (value) value->SetParent(this);
    (stmts=s)->SetParentAll(this);
}

void CaseStmt::PrintChildren(int indentLevel) {
    if (value) value->Print(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

void CaseStmt::BuildST() {
    symtab->BuildScope();
    stmts->BuildSTAll();
    symtab->ExitScope();
}

void CaseStmt::Check(checkT c) {
    if (value) value->Check(c);
    symtab->EnterScope();
    stmts->CheckAll(c);
    symtab->ExitScope();
}

SwitchStmt::SwitchStmt(Expr *e, List<CaseStmt*> *c) {
    Assert(e != NULL && c != NULL);
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
}

void SwitchStmt::PrintChildren(int indentLevel) {
    expr->Print(indentLevel+1);
    cases->PrintAll(indentLevel+1);
}

void SwitchStmt::BuildST() {
    symtab->BuildScope();
    cases->BuildSTAll();
    symtab->ExitScope();
}

void SwitchStmt::Check(checkT c) {
    expr->Check(c);
    symtab->EnterScope();
    cases->CheckAll(c);
    symtab->ExitScope();
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) {
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

void ReturnStmt::PrintChildren(int indentLevel) {
    expr->Print(indentLevel+1);
}

void ReturnStmt::Check(checkT c) {
    expr->Check(c);
    if (c == E_CheckType) {
        Node *n = this;
        // find the FnDecl.
        while (n->GetParent()) {
            if (dynamic_cast<FnDecl*>(n) != NULL) break;
            n = n->GetParent();
        }
        Type *t_given = expr->GetType();
        Type *t_expected = dynamic_cast<FnDecl*>(n)->GetType();
        if (t_given && t_expected) {
            if (!t_expected->IsCompatibleWith(t_given)) {
                ReportError::ReturnMismatch(this, t_given, t_expected);
            }
        }
    }
}

PrintStmt::PrintStmt(List<Expr*> *a) {
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::PrintChildren(int indentLevel) {
    args->PrintAll(indentLevel+1, "(args) ");
}

void PrintStmt::Check(checkT c) {
    args->CheckAll(c);
    if (c == E_CheckType) {
        for (int i = 0; i < args->NumElements(); i++) {
            Type *t = args->Nth(i)->GetType();
            if (t != NULL && t != Type::stringType && t != Type::intType
                     && t != Type::boolType) {
                ReportError::PrintArgMismatch(args->Nth(i), i + 1, t);
            }
        }
    }
}

