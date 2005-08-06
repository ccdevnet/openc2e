#ifndef DIALECT_H
#define DIALECT_H 1

#include "token.h"
#include "bytecode.h"
#include "caosScript.h"
#include "cmddata.h"
#include <map>

class parseDelegate {
    public:
        virtual void operator()(class caosScript *s, class Dialect *curD) = 0;
        virtual ~parseDelegate() {}
};

class DefaultParser : public parseDelegate {
    protected:
        void (caosVM::*handler)();
        int idx;
    public:
        DefaultParser(void (caosVM::*h)(), int i) :
            handler(h), idx(i) {}
        virtual void operator()(class caosScript *s, class Dialect *curD);
};

class Dialect {
    public:
        bool stop;

        Dialect() : stop(false) {}
        
        std::map<std::string, parseDelegate *> delegates;
        virtual void doParse(class caosScript *s) {
            while (!stop && parseOne(s));
        }
        virtual bool parseOne(class caosScript *s);
        virtual void handleToken(class caosScript *s, token *t);
};

extern Dialect *cmd_dialect, *exp_dialect;

class OneShotDialect : public Dialect {
    public:
        void doParse(class caosScript *s) {
            Dialect::parseOne(s);
        }
};

// XXX: these don't really belong here

void parseCondition(caosScript *s, caosOp *success, caosOp *failure);

class DoifDialect : public Dialect {
    protected:
        caosOp *success, *failure, *exit;
    public:
        DoifDialect(caosOp *s, caosOp *f, caosOp *e)
            : success(s), failure(f), exit(e) {
                delegates = cmd_dialect->delegates; // XXX
            }
        void handleToken(class caosScript *s, token *t); 
};

class DoifParser : public parseDelegate {
    protected:
        caosOp *success, *failure, *exit;
    public:
        virtual void operator()(class caosScript *s, class Dialect *curD) {
            success = new caosNoop();
            failure = new caosNoop();
            exit = new caosNoop();
            s->current->addOp(success);
            s->current->addOp(failure);
            s->current->addOp(exit);
            
            parseCondition(s, success, failure);
            
            s->current->thread(failure);
            s->current->last = success;
            DoifDialect d(success, failure, exit);
            d.doParse(s);
        }
};
        
class NamespaceDelegate : public parseDelegate {
    public:
        OneShotDialect dialect;
        void operator() (class caosScript *s, class Dialect *curD) {
            dialect.parseOne(s);
        }
};

class REPE : public parseDelegate {
    public:
        void operator() (class caosScript *s, class Dialect *curD) {
            curD->stop = true;
        }
};

class parseREPS : public parseDelegate {
    public:
        void operator() (class caosScript *s, class Dialect *curD) {
            caosOp *exit = new caosNoop();
            s->current->addOp(exit);
            
            exp_dialect->parseOne(s); // repcount
            caosOp *entry = new caosREPS(exit);
            s->current->thread(entry);

            Dialect d;
            REPE r;
            d.delegates = cmd_dialect->delegates;
            d.delegates["repe"] = &r;

            d.doParse(s);
            s->current->last->setSuccessor(entry);
            s->current->last = exit;
        }
};

class EVER : public parseDelegate {
    protected:
        caosOp *exit;
    public:
        EVER(caosOp *exit_) : exit(exit_) {}
        void operator() (class caosScript *s, class Dialect *curD) {
            s->current->thread(exit);
            curD->stop = true;
        }
};

class UNTL : public parseDelegate {
    protected:
        caosOp *entry, *exit;
    public:
        UNTL(caosOp *en, caosOp *ex) : entry(en), exit(ex) {}
        void operator() (class caosScript *s, class Dialect *curD) {
            parseCondition(s, exit, entry);
            curD->stop = true;
        }
};

class parseLOOP : public parseDelegate {
    public:
        void operator() (class caosScript *s, class Dialect *curD) {
            caosOp *exit = new caosNoop();
            s->current->addOp(exit);
            
            caosOp *entry = new caosNoop();
            s->current->thread(entry);

            Dialect d;
            EVER ever(entry); UNTL untl(entry, exit);
            d.delegates = cmd_dialect->delegates;
            d.delegates["ever"] = &ever;
            d.delegates["untl"] = &untl;

            d.doParse(s);
            // No need to thread - if we use UNTL, we _will_ go to either
            // entry or exit
            s->current->last = exit;
        }
};


class parseGSUB : public parseDelegate {
    public:
        void operator() (class caosScript *s, class Dialect *curD) {
            token *t = getToken(TOK_WORD);
            std::string label = t->word;
            caosOp *targ = s->current->gsub[label];
            if (!targ) {
                targ = new caosNoop();
                s->current->addOp(targ);
                s->current->gsub[label] = targ;
            }
            s->current->thread(new caosGSUB(targ));
        }
};

class parseSUBR : public parseDelegate {
    public:
        void operator() (class caosScript *s, class Dialect *curD) {
            token *t = getToken(TOK_WORD);
            std::string label = t->word;
            caosOp *targ = s->current->gsub[label];
            if (!targ) {
                targ = new caosNoop();
                s->current->addOp(targ);
                s->current->gsub[label] = targ;
            }
            s->current->last = targ;
        }
};

void registerDelegates();

#endif

