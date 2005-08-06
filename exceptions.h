#ifndef __C2EEXCEPTIONS_H
#define __C2EEXCEPTIONS_H

#include <exception>
#include <cstdlib>
#include <string>

class creaturesException : public std::exception {
protected:
    bool malloced;
	const char *r;

public:
	creaturesException(const char *s) throw() { r = s; malloced = false; }
    creaturesException(const std::string &s) throw() {
        r = strdup(s.c_str());
        if (!r) {
            abort();
        }
        malloced = true;
    }
    ~creaturesException() throw() {
        if (malloced) free((void *)r);
    }
	const char* what() const throw() { return r; }
};

class tokeniseFailure : public creaturesException {
public:
	tokeniseFailure(const char *s) throw() : creaturesException(s) { }
    tokeniseFailure(const std::string &s) throw() : creaturesException(s) { }
};

class parseFailure : public creaturesException {
public:
	parseFailure(const char *s) throw() : creaturesException(s) { }
	parseFailure(const std::string &s) throw() : creaturesException(s) { }
};

typedef parseFailure parseException;

class genomeException : public creaturesException {
public:
	genomeException(const char *s) throw() : creaturesException(s) { }
	genomeException(const std::string &s) throw() : creaturesException(s) { }
};

#endif
