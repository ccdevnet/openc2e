#ifndef __C2EEXCEPTIONS_H
#define __C2EEXCEPTIONS_H

#include <exception>

class creaturesException : public std::exception {
protected:
	const char *r;

public:
	creaturesException(const char *s) throw() { r = s; }
	const char* what() const throw() { return r; }
};

class tokeniseFailure : public creaturesException {
public:
	tokeniseFailure(const char *s) throw() : creaturesException(s) { }
};

class parseFailure : public creaturesException {
public:
	parseFailure(const char *s) throw() : creaturesException(s) { }
};

typedef parseFailure parseException;

class genomeException : public creaturesException {
public:
	genomeException(const char *s) throw() : creaturesException(s) { }
};

#endif
