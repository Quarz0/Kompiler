#include "DFA.h"
#include "machine.h"
#include <stack>
#include <iostream>
#define EPS 0xDE

std::set<state> dfa::epsilon_closure(state s)
{
    std::set<state> res;
    std::stack<state> stack;

    res.insert(s);

    stack.push(s);
    while (!stack.empty()) {
        state u = stack.top();
        stack.pop();
        std::vector<state::transition> eps_trans = u.get_transitions_for(EPS);
        for (state::transition t : eps_trans) {
            state v = t.get_end_state();
            stack.push(v);

            res.insert(v);
        }
    }

    return res;
}

std::set<state> dfa::epsilon_closure(std::set<state> state_set)
{
    std::set<state> res = state_set;

    for (state i : state_set) {
        state s = i;
        std::set<state> eps_set = epsilon_closure(s);

        res.insert(eps_set.begin(), eps_set.end());
    }

    return res;
}

std::set<state> dfa::move(state s, char in)
{
    std::set<state> res;

    std::vector<state::transition> in_trans = s.get_transitions_for(in);
    for (state::transition t : in_trans) {
        state u = t.get_end_state();
        res.insert(u);
    }

    return res;
}

std::set<state> dfa::move(std::set<state> state_set, char in)
{
    std::set<state> res = state_set;

    for (state i : state_set) {
        state s = i;
        std::set<state> ch_set = dfa::move(s, in);

        res.insert(ch_set.begin(), ch_set.end());
    }

    return res;
}

std::string get_key(std::set<int> &s) {
    return "";
}

std::string get_token_type(std::set<int> &s, machine &nfa, bool &is_final) {

    return "";
}

machine* dfa::to_dfa(machine &nfa) {
    std::vector<int> unmarked_states;
    std::set<int> cur_states = epsilon_closure(nfa.get_starting_state());
    std::set<int> dfa_states;
    int count = 0;
    machine dfa_machine("dfa");
    int starting_id = dfa_machine.add_starting_state();
    unmarked_states.push_back(starting_id);
    dfa_machine.set_key_for(starting_id, "" + starting_id);
    while (!unmarked_states.empty()) {
        int cur = unmarked_states.back();
        unmarked_states.pop_back();
        for (char input : nfa.get_inputs()) {
            std::set<int> temp = dfa::move(cur_states, input);
            cur_states = dfa::epsilon_closure(temp);
            int found_state = -1;
            std::string new_key = get_key(cur_states);

            for (int s = 0 ; s < dfa_machine.get_states_count() ; s++) {
                if (dfa_machine.get_key_for(s) == new_key)
                    found_state = s;
            }

            if (found_state == -1) {
                bool is_final = false;
                std::string token_type = get_token_type(cur_states, nfa, is_final);
                int new_dfa_state = dfa_machine.add_new_state(token_type, is_final);
                unmarked_states.push_back(new_dfa_state);
                dfa_machine.add_new_transition(cur, new_dfa_state, input);
            } else {
                dfa_machine.add_new_transition(cur, found_state, input);
            }
        }
    }
    return &dfa_machine;
}

bool same_partition(const machine &dfa, const state &a, const state &b) {
    for (char input : dfa.get_inputs()) {
        std::vector<state::transition> a_transitions = a.get_transitions_for(input);
        std::vector<state::transition> b_transitions = b.get_transitions_for(input);
        if (b_transitions.size() > 1 || a_transitions.size() > 1) {
            std::cerr << "Error: more than 1 transition over the same symbol in DFA" << std::endl;
            return false;
        }
        if (b_transitions.size() != a_transitions.size())
            return false;
        if (a_transitions.size() == 1) {
            if (a_transitions[0] != b_transitions[0])
                return false;
        }
    }
    return true;
}

std::set<std::set<state> > refine(const machine &dfa, const std::set<std::set<state> > &sets) {
    std::set<std::set<state> > new_sets;
    for (std::set<state> states : sets) {
        std::set<state> working_set(states);
        while (working_set.size() > 0) {
            state s = *working_set.begin();
            std::set<state> new_set;
            new_set.insert(s);
            working_set.erase(s);
            for (state q : working_set) {
                if (same_partition(dfa, s, q)) {
                    new_set.insert(q);
                }
            }
            new_sets.insert(new_set);
        }
    }
    return new_sets;
}

machine* build_dfa(std::set<std::set<state> > sets) {
    machine dfa("min_dfa");
    if (sets.size() == 0)
        return &dfa;
    std::vector<std::set<state> > sets_vec(sets.begin(), sets.end());
    state starting_state(0, sets_vec[0]);
    dfa.set_starting_state(&starting_state);
    for (int i = 1 ; i < sets_vec.size() ; i++) {
        state cur_state(i, sets_vec[i]);
        dfa.add_new_state(cur_state);
    }
    return &dfa;
}

machine* dfa::minimize_dfa(machine& dfa) {
    std::set<std::set<state> > cur_set;
    std::set<state> accepting;
    std::set<state> non_accepting;
    for (state s : dfa.get_states()) {
        if (s.is_accepting()) {
            accepting.insert(s);
        } else {
            non_accepting.insert(s);
        }
    }
    cur_set.insert(accepting);
    cur_set.insert(non_accepting);

    while (true) {
        std::set<std::set<state> > new_set = refine(dfa, cur_set);
        if (new_set.size() == cur_set.size())
            break;
        cur_set = new_set;
    }
    return build_dfa(cur_set);
}
