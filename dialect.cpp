#include "dialect.h"
#include "lex.yy.h"
#include "token.h"
#include <string>
#include <cctype>
#include "caosScript.h"
#include "cmddata.h"
#include "exceptions.h"
#include "caosVar.h"

Dialect *cmd_dialect, *exp_dialect;

void Dialect::doParse(caosScript *s) {
    token *t;
    while (!stop) {
        t = getToken();
        if (!t) break;
        handleToken(s, t);
    }
}

void Dialect::handleToken(caosScript *s, token *t) {
    if (t->type != TOK_WORD)
        throw parseFailure("unexpected non-word");
    std::string word = t->word;
    if (delegates.find(word) == delegates.end())
        throw parseException("no delegate for word");
    parseDelegate &p = *delegates[word];
    p(s, this);
}

class ConstOp : public caosOp {
    protected:
        caosVar constVal;
    public:
        virtual void execute(caosVM *vm) {
            vm->valueStack.push_back(constVal);
            caosOp::execute(vm);
        }

        ConstOp(const caosVar &val) {
            constVal = val;
        }
};

class ExprDialect : public BaseExprDialect {
    public:
        void handleToken(caosScript *s, token *t) {
            switch (t->type) {
                case TOK_CONST:
                    s->current->thread(new ConstOp(t->constval));
                    break;
                case TOK_WORD:
                    // TODO: VAxx, OVxx
                    
                    Dialect::handleToken(s, t);
                    return;
                case TOK_BYTESTR:
                    // TODO
                    abort();
                    break;
                default:
                    assert(false);
            }
        }
};

void DoifDialect::handleToken(class caosScript *s, token *t) {
    if (t->type == TOK_WORD) {
        if (t->word == "endi") {
            if (failure) // we don't have an else clause
                s->current->thread(failure);
            s->current->thread(exit);
            stop = true;
            return;
        }
        if (t->word == "else") {
            if (!failure)
                throw new parseException("double else clause is forbidden");
            s->current->thread(exit);
            s->current->last = failure;
            failure = NULL;
            return;
        }
        if (t->word == "elif") {
            // emuluate an else-doif-endi block
            if (!failure)
                throw new parseException("double else clause is forbidden");
            s->current->thread(exit);
            s->current->last = failure;
            failure = NULL;
            DoifParser dip;
            dip(s, this);
            s->current->thread(exit);
            stop = true;
            return;
        }
    }
    CommandDialect::handleToken(s, t);
}

    
void registerDelegates() {
    cmd_dialect = new CommandDialect();
    exp_dialect = new ExprDialect();
    registerAutoDelegates();
}
