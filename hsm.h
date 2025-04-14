#pragma once

// This code is from:
// Yet Another Hierarchical State Machine
// by Stefan Heinzmann
// Overload issue 64 december 2004
// http://www.state-machine.com/resources/Heinzmann04.pdf

/* This is a basic implementation of UML Statecharts.
 * The key observation is that the machine can only
 * be in a leaf state at any given time. The composite
 * states are only traversed, never final.
 * Only the leaf states are ever instantiated. The composite
 * states are only mechanisms used to generate code. They are
 * never instantiated.
 */

// Top State, Composite State and Leaf State

template <typename _Host>
struct TopState {
    using Host = _Host;
    using Base = void;
    virtual void handler(Host&) const = 0;
    virtual unsigned getId() const = 0;
};

template <typename _Host, unsigned id, typename _Base>
struct CompState;

template <typename _Host, unsigned id, typename _Base = CompState<_Host, 0, TopState<_Host> > >
struct CompState : _Base {
    using Base = _Base;
    using This = CompState<_Host, id, Base>;
    template <typename X>
    void handle(_Host& h, const X& x) const {
        Base::handle(h, x);
    }
    static void init(_Host&);  // no implementation
    static void entry(_Host&) {}
    static void exit(_Host&) {}
};

template <typename _Host>
struct CompState<_Host, 0, TopState<_Host> > : TopState<_Host> {
    using Base = TopState<_Host>;
    using This = CompState<_Host, 0, Base>;
    template <typename X>
    void handle(_Host&, const X&) const {}
    static void init(_Host&);  // no implementation
    static void entry(_Host&) {}
    static void exit(_Host&) {}
};

template <typename _Host, unsigned id, typename _Base = CompState<_Host, 0, TopState<_Host> > >
struct LeafState : _Base {
    using Host = _Host;
    using Base = _Base;
    using This = LeafState<Host, id, Base>;
    template <typename X>
    void handle(Host& h, const X& x) const {
        Base::handle(h, x);
    }
    virtual void handler(Host& hsm) const { handle(hsm, *this); }
    virtual unsigned getId() const { return id; }
    static void init(Host& hsm) { hsm.next(obj); }  // don't specialize this
    static void entry(Host&) {}
    static void exit(Host&) {}
    static const LeafState obj;  // only the leaf states have instances
};

template <typename _Host, unsigned id, typename _Base>
const LeafState<_Host, id, _Base> LeafState<_Host, id, _Base>::obj;

// Helpers

// A gadget from Herb Sutter's GotW #71 -- depends on SFINAE
template <class _Derived, class _Base>
class IsDerivedFrom {
    class Yes {
        char a[1];
    };
    class No {
        char a[10];
    };
    static Yes Test(_Base *);  // undefined
    static No Test(...);  // undefined
   public:
    enum { Is = sizeof(Test(static_cast<_Derived *>(0))) == sizeof(Yes) ? 1 : 0 };
};

template <bool>
class Bool {};

// Transition Object

template <typename _Current, typename _Source, typename _Target>
struct Tran {
    typedef typename _Current::Host Host;

    typedef typename _Current::Base CurrentBase;
    typedef typename _Source::Base SourceBase;
    typedef typename _Target::Base TargetBase;
    enum {  // work out when to terminate template recursion
        eTB_CB = IsDerivedFrom<TargetBase, CurrentBase>::Is,
        eT_CB = IsDerivedFrom<_Target, CurrentBase>::Is,

        eT_C = IsDerivedFrom<_Target, _Current>::Is,
        eT_S = IsDerivedFrom<_Target, _Source>::Is,
        eS_T = IsDerivedFrom<_Source, _Target>::Is,
        eSB_T = IsDerivedFrom<SourceBase, _Target>::Is,

        eS_C = IsDerivedFrom<_Source, _Current>::Is,
        eC_S = IsDerivedFrom<_Current, _Source>::Is,
        eS_CB = IsDerivedFrom<_Source, CurrentBase>::Is,

        exitStop = (eTB_CB && eS_CB) || (eSB_T && eT_CB),

        entryStop = eS_C || (eS_CB && (!eC_S || eT_C))
    };

    // We use overloading to stop recursion.
    // The more natural template specialization
    // method would require to specialize the inner
    // template without specializing the outer one,
    // which is forbidden.

    static void exitActions(Host& hsm, Bool<true>) {}
    static void exitActions(Host& hsm, Bool<false>) {
        _Current::exit(hsm);
        Tran<CurrentBase, _Source, _Target>::exitActions(hsm, Bool<exitStop>());
    }

    Tran(Host& hsm) : host_(hsm) {
        exitActions(host_, Bool<eT_S && eS_C>());
    }

    static void entryActions(Host& hsm, Bool<true>) {}
    static void entryActions(Host& hsm, Bool<false>) {
        Tran<CurrentBase, _Source, _Target>::entryActions(hsm, Bool<entryStop>());
        _Current::entry(hsm);
    }

    ~Tran() {
        Tran<_Target, _Source, _Target>::entryActions(host_, Bool<eS_T>());
        _Target::init(host_);
    }

    Host& host_;
};
