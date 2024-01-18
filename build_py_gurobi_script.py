import re
import gurobipy as gp
from gurobipy import GRB

import time



#file names to read
FILE_NAME = 'current'
FILE_NAME_STA = FILE_NAME + '.sta'
FILE_NAME_TRA = FILE_NAME + '.tra'
FILE_NAME_LAB = FILE_NAME + '.lab'
FILE_NAME_TREW = FILE_NAME + '.trew'
PRISM_LOG_NAME = FILE_NAME + '.log'

def make_reverse_label_dict(label_dict):
    return {v:k for k,v in label_dict.items()}

def get_labeled_states(state_to_labels_dict, label_dict, label):
    reverse =  make_reverse_label_dict(label_dict)
    collect = set()
    for state in state_to_labels_dict:
        if reverse[label] in state_to_labels_dict[state]:
            collect.add(state)
    return collect

def read_states():
    max_state = 0
    
    with open(FILE_NAME_STA) as states_file:
        file_content = states_file.read()
        #m = re.search(r'\d+(?=\:)', file_content)
        #print(m.group(0))
        
        for m in re.finditer(r'\d+(?=\:)', file_content):
            state = int(m.group(0))
            if (state > max_state):
                max_state = state
    
    return max_state + 1


def prism_log_read_e_max():
    with open(PRISM_LOG_NAME) as file:
        file_content = file.read()
        m = re.search(r'(?<=Result\: )[-+]?[0-9]*\.?[0-9]*', file_content)
        #print(m.group(0))
        return float(m.group(0))



def read_transitions(transition_table):
    #state_mapped_to_action_list = {}
    
    max_action_id = 0
    
    with open(FILE_NAME_TRA) as file:
        file_content = file.read()
        
        regex_integer = r'[0-9]+'
        regex_float = r'[-+]?[0-9]*\.?[0-9]+'
        regex = r'[0-9]* [0-9]* [0-9]* [-+]?[0-9]*\.?[0-9]*'
        #m = re.search(regex, file_content)
        #print(m.group(0))
        for m in re.finditer(regex, file_content):
            line0 = m.group(0)
            m_from_state = re.search(regex_integer, line0)
            state = int(m_from_state.group(0))
            #print(line0)
            
            
            line1 = line0[m_from_state.end(0):]
            #print(line1)
            
            
            m_selected_action = re.search(regex_integer, line1)
            line2 = line1[m_selected_action.end(0):]
            #print(line2)
            act = int(m_selected_action.group(0))
            if (act > max_action_id):
                max_action_id = act
            
            
            m_to_state = re.search(regex_integer, line2)
            line3 = line2[m_to_state.end(0):]
            succ_state = int(m_to_state.group(0))
            
            
            m_probability = re.search(regex_float, line3)
            line4= line3[m_probability.end(0):]
            prob = float(m_probability.group(0))
            
            
            
            #if not state in state_mapped_to_action_list:
            #    state_mapped_to_action_list[state] = set()
            #state_mapped_to_action_list[state].add(act)
            
            if not state in transition_table:
                transition_table[state] = {}
            if not act in transition_table[state]:
                transition_table[state][act] = {}
            transition_table[state][act][succ_state] = prob
            #print('debug')
            
            
    return max_action_id + 1


def read_transition_rewards(transition_rewards):
    with open(FILE_NAME_TREW) as file:
        file_content = file.read()
        
        regex_reward_definition_line = r'([0-9]+) ([0-9]+) ([0-9]+) ([0-9]+)'
        
        list_of_rew_items = re.findall(regex_reward_definition_line, file_content)
        #print(list_of_rew_items)
        
        for element in list_of_rew_items:
            from_state = int(element[0])
            act = int(element[1])
            to_state = int(element[2])
            rew = int(element[3])
            
            if from_state not in transition_rewards:
                transition_rewards[from_state] = {}
            if act not in transition_rewards[from_state]:
                transition_rewards[from_state][act] = {}
            if to_state not in transition_rewards[from_state][act]:
                transition_rewards[from_state][act][to_state] = {}
            transition_rewards[from_state][act][to_state] = rew


def get_transition_reward(transition_rewards, from_state, action, to_state):
    if from_state not in transition_rewards:
        return 0.0
    if action not in transition_rewards[from_state]:
        return 0.0
    if to_state not in transition_rewards[from_state][action]:
        return 0.0
    return transition_rewards[from_state][action][to_state]


def read_labels(label_dict, state_to_labels_dict):

    with open(FILE_NAME_LAB) as labels_file:
        file_content = labels_file.read()
        
        regex_label_definitions = r'([0-9]+=\"\w+\"\s?)+'
        regex_one_label_definition = r'([0-9]+)=\"(\w+)\"'
        
        m_all_definitions = re.search(regex_label_definitions, file_content)
        all_definitions = m_all_definitions.group(0)
        
        label_definition_list = re.findall(regex_one_label_definition, all_definitions)
        #
        #print(label_definition_list)#[('0', 'init'), ('1', 'deadlock'), ('2', 'elected')]
        
        for label_definition in label_definition_list:
            #m = re.search(regex_one_label_definition, label_definition)
            #print("label no{}  :  {}".format(m.group(1), m.group(2)))
            label_dict[label_definition[0]] = label_definition[1]
        
        rest_file = file_content[m_all_definitions.end(0):]
        
        
        regex_state_to_labels = r'([0-9]+):\s*(( [0-9]+)+)'
        regex_integer = r'[0-9]+'
        
        state_label_list = re.findall(regex_state_to_labels, rest_file)
        #print(state_label_list)

        for element in state_label_list:
            state_to_labels_dict[int(element[0])] = set()
            list_of_label_ids = re.findall(regex_integer, element[1])
            for id in list_of_label_ids:
                state_to_labels_dict[int(element[0])].add(id)
        #print(state_to_labels_dict)
        

def get_variable_name(from_state, weight, action):
    if weight is None:
        return "var__x__s{}_a{}".format(from_state, action)
    return "var__x__s{}_w{}_a{}".format(from_state, weight, action)


def get_gurobi_var(gurobi_model, all_variables, state, weight, action):
    #all_variables {state to { weight to { action to gurobi_variable}}
    if not state in all_variables:
        all_variables[state] = {}
    if not weight in all_variables[state]:
        all_variables[state][weight] = {}
    if not action in all_variables[state][weight]:
        all_variables[state][weight][action] = gurobi_model.addVar(lb=0.0, name=get_variable_name(state, weight, action))
    return all_variables[state][weight][action]

def get_penalty_variable_name(state, weight):
    return "penalty_s{}_w{}".format(state, weight)

def get_gurobi_penalty_var(gurobi_model, penalty_variables, state, weight):
    #all_variables {state to { weight to { action to gurobi_variable}}
    if not state in penalty_variables:
        penalty_variables[state] = {}
    if not weight in penalty_variables[state]:
        penalty_variables[state][weight] = gurobi_model.addVar(name=get_penalty_variable_name(state, weight))
    return penalty_variables[state][weight]


def get_in_out_term(the_dict, state, weight):
    if state not in the_dict:
        the_dict[state] = {}
    if weight not in the_dict[state]:
        the_dict[state][weight] = float(0)
    return the_dict[state][weight]

def set_in_out_term(the_dict, state, weight, value):
    if state not in the_dict:
        the_dict[state] = {}
    the_dict[state][weight] = value

def build_linear_constraints(gurobi_model, all_variables, count_states, state_to_labels_dict, label_dict, transition_table, transition_rewards, global_e_max, emax_statewise_dict):
    
    lambda_factor = 0.4
    
    already_explored_state_reward_pairs = set()
    state_reward_pairs_to_explore = [] #
    
    in_term_dict  = {} # state |-> ( weight |-> gurobi_term)
    out_term_dict = {} # state |-> ( weight |-> gurobi_term)
    
    over_e_max_in_terms = []
    
    
    #print(label_dict)
    
    #find initial state
    reverse_label_dict = make_reverse_label_dict(label_dict)
    
    final_states_set = get_labeled_states(state_to_labels_dict, label_dict, 'elected')
    initial_states_set = get_labeled_states(state_to_labels_dict, label_dict, 'init')
    
    #print(reverse_label_dict)
    initial_state_id = int(list(initial_states_set)[0])
    #check that there is only one initial state!
    
    #zero_var_candidates = set() not needed anymore
    
    state_reward_pairs_to_explore.append((initial_state_id, 0))
    
    set_in_out_term(in_term_dict, initial_state_id, 0, 1) # in going 1 in initial state
    
    while len(state_reward_pairs_to_explore) > 0:
        
        
        #print (state_reward_pairs_to_explore)
        (state, rew) = state_reward_pairs_to_explore.pop(0)
        #check if already expanded:
        
        if (state, rew) in already_explored_state_reward_pairs:
            continue
        already_explored_state_reward_pairs.add((state,rew))
        
        # also continue if the variable is past the model's E_max
        if rew > global_e_max:
            over_e_max_in_terms.append((state,rew))
            continue
        
        # also continue if the variable is goal variable
        if state in final_states_set:
            over_e_max_in_terms.append((state,rew))
            continue
        
        #in_term = 0
        #out_term = 0
        
        #print(transition_table)
        #print(state)
        #print(state in transition_table)
        #print(transition_table[state])
        
        
        for action in transition_table[state]:
            #if out_term is None:
            #    out_term = get_gurobi_var(gurobi_model, all_variables, state, rew, action)
            #else:
            
            # set out term:
            gvar = get_gurobi_var(gurobi_model, all_variables, state, rew, action)
            out_term = get_in_out_term(out_term_dict, state, rew) + gvar
            set_in_out_term(out_term_dict, state, rew, out_term)
            
            # add term to in_term of successor states
            for succ_state in transition_table[state][action]:
                difference_rew = 0
                try:
                    difference_rew = transition_rewards[state][action][succ_state] # must be set !!!!!!
                except:
                    difference_rew = 0
                new_reward = rew + difference_rew
                
                state_reward_pairs_to_explore.append((succ_state, new_reward)) # to be explored
                
                in_term = get_in_out_term(in_term_dict, succ_state, new_reward) + transition_table[state][action][succ_state] * gvar # in_term + Prob(s, alpha, t) * x_s_w_alpha
                set_in_out_term(in_term_dict, succ_state, new_reward, in_term)
        
        #for previous_state in transition_table:
        #    for action in transition_table[previous_state]:
        #        if state in transition_table[previous_state][action]:
        #            difference_rew = 0
        #            try:
        #                difference_rew = transition_rewards[previous_state][action][state] # must be set !!!!!!
        #            except:
        #                difference_rew = 0
        #            
        #            
        #            if in_term is None:
        #                in_term = get_gurobi_var(gurobi_model, all_variables, previous_state, rew - difference_rew, action)
        #            else:
        #                in_term = in_term + get_gurobi_var(gurobi_model, all_variables, previous_state, rew - difference_rew, action)
        #            zero_var_candidates.add(get_gurobi_var(gurobi_model, all_variables, previous_state, rew - difference_rew, action))
        #
        ##if initial add input 1
        #if (state, rew) == (initial_state_id, 0):
        #    in_term += 1
    
    for state in out_term_dict:
        for weight in out_term_dict[state]:
            out_term = out_term_dict[state][weight]
            in_term = in_term_dict[state][weight]
            gurobi_model.addConstr(in_term == out_term, "flow__s{}__w{}".format(state, weight))
    
    end_var_dict = {}
    
    expect_gurobi_var = gurobi_model.addVar(name='expectation')
    expectation_term = 0
    for (state, rew) in over_e_max_in_terms:
        expectation_term += get_in_out_term(in_term_dict, state, rew) * (rew + emax_statewise_dict[state])
    
    gurobi_model.addConstr(expectation_term == expect_gurobi_var, "expectation")
    
    penalty_variables = {}
    
    objective_term = expect_gurobi_var
    for (state, rew) in over_e_max_in_terms:
        gurobi_penalty_var = get_gurobi_penalty_var(gurobi_model, penalty_variables, state, rew)
        term = rew + emax_statewise_dict[state] - expect_gurobi_var
        gurobi_model.addConstr(gurobi_penalty_var + term >= 0, "penalty_abs0__s{}__w{}".format(state, rew))
        gurobi_model.addConstr(gurobi_penalty_var - term >= 0, "penalty_abs1__s{}__w{}".format(state, rew))
        objective_term -= lambda_factor * gurobi_penalty_var
    
    gurobi_model.setObjective(0 - objective_term)
    
    gurobi_model.optimize()
    
    with open("MADPE_solution.txt", "w") as MADPE_solution_file:
        for v in gurobi_model.getVars():
            MADPE_solution_file.write(f"{v.VarName} {v.X:g}\n")
        MADPE_solution_file.write(f"Obj: {gurobi_model.ObjVal:g}\n")
    


def get_emax_gurobi_var_name(state_id):
    return "E_max_{}".format(state_id)

def get_emax_gurobi_var(gurobi_model_emax, emax_state_map, state_id):
    if state_id in emax_state_map:
        return emax_state_map[state_id]
    x = gurobi_model_emax.addVar(name=get_emax_gurobi_var_name(state_id))
    emax_state_map[state_id] = x
    return x


def solve_statewise_max_expectation(gurobi_model_emax, emax_state_map, transition_table, transition_rewards, state_to_labels_dict, goal_label):
    
    print("Solving Emax statewise...")
    #print(transition_table)
    
    #print("show rewards...")
    #print(transition_rewards)
    
    #print("show labels...")
    #print(state_to_labels_dict)
    
    goal_states = set()
    
    for state in state_to_labels_dict:
        if goal_label in state_to_labels_dict[state]:
            goal_states.add(state)
    
    #constraints:
    for from_state in transition_table:
        if from_state in goal_states:
            gurobi_variable = get_emax_gurobi_var(gurobi_model_emax, emax_state_map, from_state)
            gurobi_model_emax.addConstr(gurobi_variable >= 0, "from__{}__goal".format(from_state))
            
        else:
            for action in transition_table[from_state]:
                
                #E_max_{from_state} >= SUM(p * (E_max_{next_state} + rew(s, alpha, t) ))
                gurobi_variable = get_emax_gurobi_var(gurobi_model_emax, emax_state_map, from_state)
                right_term = 0
                
                for to_state in transition_table[from_state][action]:
                    p = transition_table[from_state][action][to_state]
                    e_max_next = get_emax_gurobi_var(gurobi_model_emax, emax_state_map, to_state)
                    transition_rew = get_transition_reward(transition_rewards, from_state, action, to_state)
                    right_term = right_term + p * (e_max_next + transition_rew)
                
                gurobi_model_emax.addConstr(gurobi_variable >= right_term, "from__{}__action__{}".format(from_state, action))
    
    # objective for minimization:
    obj_term = 0
    for var_id in emax_state_map:
        obj_term = obj_term + emax_state_map[var_id]
    
    gurobi_model_emax.setObjective(obj_term)
    
    gurobi_model_emax.optimize()
    
    var_name_solution_map = {}
    for v in gurobi_model_emax.getVars():
        var_name_solution_map[v.VarName] = v.X
    
    #for v in gurobi_model_emax.getVars():
    #    print(f"{v.VarName} {v.X:g}")
    
    #print(f"Obj: {gurobi_model_emax.ObjVal:g}")
    #print()
    #print()
    #print("Solving Emax statewise   ...DONE")
    
    solution_as_dict = {}
    
    for from_state in transition_table:
        solution_as_dict[from_state] = var_name_solution_map[get_emax_gurobi_var_name(from_state)]
    
    #print(solution_as_dict)
    return solution_as_dict

def main():
    
    start = time.time()
    
    test_string = 'abcdef'
    
    transition_table = {} #state mapped to { action mapped to { succ state mapped to probability }}
    label_dict = {} # label code number to name
    state_to_labels_dict = {} # state to set of labels
    transition_rewards = {}
    
    reverse_label_dict = make_reverse_label_dict(label_dict)
    
    
    count_states = read_states()
    global_e_max = prism_log_read_e_max()
    action_index_count = read_transitions(transition_table)
    read_labels(label_dict, state_to_labels_dict)
    read_transition_rewards(transition_rewards)
    
    
    gurobi_model_emax = gp.Model("emax_statewise")
    emax_state_map =  {}
    goal_label = '2'
    emax_statewise_dict = solve_statewise_max_expectation(gurobi_model_emax, emax_state_map, transition_table, transition_rewards, state_to_labels_dict, goal_label)
    
    
    #print(transition_table)
    
    print("#   COUNT_STATES  {}".format(count_states))
    print("#   E_max  {}".format(global_e_max))
    print("#   Action_index_max  {}".format(action_index_count))
    
    
    print("import gurobipy as gp")
    print("from gurobipy import GRB")
    print()
    print("# Create a new model")
    print('m = gp.Model("qp")')
    
    gurobi_model = gp.Model("qp")
    all_variables = {}
    build_linear_constraints(gurobi_model, all_variables, count_states, state_to_labels_dict, label_dict, transition_table, transition_rewards, global_e_max, emax_statewise_dict)
    
    end = time.time()
    print("Total time: {}".format(end - start))




if __name__ == "__main__":
    main()
