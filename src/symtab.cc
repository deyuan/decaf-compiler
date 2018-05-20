/* File: symtab.cc
 * ---------------
 * Build the symbol table.
 * Author: Deyuan Guo
 */

#include <iostream>
#include "string.h"

#include "symtab.h"
#include "ast.h"
#include "ast_decl.h"


/* Global Symbol Table, used by all nodes.
 */
SymbolTable *symtab;


/* Scope class, maintain the necessary information of scope structure.
 */
class Scope {
  protected:
    Hashtable<Decl*> *ht;
    const char *parent;                 // record the class inheritance
    std::list<const char *> *interface; // record the interface of class
    const char *owner;                  // record the scope owner for class

  public:
    Scope() {
        ht = new Hashtable<Decl*>;
        parent = NULL;
        interface = new std::list<const char *>;
        interface->clear();
        owner = NULL;
    }

    Hashtable<Decl*> * GetHT() { return ht; }

    bool HasParent() { return parent == NULL ? false : true; }
    void SetParent(const char *p) { parent = p; }
    const char * GetParent() { return parent; }

    bool HasInterface() { return !interface->empty(); }
    void AddInterface(const char *p) { interface->push_back(p); }
    std::list<const char *> * GetInterface() { return interface; }

    bool HasOwner() { return owner == NULL ? false : true; }
    void SetOwner(const char *o) { owner = o; }
    const char * GetOwner() { return owner; }
};

/* Implementation of Symbol Table
 */
SymbolTable::SymbolTable()
{
    /* Init the global scope. */
    scopes = new std::vector<Scope *>;
    scopes->clear();
    scopes->push_back(new Scope());

    /* Init the active scopes, and active global scope 0. */
    activeScopes = new std::vector<int>;
    activeScopes->clear();
    activeScopes->push_back(0);

    /* Init scope counter and identifier counter. */
    cur_scope = 0;
    scope_cnt = 0;
    id_cnt = 0;
}

/*
 * Resert symbol table counter and active scopes for another pass.
 */
void
SymbolTable::ResetSymbolTable() {
    activeScopes->clear();
    activeScopes->push_back(0);

    cur_scope = 0;
    scope_cnt = 0;
    id_cnt = 0;
}

/*
 * Enter a new scope.
 */
void
SymbolTable::BuildScope()
{
    scope_cnt++;
    scopes->push_back(new Scope());
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}

/*
 * Enter a new scope, and set owner for class and interface.
 */
void
SymbolTable::BuildScope(const char *key)
{
    scope_cnt++;
    scopes->push_back(new Scope());
    scopes->at(scope_cnt)->SetOwner(key);
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}

/*
 * Enter a new scope.
 */
void
SymbolTable::EnterScope()
{
    scope_cnt++;
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}

/*
 * Find scope from owner name.
 */
int
SymbolTable::FindScopeFromOwnerName(const char *key)
{
    int scope = -1;

    for (int i = 0; i < scopes->size(); i++) {
        if (scopes->at(i)->HasOwner()) {
            if (!strcmp(key, scopes->at(i)->GetOwner())) {
                scope = i;;
                break;
            }
        }
    }

    return scope;
}

/*
 * Look up symbol in all active scopes.
 */
Decl *
SymbolTable::Lookup(Identifier *id)
{
    Decl *d = NULL;
    const char *parent = NULL;
    const char *key = id->GetIdName();

    //printf("Look up %s from scope %d\n", key, cur_scope);

    // Look up all the active scopes.
    for (int i = activeScopes->size(); i > 0; --i) {

        int scope = activeScopes->at(i-1);

        d = scopes->at(scope)->GetHT()->Lookup(key);
        if (d != NULL) break;

        while (scopes->at(scope)->HasParent()) {
            parent = scopes->at(scope)->GetParent();
            scope = FindScopeFromOwnerName(parent);
            if (scope != -1) {
                if (scope == cur_scope) {
                    // If the parent relation has a loop, then report an error.
                    break;
                } else {
                    d = scopes->at(scope)->GetHT()->Lookup(key);
                    if (d != NULL) break;
                }
            } else {
                break;
            }
        }
        if (d != NULL) break;
    }

    return d;
}

/*
 * Look up symbol in parent scopes.
 */
Decl *
SymbolTable::LookupParent(Identifier *id)
{
    Decl *d = NULL;
    const char *parent = NULL;
    const char *key = id->GetIdName();
    int scope;

    scope = cur_scope;
    // Look up parent scopes.
    while (scopes->at(scope)->HasParent()) {
        parent = scopes->at(scope)->GetParent();
        scope = FindScopeFromOwnerName(parent);
        //printf("Look up %s from %s\n", key, parent);

        if (scope != -1) {
            if (scope == cur_scope) {
                // If the parent relation has a loop, then report an error.
                break;
            } else {
                d = scopes->at(scope)->GetHT()->Lookup(key);
                if (d != NULL) break;
            }
        }
    }

    return d;
}

/*
 * Look up symbol in interface scopes.
 */
Decl *
SymbolTable::LookupInterface(Identifier *id)
{
    Decl *d = NULL;
    const char *key = id->GetIdName();
    int scope;

    // Look up interface scopes.
    if (scopes->at(cur_scope)->HasInterface()) {

        std::list<const char *> * itfc = scopes->at(cur_scope)->GetInterface();

        for (std::list<const char *>::iterator it = itfc->begin();
                it != itfc->end(); it++) {
            scope = FindScopeFromOwnerName(*it);
            //printf("Look up %s from %s\n", key, *it);

            if (scope != -1) {
                d = scopes->at(scope)->GetHT()->Lookup(key);
                if (d != NULL) break;
            }
        }
    }
    return d;
}

/*
 * Insert new symbol into current scope.
 */
void
SymbolTable::InsertSymbol(Decl *decl)
{
    const char *key = decl->GetId()->GetIdName();
    scopes->at(cur_scope)->GetHT()->Enter(key, decl);
}

/*
 * Look up symbol in current scope.
 */
bool
SymbolTable::LocalLookup(Identifier *id)
{
    Decl *d;
    const char *key = id->GetIdName();

    d = scopes->at(cur_scope)->GetHT()->Lookup(key);

    return (d == NULL) ? false : true;
}

/*
 * Exit current scope and return to its uplevel scope.
 */
void
SymbolTable::ExitScope()
{
    activeScopes->pop_back();
    cur_scope = activeScopes->back();
}

/*
 * Deal with class inheritance, set parent for a subclass.
 */
void
SymbolTable::SetScopeParent(const char *key)
{
    scopes->at(cur_scope)->SetParent(key);
}

/*
 * Deal with class interface, set interfaces for a subclass.
 */
void
SymbolTable::SetInterface(const char *key)
{
    scopes->at(cur_scope)->AddInterface(key);
}

/*
 * Print the whole symbol table.
 */
void SymbolTable::Print()
{
    std::cout << "======== Symbol Table ========" << std::endl;

    for (int i = 0; i < scopes->size(); i++) {
        std::cout << "|- Hash Table for Scope " << i << ":";
        if (scopes->at(i)->HasOwner())
            std::cout << " (owner: " << scopes->at(i)->GetOwner() << ")";
        if (scopes->at(i)->HasParent())
            std::cout << " (parent: " << scopes->at(i)->GetParent() << ")";
        if (scopes->at(i)->HasInterface()) {
            std::cout << " (interface: ";
            std::list<const char *> *interface = scopes->at(i)->GetInterface();
            for (std::list<const char *>::iterator it = interface->begin();
                    it != interface->end(); it++) {
                std::cout << *it << " ";
            }
            std::cout << ")";
        }

        std::cout << std::endl;

        Iterator<Decl*> iter = scopes->at(i)->GetHT()->GetIterator();
        Decl *decl;
        while ((decl = iter.GetNextValue()) != NULL) {
            std::cout << "|  + " << decl << std::endl;
        }
    }

    std::cout << "======== Symbol Table ========" << std::endl;
}

