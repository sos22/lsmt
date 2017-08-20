#ifndef TESTOPS_H__
#define TESTOPS_H__

struct structA {
    int v;
    explicit structA(int _v) : v(_v) {}
    bool operator<(structA const &) const;
};

struct structB {
    int v;
    explicit structB(int _v) : v(_v) {}
    bool operator<(structB const &) const; };

void dotest();

#endif /* !TESTOPS_H__ */
