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
    /* pp3: one pass on AST to build the symbol table. */
    symtab = new SymbolTable();
    decls->BuildSTAll();
    //symtab->Print();
}

void Program::Check() {
    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */
    symtab->ResetSymbolTable();
    decls->CheckAll();
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

void StmtBlock::Check() {
    symtab->EnterScope();
    decls->CheckAll();
    stmts->CheckAll();
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

void ForStmt::Check() {
    init->Check();
    test->Check();
    step->Check();
    symtab->EnterScope();
    body->Check();
    symtab->ExitScope();
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

void WhileStmt::Check() {
    test->Check();
    symtab->EnterScope();
    body->Check();
    symtab->ExitScope();
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

void IfStmt::Check() {
    test->Check();
    symtab->EnterScope();
    body->Check();
    symtab->ExitScope();
    if (elseBody) {
        symtab->EnterScope();
        elseBody->Check();
        symtab->ExitScope();
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

void CaseStmt::Check() {
    if (value) value->Check();
    symtab->EnterScope();
    stmts->CheckAll();
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

void SwitchStmt::Check() {
    expr->Check();
    symtab->EnterScope();
    cases->CheckAll();
    symtab->ExitScope();
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) {
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

void ReturnStmt::PrintChildren(int indentLevel) {
    expr->Print(indentLevel+1);
}

void ReturnStmt::Check() {
    expr->Check();
}

PrintStmt::PrintStmt(List<Expr*> *a) {
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::PrintChildren(int indentLevel) {
    args->PrintAll(indentLevel+1, "(args) ");
}

void PrintStmt::Check() {
    args->CheckAll();
}

