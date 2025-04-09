#include <cstdio>

#include "hsm.h"

/* Implements the following state machine from Miro Samek's
                                                       |
                                                       |
+-------------------------------------------------S----+------------------------------------------------------+
+------------------------------------------------------+------------------------------------------------------+
|entry/                                                |                                                      |
|exit/                                                 |                                                      |
|I[foo]/foo=0                                          |                                                      |
|                                                      |                                                      |
|                              +------------------     |            +------------------S2-----------------+   |
|                              |                       +----------->+-------------------------------------+   |
|                              |                                    |entry/                               |   |
|                              |                                    |exit/                                |   |
|                              |                                    |I[!foo]/foo=1                        |   |
|                     +--------+-----S1-------------+               |                                     |   |
|                     +--------+--------------------+               |                                     |   |
|        +------------+entry/  |                    +-------C------->                    +-------------   |   |
|        |            |exit/   |          +------   |               |                    |                |   |
|  D[!foo]/foo=1      |I/      |          |         <-------C-------+                    |                |   |
|        |            |        |          |         |               |      +---------S21-+------+         |   |
<--------+            |      +-v-----S11--v--+      |               |      +-------------+------+         |   |
|                     |      +---------------+      |               |      |entry/       |      +-----+   |   |
|                     |      |entry/         |      |               |      |exit/        |    | |     A   |   |
|                     |      |exit/          |       <-------G-------+------+    +---S211-v-+  | |     |   |   |
|                     |      |               |      |               |      |    +----------<--+ <-----+   |   |
|                     +------>               +------+-------G-------+------>    |entry/    |    |         |   |
|                     |      |               |      |               |      |    |exit/     <-B--+         |   |
+---------------------+------>               <------+-------F-------+      |    |          |    |         |   |
|                     |      +----+------+---+      |               |      |    |          +-D-->         |   |
|              +------+           |      |          +-------F-------+------+---->-----+----+    |         |   |
|              |      |           |      |          |               |      |          |         |         |   |
|              A      |   D[foo]/foo=0   H          |               |      |          H         |         |   |
|              |      |   |              |          |               |      +----------+---------+         |   |
|              +------>   |              |          |               |                 |                   |   |
|                     +---v--------------+----------+               |                 |                   |   |
|                                        |                          |                 |                   |   |
|                                        |                          +-----------------+-------------------+   |
|                                        |                                            |                       |
+----------------------------------------v--------------------------------------------v-----------------------+
 * Practical Statecharts in C/C++ v2
 */

class TestHSM;

typedef CompState<TestHSM, 0> Top;
typedef CompState<TestHSM, 1, Top> S;
typedef CompState<TestHSM, 2, S> S1;
typedef LeafState<TestHSM, 3, S1> S11;
typedef CompState<TestHSM, 4, S> S2;
typedef CompState<TestHSM, 5, S2> S21;
typedef LeafState<TestHSM, 6, S21> S211;

enum Signal { A_SIG, B_SIG, C_SIG, D_SIG, E_SIG, F_SIG, G_SIG, H_SIG, I_SIG };

class TestHSM {
   public:
    TestHSM();
    ~TestHSM() {}
    void next(const TopState<TestHSM>& state) { state_ = &state; }
    Signal getSig() const { return sig_; }
    void dispatch(Signal sig) {
        sig_ = sig;
        state_->handler(*this);
    }
    void foo(int i) { foo_ = i; }
    int foo() const { return foo_; }

   private:
    const TopState<TestHSM>* state_;
    Signal sig_;
    int foo_;
};

template <>
inline void Top::init(TestHSM& hsm) {
    hsm.foo(0);
    Tran<Top, Top, S2> t(hsm);
    fprintf(stderr, "Top-INIT;");
}

TestHSM::TestHSM() {
    Top::init(*this);
}

#define HSMINIT(State, InitState)           \
    template <>                             \
    inline void State::init(TestHSM& hsm) { \
        Tran<State, State, InitState> i(hsm);     \
        fprintf(stderr, #State "-INIT;");            \
    }

HSMINIT(S, S11)
HSMINIT(S1, S11)
HSMINIT(S2, S211)
HSMINIT(S21, S211)

static TestHSM test;

bool testDispatch(char c) {
    if (c < 'a' || 'i' < c) {
        return false;
    }
    fprintf(stderr, "%c: ", c);
    test.dispatch((Signal)(c - 'a'));
    fprintf(stderr, "\n");
    return true;
}

int main(int, char**) {
    fprintf(stderr, "\n");
    testDispatch('g');
    testDispatch('i');
    testDispatch('a');
    testDispatch('d');
    testDispatch('d');
    testDispatch('c');
    testDispatch('e');
    testDispatch('e');
    testDispatch('g');
    testDispatch('i');
    testDispatch('i');
    return 0;
}

#define HSMHANDLER(State) \
    template <>           \
    template <typename X> \
    inline void State::handle(TestHSM& h, const X& x) const

template <>
template <typename X>
inline void S::handle(TestHSM& h, const X& x) const {
    switch (h.getSig()) {
        case E_SIG: {
            fprintf(stderr, "S-E;");
            Tran<X, This, S11> t(h);
            return;
        }
        case I_SIG: {
            if (h.foo()) {
                h.foo(0);
                fprintf(stderr, "S-I;");
                return;
            }
            break;
        }
        default:
            break;
    }
    return Base::handle(h, x);
}

template <>
template <typename X>
inline void S1::handle(TestHSM& h, const X& x) const {
    switch (h.getSig()) {
        case A_SIG: {
            fprintf(stderr, "S1-A;");
            Tran<X, This, S1> t(h);
            return;
        }
        case B_SIG: {
            fprintf(stderr, "S1-B;");
            Tran<X, This, S11> t(h);
            return;
        }
        case C_SIG: {
            fprintf(stderr, "S1-C;");
            Tran<X, This, S2> t(h);
            return;
        }
        case D_SIG: {
            if (!h.foo()) {
                h.foo(1);
                fprintf(stderr, "S1-D;");
                Tran<X, This, S> t(h);
                return;
            }
            break;
        }
        case F_SIG: {
            fprintf(stderr, "S1-F;");
            Tran<X, This, S211> t(h);
            return;
        }
        case I_SIG:
            fprintf(stderr, "S1-I;");
            return;
        default:
            break;
    }
    return Base::handle(h, x);
}

template <>
template <typename X>
inline void S11::handle(TestHSM& h, const X& x) const {
    switch (h.getSig()) {
        case D_SIG: {
            if (h.foo()) {
                h.foo(0);
                fprintf(stderr, "S11-D;");
                Tran<X, This, S1> t(h);
                return;
            }
            break;
        }
        case G_SIG: {
            fprintf(stderr, "S11-G;");
            Tran<X, This, S211> t(h);
            return;
        }
        case H_SIG: {
            fprintf(stderr, "S11-H;");
            Tran<X, This, S> t(h);
            return;
        }
        default:
            break;
    }
    return Base::handle(h, x);
}

template <>
template <typename X>
inline void S2::handle(TestHSM& h, const X& x) const {
    switch (h.getSig()) {
        case C_SIG: {
            fprintf(stderr, "S2-C;");
            Tran<X, This, S1> t(h);
            return;
        }
        case F_SIG: {
            fprintf(stderr, "S2-F;");
            Tran<X, This, S11> t(h);
            return;
        }
        case I_SIG: {
            if (!h.foo()) {
                h.foo(1);
                fprintf(stderr, "S2-I;");
                return;
            }
            break;
        }
        default:
            break;
    }
    return Base::handle(h, x);
}

template <>
template <typename X>
inline void S21::handle(TestHSM& h, const X& x) const {
    switch (h.getSig()) {
        case A_SIG: {
            fprintf(stderr, "S21-A;");
            Tran<X, This, S21> t(h);
            return;
        }
        case B_SIG: {
            fprintf(stderr, "s21-B;");
            Tran<X, This, S211> t(h);
            return;
        }
        case G_SIG: {
            fprintf(stderr, "S21-G;");
            Tran<X, This, S1> t(h);
            return;
        }
        default:
            break;
    }
    return Base::handle(h, x);
}

template <>
template <typename X>
inline void S211::handle(TestHSM& h, const X& x) const {
    switch (h.getSig()) {
        case D_SIG: {
            fprintf(stderr, "s211-D;");
            Tran<X, This, S21> t(h);
            return;
        }
        case H_SIG: {
            fprintf(stderr, "s211-H;");
            Tran<X, This, S> t(h);
            return;
        }
        default:
            break;
    }
    return Base::handle(h, x);
}

#define HSMENTRY(State)                  \
    template <>                          \
    inline void State::entry(TestHSM&) { \
        fprintf(stderr, #State "-ENTRY;");        \
    }

HSMENTRY(S)
HSMENTRY(S1)
HSMENTRY(S11)
HSMENTRY(S2)
HSMENTRY(S21)
HSMENTRY(S211)

#define HSMEXIT(State)                  \
    template <>                         \
    inline void State::exit(TestHSM&) { \
        fprintf(stderr, #State "-EXIT;");        \
    }

HSMEXIT(S)
HSMEXIT(S1)
HSMEXIT(S11)
HSMEXIT(S2)
HSMEXIT(S21)
HSMEXIT(S211)
