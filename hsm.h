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

#include <type_traits>

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
    virtual void handler(Host& hsm) const override { handle(hsm, *this); }
    virtual unsigned getId() const override { return id; }
    static void init(Host& hsm) { hsm.next(obj); }  // don't specialize this
    static void entry(Host&) {}
    static void exit(Host&) {}
    static inline const LeafState obj{};  // only the leaf states have instances
};

// Transition Object
template <typename _Current, typename _Source, typename _Target>
struct Tran {
    using Host = typename _Current::Host;
    using CurrentBase = typename _Current::Base;
    using SourceBase = typename _Source::Base;
    using TargetBase = typename _Target::Base;

    enum {  // work out when to terminate template recursion
        eTB_CB = std::is_base_of_v<CurrentBase, TargetBase>,
        eT_CB = std::is_base_of_v<CurrentBase, _Target>,

        eT_C = std::is_base_of_v<_Current, _Target>,
        eT_S = std::is_base_of_v<_Source, _Target>,
        eS_T = std::is_base_of_v<_Target, _Source>,
        eSB_T = std::is_base_of_v<_Target, SourceBase>,

        eS_C = std::is_base_of_v<_Current, _Source>,
        eC_S = std::is_base_of_v<_Source, _Current>,
        eS_CB = std::is_base_of_v<CurrentBase, _Source>,

        exitStop = (eTB_CB && eS_CB) || (eSB_T && eT_CB),

        entryStop = eS_C || (eS_CB && (!eC_S || eT_C))
    };

    // We use overloading to stop recursion.
    // The more natural template specialization
    // method would require to specialize the inner
    // template without specializing the outer one,
    // which is forbidden.
    template <bool ExitStop>
    static void exitActions(Host &hsm) {
        if constexpr (!ExitStop) {
          _Current::exit(hsm);
          Tran<CurrentBase, _Source, _Target>::template exitActions<exitStop>(hsm);
        }
    }

    Tran(Host& hsm) : host_(hsm) {
        exitActions<eT_S && eS_C>(host_);
    }

    template <bool EntryStop>
    static void entryActions(Host &hsm) {
        if constexpr (!EntryStop) {
          Tran<CurrentBase, _Source, _Target>::template entryActions<entryStop>(hsm);
          _Current::entry(hsm);
        }
    }

    ~Tran() {
        Tran<_Target, _Source, _Target>::template entryActions<eS_T>(host_);
        _Target::init(host_);
    }

    Host& host_;
};
