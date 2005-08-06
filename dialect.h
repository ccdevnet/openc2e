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
            
            while(1) {
                exp_dialect->doParse(s);
                token *comparison = getToken(TOK_WORD);
                std::string cword = comparison->word;
                int compar;
                if (cword == "eq")
                    compar = CEQ;
                else if (cword == "gt")
                    compar = CGT;
                else if (cword == "ge")
                    compar = CGE;
                else if (cword == "lt")
                    compar = CLT;
                else if (cword == "le")
                    compar = CLE;
                else if (cword == "ne")
                    compar = CNE;
                exp_dialect->doParse(s);
                

                /*
                 * If the next bind is or, we jump to success.
                 * Otherwise, we negate and jump to failure.
                 * The last item is always considered or-ed.
                 */
                int isOr = 1;
                int isLast = 0;

                struct token *peek = tokenPeek();
                if (!peek)
                    throw parseException("unexpected eoi");
                if (peek->type == TOK_WORD) {
                    if (peek->word == "and") {
                        getToken();
                        isOr = 0;
                    } else if (peek->word == "or")
                        getToken();
                    else
                        isLast = 1;
                }

                caosOp *jumpTarget = isOr ? success : failure;
                if (!isOr) compar = ~compar;
                
                s->current->thread(new caosCond(compar, jumpTarget));
                if (isLast) break;
            }
            
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

void registerDelegates();

#endif

