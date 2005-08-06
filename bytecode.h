#ifndef BYTECODE_H
#define BYTECODE_H 1

#include "caosVM.h"

typedef void (caosVM::*ophandler)();
class caosOp {
    public:
        virtual void execute(caosVM *vm) {
            vm->timeslice -= evalcost;
            vm->nip = successor;
        }
        void setSuccessor(caosOp *succ) {
            successor = succ;
        }
        void setCost(int cost) {
            evalcost = cost;
        }
        caosOp() : evalcost(1), successor(NULL) {}
        virtual ~caosOp() {};
    protected:
        int evalcost;
        caosOp *successor;
};

class caosNoop : public caosOp {
    public:
        caosNoop() { evalcost = 0; }
};

class simpleCaosOp : public caosOp {
    protected:
        ophandler handler;
    public:
        simpleCaosOp(ophandler h) : handler(h) {}
        void execute(caosVM *vm) {
            caosOp::execute(vm);
            (vm->*handler)();
        }
};

// Condition classes
#define CEQ 1
#define CLT 2
#define CGT 4
#define CLE (CEQ | CLT)
#define CGE (CEQ | CGT)
#define CNE (CLT | CGT)

class caosCond : public caosOp {
    protected:
        int cond;
        caosOp *branch;
    public:
        caosCond(int condition, caosOp *br)
            : cond(condition), branch(br) {}
        void execute(caosVM *vm) {
            caosOp::execute(vm);
            
            VM_PARAM_VALUE(arg2);
            VM_PARAM_VALUE(arg1);

            int cres;
            if (arg2.hasString() && arg1.hasString()) {
                std::string str1 = arg1.getString();
                std::string str2 = arg2.getString();
                
                if (str1 < str2)
                    cres = CLT;
                else if (str1 > str2)
                    cres = CGT;
                else
                    cres = CEQ;
            } else if (arg2.hasDecimal() && arg1.hasDecimal()) {
                float val1 = arg1.getFloat();
                float val2 = arg2.getFloat();

                if (val1 < val2)
                    cres = CLT;
                else if (val1 > val2)
                    cres = CGT;
                else
                    cres = CEQ;
            } else cres = 0;

            if (cres & cond)
                vm->nip = branch;
        }
};

#endif

