/*=============================================================================
# Filename: PlanTree.cpp
# Author: Linglin Yang
# Mail: fyulingi@gmail.com
=============================================================================*/


#include "PlanTree.h"
using namespace std;

// for yanglei's paper, will not used in gStore v1.0
string NodeJoinTypeStr(NodeJoinType node_join_type)
{
  switch (node_join_type) {
    case NodeJoinType::JoinANode: return "JoinANode";
    case NodeJoinType::JoinTwoTable: return "JoinTwoTable";
    case NodeJoinType::LeafNode: return "LeafNode";
  }
  return "NodeJoinType::No Exist";
}


OldPlanTree::OldPlanTree(int first_node) {
    root_node = new Old_Tree_Node(first_node);
}

OldPlanTree::OldPlanTree(OldPlanTree *last_plantree, int next_node) {
    root_node = new Old_Tree_Node();
    root_node->joinType = NodeJoinType::JoinANode;
    root_node->left_node = new Old_Tree_Node(last_plantree->root_node);
    root_node->right_node = new Old_Tree_Node(next_node);
}

OldPlanTree::OldPlanTree(OldPlanTree *left_plan, OldPlanTree *right_plan) {
    root_node = new Old_Tree_Node();
    root_node->joinType = NodeJoinType::JoinTwoTable;

    root_node->left_node = new Old_Tree_Node(left_plan->root_node);
    root_node->right_node = new Old_Tree_Node(right_plan->root_node);
}

OldPlanTree::OldPlanTree(const vector<int> nodes_order) {
	root_node = new Old_Tree_Node(nodes_order[0]);
	for(int i = 1; i < nodes_order.size(); ++i){
		Old_Tree_Node* last_plan = root_node;
		root_node = new Old_Tree_Node();
		root_node->joinType = NodeJoinType::JoinANode;
		root_node->left_node = last_plan;
		root_node->right_node = new Old_Tree_Node(nodes_order[i]);
	}
}

void OldPlanTree::delete_tree_node(Old_Tree_Node *root_node) {
    if(root_node == nullptr){
        return;
    }

    delete_tree_node(root_node->left_node);
    delete_tree_node(root_node->right_node);
    delete root_node;
}

OldPlanTree::~OldPlanTree() {
    delete_tree_node(root_node);
}


void OldPlanTree::print_tree_node(Old_Tree_Node* node, BasicQuery* basicquery){
	if(node == nullptr){
		return;
	}

	switch (node->joinType) {
		case NodeJoinType::LeafNode:
			cout << basicquery->getVarName(node->node_to_join);
			break;
		case NodeJoinType::JoinTwoTable:
			cout << "Binary join(";
			break;
		case NodeJoinType::JoinANode:
			cout << "WCO join(";
			break;
	}

	if(node->left_node!= nullptr){
		print_tree_node(node->left_node, basicquery);
		cout << ",";
		print_tree_node(node->right_node, basicquery);
	}

	if(node->joinType!=NodeJoinType::LeafNode){
		cout << ")";
	}
}

void OldPlanTree::print(BasicQuery* basicquery) {
	cout << "Plan: ";
	print_tree_node(root_node, basicquery);
	cout << ", cost: "<<this->plan_cost;
	cout << endl;
}


// codes below will be used in gStore v1.0

JoinMethod Tree_node::get_join_method(bool s_is_var, bool p_is_var, bool o_is_var) {

	int var_num = 0;
	if(s_is_var) var_num += 1;
	if(p_is_var) var_num += 1;
	if(o_is_var) var_num += 1;

	// todo: this is a question, what about a triple like ?s ?p ?o ?
	if(var_num == 2){
		if(!s_is_var) return JoinMethod::s2po;
		if(!o_is_var) return JoinMethod::o2ps;
		if(!p_is_var) return JoinMethod::p2so;
	} else if(var_num == 1){
		if(s_is_var) return JoinMethod::po2s;
		if(o_is_var) return JoinMethod::sp2o;
		if(p_is_var) return JoinMethod::so2p;
	} else{
		cout << "error: var_num not equal to 1 or 2" << endl;
		exit(-1);
	}
}

Tree_node::Tree_node(unsigned int node_id, BGPQuery *bgpquery, bool is_first_node) {

	// codes below are for is_first_node == true, I will remove param is first_node from this fun.
	// If node_id is not the first node,
	// then the tree_node should be generated by some funs like
	// Tree_node(unsigned node_id, set<unsigned> already_id);
	auto edge_info = make_shared<vector<EdgeInfo>>();
	auto edge_constant_info = make_shared<vector<EdgeConstantInfo>>();

	auto var_descrip = bgpquery->get_vardescrip_by_id(node_id);

	auto var_degree = var_descrip->degree_;

	if(var_descrip->var_type_ == VarDescriptor::VarType::Entity){
		for(unsigned i_th_edge = 0; i_th_edge < var_degree; ++ i_th_edge){
			if(var_descrip->so_edge_pre_type_[i_th_edge] != VarDescriptor::PreType::VarPreType ||
					var_descrip->so_edge_nei_type_[i_th_edge] != VarDescriptor::EntiType::VarEntiType){
				unsigned triple_id = var_descrip->so_edge_index_[i_th_edge];

				unsigned constant_num = 0;
				if(bgpquery->s_is_constant_[triple_id]) constant_num += 1;
				if(bgpquery->p_is_constant_[triple_id]) constant_num += 1;
				if(bgpquery->o_is_constant_[triple_id]) constant_num += 1;

				JoinMethod join_method;
				if(constant_num == 2){
					if(var_descrip->so_edge_type_[i_th_edge] == Util::EDGE_IN)
						join_method = JoinMethod::sp2o;
					else
						join_method = JoinMethod::po2s;
				} else{
					if(bgpquery->p_is_constant_[triple_id]){
						if(var_descrip->so_edge_type_[i_th_edge] == Util::EDGE_IN)
							// ?s p ?o, ?o is the var_descrip
							join_method = JoinMethod::p2o;
						else
							join_method = JoinMethod::p2s;
					} else{
						if(var_descrip->so_edge_type_[i_th_edge] == Util::EDGE_IN)
							// s ?p ?o, ?o is the var_descrip
							join_method = JoinMethod::s2o;
						else
							join_method = JoinMethod::o2s;
					}
				}

				edge_info->emplace_back(bgpquery->s_id_[triple_id], bgpquery->p_id_[triple_id], bgpquery->o_id_[triple_id], join_method);
				edge_constant_info->emplace_back(bgpquery->s_is_constant_[triple_id], bgpquery->p_is_constant_[triple_id], bgpquery->o_is_constant_[triple_id]);

			}

		}
	} else{
		for(unsigned i_th_edge = 0; i_th_edge < var_degree; ++ i_th_edge){
			if(var_descrip->s_type_[i_th_edge] != VarDescriptor::EntiType::VarEntiType &&
					var_descrip->o_type_[i_th_edge] != VarDescriptor::EntiType::VarEntiType){
				unsigned triple_id = var_descrip->pre_edge_index_[i_th_edge];
				unsigned constant_num = 0;
				if(bgpquery->s_is_constant_[triple_id]) constant_num += 1;
				if(bgpquery->o_is_constant_[triple_id]) constant_num += 1;

				JoinMethod join_method;
				if(constant_num == 2){

					join_method = JoinMethod::so2p;
				} else{
					if(bgpquery->s_is_constant_[triple_id])
						join_method = JoinMethod::s2p;
					else
						join_method = JoinMethod::o2p;
				}


				edge_info->emplace_back(bgpquery->s_id_[triple_id], bgpquery->p_id_[triple_id], bgpquery->o_id_[triple_id], join_method);
				edge_constant_info->emplace_back(bgpquery->s_is_constant_[triple_id], bgpquery->p_is_constant_[triple_id], bgpquery->o_is_constant_[triple_id]);

			}

		}
	}

	auto one_step_join_node = make_shared<FeedOneNode>();
	one_step_join_node->node_to_join_ = node_id;
	one_step_join_node->edges_ = edge_info;
	one_step_join_node->edges_constant_info_ = edge_constant_info;

	node = make_shared<StepOperation>();
	// if(is_first_node){
	// 	node->join_type_ = StepOperation::JoinType::GenerateCandidates;
	// 	node->edge_filter_ = one_step_join_node;
	//
	// } else{
	// 	node->join_type_ = StepOperation::JoinType::JoinNode;
	// 	node->join_node_ = one_step_join_node;
	// }

	node->join_type_ = StepOperation::JoinType::JoinNode;
	node->join_node_ = one_step_join_node;
	left_node = nullptr;
	right_node = nullptr;
}

Tree_node::Tree_node(unsigned node_id, set<unsigned> already_in, BGPQuery *bgpquery) {
	auto edge_info = make_shared<vector<EdgeInfo>>();
	auto edge_constant_info = make_shared<vector<EdgeConstantInfo>>();

	auto var_descrip = bgpquery->get_vardescrip_by_id(node_id);

	auto var_degree = var_descrip->degree_;

	if(var_descrip->var_type_ == VarDescriptor::VarType::Entity){
		for(unsigned i_th_edge = 0; i_th_edge < var_degree; ++ i_th_edge){

			// first deal with const edge
			// TODO: zhou yuqi can remove this
			if(var_descrip->so_edge_pre_type_[i_th_edge] == VarDescriptor::PreType::ConPreType &&
					var_descrip->so_edge_nei_type_[i_th_edge] == VarDescriptor::EntiType::ConEntiType){
				// deal with ?s p o or s p ?o
				// todo: constant edge should be dealed first
				unsigned triple_id = var_descrip->so_edge_index_[i_th_edge];

				JoinMethod join_method;
				if(var_descrip->so_edge_type_[i_th_edge] == Util::EDGE_IN)
					// s p ?o
					join_method = JoinMethod::sp2o;
				else
					// ?s p o
					join_method = JoinMethod::po2s;

				edge_info->emplace_back(bgpquery->s_id_[triple_id], bgpquery->p_id_[triple_id], bgpquery->o_id_[triple_id], join_method);
				edge_constant_info->emplace_back(bgpquery->s_is_constant_[triple_id], bgpquery->p_is_constant_[triple_id], bgpquery->o_is_constant_[triple_id]);

			}

			if((var_descrip->so_edge_pre_type_[i_th_edge] == VarDescriptor::PreType::VarPreType &&
					already_in.count(var_descrip->so_edge_pre_id_[i_th_edge]) == 1)  ||
				(var_descrip->so_edge_nei_type_[i_th_edge] == VarDescriptor::EntiType::VarEntiType &&
			   		already_in.count(var_descrip->so_edge_nei_[i_th_edge] == 1))){

				// deal with edge linked with var in already_in
				unsigned triple_id = var_descrip->so_edge_index_[i_th_edge];

				bool s_is_const = (bgpquery->s_is_constant_[triple_id]);
				bool p_is_const = (bgpquery->p_is_constant_[triple_id]);
				bool o_is_const = (bgpquery->o_is_constant_[triple_id]);

				unsigned const_num = 0;
				if(s_is_const) const_num += 1;
				if(p_is_const) const_num += 1;
				if(o_is_const) const_num += 1;

				JoinMethod join_method;

				if(const_num == 1){
					// var_num == 2, var_descrip is a var, so the another var must be in already_in
					if(var_descrip->so_edge_type_[i_th_edge] == Util::EDGE_IN)
						// (s ?p ?o) or (?s p ?o), ?o is the var_descrip
						join_method = JoinMethod::sp2o;
					else
						// (?s ?p o) or (?s p ?o), ?s is the var_descrip
						join_method = JoinMethod::po2s;
				} else{
					// ?s ?p ?o
					if(already_in.count(bgpquery->p_id_[triple_id]) == 1){
						// ?p in already_in
						if(var_descrip->so_edge_type_[i_th_edge] == Util::EDGE_IN)
							// var_descrip is ?o
							if(already_in.count(bgpquery->s_id_[triple_id]) == 1)
								// ?s in already_in
								join_method = JoinMethod::sp2o;
							else
								// ?s not in already_in
								join_method = JoinMethod::p2o;
						else
							// var_descrip is ?s
							if(already_in.count(bgpquery->o_id_[triple_id]) == 1)
								// ?o is in already_in
								join_method = JoinMethod::po2s;
							else
								// ?o not in already_in
								join_method = JoinMethod::p2s;
					} else{
						// ?p not in already_in
						if(var_descrip->so_edge_type_[i_th_edge] == Util::EDGE_IN)
							// var_descrip is ?o
							if(already_in.count(bgpquery->s_id_[triple_id]) == 1)
								// ?s in already_in
								join_method = JoinMethod::s2o;
							else{
								// ?s not in already_in,
								// then ?p, ?s both not in already, conflict with the if condition
								cout << "error: if error!" << endl;
								exit(-1);
							}
						else
							// var_descrip is ?s
							if(already_in.count(bgpquery->o_id_[triple_id]) == 1)
								// ?o is in already_in
								join_method = JoinMethod::o2s;
							else{
								// ?o not in already_in,
								// then ?p, ?s both not in already, conflict with the if condition
								cout << "error: if error!" << endl;
								exit(-1);
							}
					}
				}

				edge_info->emplace_back(bgpquery->s_id_[triple_id], bgpquery->p_id_[triple_id], bgpquery->o_id_[triple_id], join_method);
				edge_constant_info->emplace_back(bgpquery->s_is_constant_[triple_id], bgpquery->p_is_constant_[triple_id], bgpquery->o_is_constant_[triple_id]);

			}

		}
	} else{
		// node_id is a pre var
		// TODO: zhou yuqi can remove this
		for(unsigned i_th_edge = 0; i_th_edge < var_degree; ++ i_th_edge){
			if(var_descrip->s_type_[i_th_edge] == VarDescriptor::EntiType::ConEntiType &&
					var_descrip->o_type_[i_th_edge] == VarDescriptor::EntiType::ConEntiType){

				unsigned triple_id = var_descrip->pre_edge_index_[i_th_edge];

				JoinMethod join_method = JoinMethod::so2p;

				edge_info->emplace_back(bgpquery->s_id_[triple_id], bgpquery->p_id_[triple_id], bgpquery->o_id_[triple_id], join_method);
				edge_constant_info->emplace_back(bgpquery->s_is_constant_[triple_id], bgpquery->p_is_constant_[triple_id], bgpquery->o_is_constant_[triple_id]);

			}

			if((var_descrip->s_type_[i_th_edge] == VarDescriptor::EntiType::VarEntiType &&
					already_in.count(var_descrip->s_id_[i_th_edge]) == 1) ||
				(var_descrip->o_type_[i_th_edge] == VarDescriptor::EntiType::VarEntiType &&
					already_in.count(var_descrip->o_id_[i_th_edge]) == 1)){

				unsigned triple_id = var_descrip->pre_edge_index_[i_th_edge];

				unsigned const_num = 0;
				bool s_is_const = bgpquery->s_is_constant_[triple_id];
				bool o_is_const = bgpquery->o_is_constant_[triple_id];

				if(s_is_const) const_num += 1;
				if(o_is_const) const_num += 1;

				JoinMethod join_method;

				if(const_num == 1){
					// (s ?p ?o) or (?s ?p o), ?p is the var_descrip, ?s or ?o in already_in
					join_method = JoinMethod::so2p;
				} else{
					// const_num == 0, ?s ?p ?o, ?p is the var_descrip
					if(already_in.count(var_descrip->s_id_[i_th_edge]) == 1)
						if(already_in.count(var_descrip->o_id_[i_th_edge]) == 1)
							// ?s ?p ?o, ?s and ?o are in already_in
							join_method = JoinMethod::so2p;
						else
							join_method = JoinMethod::s2p;
					else
						if(already_in.count(var_descrip->o_id_[i_th_edge]) == 1)
							join_method = JoinMethod::o2p;
						else{
							// ?s ?p ?o, ?s and ?o neither in already_in, conflict with if condition
							cout << "error: if error!" << endl;
							exit(-1);
						}
				}

				edge_info->emplace_back(bgpquery->s_id_[triple_id], bgpquery->p_id_[triple_id], bgpquery->o_id_[triple_id], join_method);
				edge_constant_info->emplace_back(bgpquery->s_is_constant_[triple_id], bgpquery->p_is_constant_[triple_id], bgpquery->o_is_constant_[triple_id]);

			}

		}
	}

	auto one_step_join_node = make_shared<FeedOneNode>();
	one_step_join_node->node_to_join_ = node_id;
	one_step_join_node->edges_ = edge_info;
	one_step_join_node->edges_constant_info_ = edge_constant_info;

	node = make_shared<StepOperation>();
	node->join_type_ = StepOperation::JoinType::JoinNode;
	node->join_node_ = one_step_join_node;
}

PlanTree::PlanTree(int first_node) {
	root_node = new Tree_node(first_node);
}

PlanTree::PlanTree(PlanTree *last_plantree, int next_node) {
	root_node = new Tree_node();
	root_node->joinType = NodeJoinType::JoinANode;
	root_node->left_node = new Tree_node(last_plantree->root_node);
	root_node->right_node = new Tree_node(next_node);
}

JoinMethod PlanTree::get_join_strategy(BGPQuery *bgp_query, shared_ptr<VarDescriptor> var_descrip, unsigned int edge_index, bool join_two_node) {
	if(join_two_node){
		// auto pre_decrip = bgp_query->get_vardescrip_by_id(var_descrip->so_edge_pre_id_[edge_index]);
		// if(pre_decrip->degree_ == 1 and pre_decrip->selected_ == false)
		// 	return var_descrip->so_edge_type_[edge_index] == Util::EDGE_IN ? JoinMethod::s2o : JoinMethod::o2s;
		// else
			return var_descrip->so_edge_type_[edge_index] == Util::EDGE_IN ? JoinMethod::s2po : JoinMethod::o2ps;
	} else{
		cout << "not support" << endl;
		exit(-1);
	}
}


// We want to join ?o.
// case1 s p ?o. This is done in candidate generation.
// case2 s ?p ?o. ?p is degree_one, then s2o, else s2po(?p not alrealy) or s2po(?p already).
// case3 ?s p ?o. this should be done first by sp2o.
// case4 ?s ?p ?o. ?p is degree_one pre_var(only exists in this triple), then we only need s2o on s.
// 	      If ?p is not degree one, then join two_node(?p not already) by s2po, or join one_node(?p already) by sp2o.
// First sp2o, then join other pre_var by join_a_node
PlanTree::PlanTree(PlanTree *last_plantree, BGPQuery *bgpquery, unsigned next_node) {
	// cout << endl << endl << "next node id : " << next_node << endl;

	auto var_descrip = bgpquery->get_vardescrip_by_id(next_node);
	// todo: 这个构造函数用得对吗？
	root_node = last_plantree->root_node;
	already_so_var = last_plantree->already_so_var;
	already_joined_pre_var = last_plantree->already_joined_pre_var;

	auto join_a_node_edge_info = make_shared<vector<EdgeInfo>>();
	auto join_a_node_edge_const_info = make_shared<vector<EdgeConstantInfo>>();
	vector<unsigned> need_join_two_nodes_index;

	for(int i = 0; i < var_descrip->degree_; ++i){

		// ?s (?)p ?o, join ?o, however, ?s is not already, then donnot need deal with this triple.
		// This triple will be dealed when join ?s next time.
		// After this if, we can believe (?)s is ready for join.
		if(var_descrip->so_edge_nei_type_[i] == VarDescriptor::EntiType::VarEntiType and
		find(already_so_var.begin(), already_so_var.end(), var_descrip->so_edge_nei_[i]) == already_so_var.end())
			continue;

		// s p ?o. This edge will be done in candidate generation
		if(var_descrip->so_edge_nei_type_[i] == VarDescriptor::EntiType::ConEntiType and
		var_descrip->so_edge_pre_type_[i] == VarDescriptor::PreType::ConPreType)
			continue;

		if(var_descrip->so_edge_pre_type_[i] == VarDescriptor::PreType::ConPreType or
		find(already_joined_pre_var.begin(), already_joined_pre_var.end(), var_descrip->so_edge_pre_id_[i]) != already_joined_pre_var.end()){
			// s ?p ?o, ?p ready.
			// ?s p ?o, ?s ready.
			// ?s ?p ?o, ?s ready, ?p ready.
			unsigned edge_index = var_descrip->so_edge_index_[i];
			join_a_node_edge_info->emplace_back(bgpquery->s_id_[edge_index], bgpquery->p_id_[edge_index], bgpquery->o_id_[edge_index],
												(var_descrip->so_edge_type_[i] == Util::EDGE_IN ? JoinMethod::sp2o : JoinMethod::po2s));
			join_a_node_edge_const_info->emplace_back(bgpquery->s_is_constant_[edge_index], bgpquery->p_is_constant_[edge_index], bgpquery->o_is_constant_[edge_index]);
		} else{
			// s ?p ?o, ?p not ready.
			// Or ?s ?p ?o, ?s ready, ?p not ready.
			need_join_two_nodes_index.emplace_back(i);
			// todo: degreeone!

		}

	}

	// cout << "edge_info size: " << join_a_node_edge_info->size() << endl;
	// cout << "edge_const size: " << join_a_node_edge_const_info->size() << endl;
	// cout << "need_join_two_nodes size: " << need_join_two_nodes_index.size() << endl;
	if(!join_a_node_edge_info->empty()){
		shared_ptr<FeedOneNode> join_a_node = make_shared<FeedOneNode>(next_node, join_a_node_edge_info, join_a_node_edge_const_info);
		// shared_ptr<StepOperation> join_one_node = make_shared<StepOperation>()
		Tree_node *new_tree_node = new Tree_node(make_shared<StepOperation>(StepOperation::JoinType::JoinNode, join_a_node,
																			nullptr, nullptr, nullptr));
		new_tree_node->left_node = root_node;
		root_node = new_tree_node;
	}

	if(!need_join_two_nodes_index.empty()){

		unsigned beg = 0;
		if(join_a_node_edge_info->empty()){
			unsigned edge_index = var_descrip->so_edge_index_[need_join_two_nodes_index[0]];
			shared_ptr<FeedTwoNode> join_two_nodes = make_shared<FeedTwoNode>(var_descrip->so_edge_pre_id_[need_join_two_nodes_index[0]], next_node,
																			  EdgeInfo(bgpquery->s_id_[edge_index], bgpquery->p_id_[edge_index], bgpquery->o_id_[edge_index],
																					   get_join_strategy(bgpquery, var_descrip, need_join_two_nodes_index[0])),
																					   EdgeConstantInfo(bgpquery->s_is_constant_[edge_index], bgpquery->p_is_constant_[edge_index], bgpquery->o_is_constant_[edge_index]));
			Tree_node *new_tree_node = new Tree_node(make_shared<StepOperation>(StepOperation::JoinType::JoinTwoNodes,
																				nullptr, join_two_nodes, nullptr,nullptr));
			new_tree_node->left_node = root_node;
			root_node = new_tree_node;
			already_joined_pre_var.emplace_back(bgpquery->p_id_[edge_index]);
			beg = 1;

		}

		for(unsigned i = beg; i < need_join_two_nodes_index.size(); ++i){
			unsigned edge_index = var_descrip->so_edge_index_[need_join_two_nodes_index[i]];
			if(find(already_joined_pre_var.begin(), already_joined_pre_var.end(), bgpquery->p_id_[edge_index]) != already_joined_pre_var.end()){
				continue;
			} else{
				auto pre_var_descrip = bgpquery->get_vardescrip_by_id(var_descrip->so_edge_pre_id_[need_join_two_nodes_index[i]]);
				if(!pre_var_descrip->selected_ and pre_var_descrip->degree_ == 1){
					shared_ptr<FeedOneNode> edge_check = make_shared<FeedOneNode>(pre_var_descrip->id_,
																				  EdgeInfo(bgpquery->s_id_[edge_index],bgpquery->p_id_[edge_index],bgpquery->p_id_[edge_index],JoinMethod::so2p),
																				  EdgeConstantInfo(bgpquery->s_is_constant_[edge_index], bgpquery->p_is_constant_[edge_index], bgpquery->o_is_constant_[edge_index]));
					Tree_node *new_edge_check = new Tree_node(make_shared<StepOperation>(StepOperation::JoinType::EdgeCheck,
																						 nullptr, nullptr, nullptr, edge_check));

					new_edge_check->left_node = root_node;
					root_node = new_edge_check;
				} else{
					// selected_ == true or degree >= 2
					shared_ptr<vector<EdgeInfo>> edges_info = make_shared<vector<EdgeInfo>>();
					shared_ptr<vector<EdgeConstantInfo>> edges_const = make_shared<vector<EdgeConstantInfo>>();

					for(int j = i; j < need_join_two_nodes_index.size(); ++j){
						// need_join_two_nodes_index include only edges with pre_var
						if(var_descrip->so_edge_pre_id_[j] != pre_var_descrip->id_){// or var_descrip->so_edge_pre_type_[j] == VarDescriptor::PreType::ConPreType){
							continue;
						}
						edge_index = var_descrip->so_edge_index_[j];
						edges_info->emplace_back(bgpquery->s_id_[edge_index], bgpquery->p_id_[edge_index], bgpquery->o_id_[edge_index], JoinMethod::so2p);
						edges_const->emplace_back(bgpquery->s_is_constant_[edge_index], bgpquery->p_is_constant_[edge_index], bgpquery->o_is_constant_[edge_index]);
					}
					shared_ptr<FeedOneNode> join_pre_node = make_shared<FeedOneNode>(pre_var_descrip->id_, edges_info, edges_const);
					Tree_node *new_join_a_node = new Tree_node(
							make_shared<StepOperation>(StepOperation::JoinType::JoinNode, join_pre_node, nullptr, nullptr, nullptr));

					new_join_a_node->left_node = root_node;
					root_node = new_join_a_node;
					already_joined_pre_var.emplace_back(pre_var_descrip->id_);
				}
			}
		}
	}

	already_so_var.emplace_back(next_node);

	// cout << endl << endl;

}

PlanTree::PlanTree(PlanTree *left_plan, PlanTree *right_plan) {
	root_node = new Tree_node();
	root_node->joinType = NodeJoinType::JoinTwoTable;

	root_node->left_node = new Tree_node(left_plan->root_node);
	root_node->right_node = new Tree_node(right_plan->root_node);
}

// todo: check these three cases would not apprear
// After join left and right table, we need consider some edges between them:
// case1 ?s p ?o, ?s in left, ?o in right
// case2 ?s ?p ?o, join ?p(not appear in left and right) or edgecheck(?p ready)
// case3 ?s1 ?p ?o1 in left, ?s2 ?p ?o2 in right, this can be done in JoinTwoTable
// todo: 用Plangenerator的参数，直接算出哪些是不被join的
PlanTree::PlanTree(PlanTree *left_plan, PlanTree *right_plan, BGPQuery *bgpquery, set<unsigned> &join_nodes) {

	shared_ptr<vector<unsigned>> public_variables = make_shared<vector<unsigned>>(join_nodes.begin(), join_nodes.end());

	for(auto x : left_plan->already_so_var){
		if(find(right_plan->already_so_var.begin(), right_plan->already_so_var.end(), x) != right_plan->already_so_var.end()){
			public_variables->emplace_back(x);
		}
	}

	already_so_var = left_plan->already_so_var;
	for(auto x : right_plan->already_so_var)
		if(find(already_so_var.begin(), already_so_var.end(), x) == already_so_var.end())
			already_so_var.emplace_back(x);

	already_joined_pre_var = left_plan->already_joined_pre_var;
	for(auto x : right_plan->already_joined_pre_var)
		if(find(already_joined_pre_var.begin(), already_joined_pre_var.end(), x) == already_joined_pre_var.end())
			already_joined_pre_var.emplace_back(x);


	for(auto x : left_plan->already_joined_pre_var){
		if(find(right_plan->already_joined_pre_var.begin(), right_plan->already_joined_pre_var.end(), x) != right_plan->already_so_var.end()){
			public_variables->emplace_back(x);
		}
	}

	root_node = new Tree_node(make_shared<StepOperation>(StepOperation::JoinType::JoinTable,
												nullptr, nullptr, make_shared<JoinTwoTable>(public_variables), nullptr));


	root_node->left_node = left_plan->root_node;
	root_node->right_node = right_plan->root_node;


}

PlanTree::PlanTree(unsigned int first_node, BGPQuery *bgpquery, bool used_in_random_plan) {
	root_node = new Tree_node(first_node, bgpquery, true);
	// todo: if used in normal plan (invoked by optimizer), then should give its cost
}

// todo: not complete
PlanTree::PlanTree(unsigned int first_node, BGPQuery *bgpquery) {
	// root_node = new Tree_node(first_node, bgpquery, true);
	root_node = new Tree_node();
	auto join_node = make_shared<FeedOneNode>(first_node, make_shared<vector<EdgeInfo>>(), make_shared<vector<EdgeConstantInfo>>());
	root_node->node = make_shared<StepOperation>(StepOperation::JoinType::JoinNode, join_node, nullptr, nullptr, nullptr);
	already_so_var = {first_node};
	// todo: if used in normal plan (invoked by optimizer), then should give its cost
}

PlanTree::PlanTree(shared_ptr<StepOperation> &first_node) {
	root_node = new Tree_node(first_node);
}


/**
 * Build plantree for adding one node (FeedOneNode type) to last_plan_tree
 * @param last_plan_tree plan tree need adding one node
 * @param next_join_var_id the id of added node
 * @param already_id node ids in last_plan_tree
 * @param bgpquery bgp information of the query
 */
PlanTree::PlanTree(PlanTree *last_plan_tree, unsigned int next_join_var_id, set<unsigned> already_id, BGPQuery *bgpquery) {
	root_node = new Tree_node(next_join_var_id, already_id, bgpquery);
	root_node->left_node = new Tree_node(last_plan_tree->root_node, true);
	root_node->right_node = nullptr;

}

// ?s p ?o joinanode sp2o
// ?s ?p ?o, ?p already, sp2o joinanode
// below: ?p not ready
// ?s ?p ?o, ?p degree1 notselected, joinanode s2o
// ?s ?p ?o, ?p degree1 selected or degree > 1, jointwonode s2po
void PlanTree::add_satellitenode(BGPQuery *bgpquery, unsigned int satellitenode_id) {
	;
	auto var_descrip = bgpquery->get_vardescrip_by_id(satellitenode_id);
	unsigned edge_index = var_descrip->so_edge_index_[0];


	if(var_descrip->so_edge_pre_type_[0] == VarDescriptor::PreType::ConPreType or
			find(already_joined_pre_var.begin(), already_joined_pre_var.end(), var_descrip->so_edge_pre_id_[0])
				!= already_joined_pre_var.end()){

		auto join_a_node_edge_info = make_shared<vector<EdgeInfo>>();
		auto join_a_node_edge_const_info = make_shared<vector<EdgeConstantInfo>>();

		join_a_node_edge_info->emplace_back(bgpquery->s_id_[edge_index], bgpquery->p_id_[edge_index], bgpquery->o_id_[edge_index],
											var_descrip->so_edge_type_[0] == Util::EDGE_IN ? JoinMethod::sp2o : JoinMethod::po2s);
		join_a_node_edge_const_info->emplace_back(bgpquery->s_is_constant_[edge_index], bgpquery->p_is_constant_[edge_index], bgpquery->o_is_constant_[edge_index]);

		auto join_node = make_shared<FeedOneNode>(satellitenode_id,join_a_node_edge_info, join_a_node_edge_const_info);
		Tree_node *new_join_node = new Tree_node(make_shared<StepOperation>(StepOperation::JoinType::JoinNode,
																			join_node, nullptr, nullptr, nullptr));

		new_join_node->left_node = root_node;
		root_node = new_join_node;
		already_so_var.emplace_back(satellitenode_id);
	} else{
		auto pre_var_descrip = bgpquery->get_vardescrip_by_id(var_descrip->so_edge_pre_id_[0]);
		if(!pre_var_descrip->selected_ and pre_var_descrip->degree_ == 1){
			auto join_a_node_edge_info = make_shared<vector<EdgeInfo>>();
			auto join_a_node_edge_const_info = make_shared<vector<EdgeConstantInfo>>();

			join_a_node_edge_info->emplace_back(bgpquery->s_id_[edge_index], bgpquery->p_id_[edge_index], bgpquery->o_id_[edge_index],
												var_descrip->so_edge_type_[0] == Util::EDGE_IN ? JoinMethod::s2o : JoinMethod::p2s);
			join_a_node_edge_const_info->emplace_back(bgpquery->s_is_constant_[edge_index], bgpquery->p_is_constant_[edge_index], bgpquery->o_is_constant_[edge_index]);

			auto join_node = make_shared<FeedOneNode>(satellitenode_id,join_a_node_edge_info, join_a_node_edge_const_info);
			Tree_node *new_join_node = new Tree_node(make_shared<StepOperation>(StepOperation::JoinType::JoinNode,
																				join_node, nullptr, nullptr, nullptr));
			new_join_node->left_node = root_node;
			root_node = new_join_node;
			already_so_var.emplace_back(satellitenode_id);

		}else{

			auto join_two_node = make_shared<FeedTwoNode>(pre_var_descrip->id_, satellitenode_id,
														  EdgeInfo(bgpquery->s_id_[edge_index], bgpquery->p_id_[edge_index], bgpquery->o_id_[edge_index],
																   var_descrip->so_edge_type_[0] == Util::EDGE_IN ? JoinMethod::s2po : JoinMethod::o2ps),
														  EdgeConstantInfo(bgpquery->s_is_constant_[edge_index], bgpquery->p_is_constant_[edge_index], bgpquery->o_is_constant_[edge_index]));
			Tree_node *new_join_node = new Tree_node(make_shared<StepOperation>(StepOperation::JoinType::JoinTwoNodes,
																				nullptr, join_two_node, nullptr, nullptr));

			new_join_node->left_node = root_node;
			root_node = new_join_node;
			already_joined_pre_var.emplace_back(pre_var_descrip->id_);
			already_so_var.emplace_back(satellitenode_id);
		}
	}
}

PlanTree::PlanTree(const vector<int> nodes_order) {
	root_node = new Tree_node(nodes_order[0]);
	for(int i = 1; i < nodes_order.size(); ++i){
		Tree_node* last_plan = root_node;
		root_node = new Tree_node();
		root_node->joinType = NodeJoinType::JoinANode;
		root_node->left_node = last_plan;
		root_node->right_node = new Tree_node(nodes_order[i]);
	}
}

void PlanTree::delete_tree_node(Tree_node *root_node) {
	if(root_node == nullptr){
		return;
	}

	delete_tree_node(root_node->left_node);
	delete_tree_node(root_node->right_node);
	delete root_node;
}

PlanTree::~PlanTree() {
	delete_tree_node(root_node);
}


void PlanTree::print_tree_node(Tree_node* node, BasicQuery* basicquery){
	if(node == nullptr){
		return;
	}

	switch (node->joinType) {
		case NodeJoinType::LeafNode:
			cout << basicquery->getVarName(node->node_to_join);
			break;
			case NodeJoinType::JoinTwoTable:
				cout << "Binary join(";
				break;
				case NodeJoinType::JoinANode:
					cout << "WCO join(";
					break;
	}

	if(node->left_node!= nullptr){
		print_tree_node(node->left_node, basicquery);
		cout << ",";
		print_tree_node(node->right_node, basicquery);
	}

	if(node->joinType!=NodeJoinType::LeafNode){
		cout << ")";
	}
}

void PlanTree::print(BasicQuery* basicquery) {
	cout << "Plan: ";
	print_tree_node(root_node, basicquery);
	cout << ", cost: "<<this->plan_cost;
	cout << endl;
}

void PlanTree::print_tree_node(Tree_node *node, BGPQuery *bgpquery) {

	if(node == nullptr)
		return;
	if(node->left_node != nullptr)
		print_tree_node(node->left_node, bgpquery);
	if(node->right_node != nullptr)
		print_tree_node(node->right_node, bgpquery);

	auto stepoperation = node->node;
	cout << "join type: " << stepoperation->JoinTypeToString(stepoperation->join_type_) << endl;

	switch (stepoperation->join_type_) {
		case StepOperation::JoinType::GenerateCandidates:
		case StepOperation::JoinType::EdgeCheck:{
			for(unsigned i = 0; i < stepoperation->edge_filter_->edges_->size(); ++i){
				cout << "\tedge[" << i << "]:" << endl;
				cout << "\t\ts[" << (*stepoperation->edge_filter_->edges_)[i].s_ << "]" << ((*stepoperation->edge_filter_->edges_constant_info_)[i].s_constant_ ? "const" : "var") << "    ";
				cout << "p[" << (*stepoperation->edge_filter_->edges_)[i].p_ << "]" << ((*stepoperation->edge_filter_->edges_constant_info_)[i].p_constant_ ? "const" : "var") << "    ";
				cout << "o[" << (*stepoperation->edge_filter_->edges_)[i].o_ << "]" << ((*stepoperation->edge_filter_->edges_constant_info_)[i].o_constant_ ? "const" : "var") << "    ";
				cout << JoinMethodToString((*stepoperation->edge_filter_->edges_)[i].join_method_) << endl;
			}
			cout << "\tnode id: " << stepoperation->edge_filter_->node_to_join_ << endl;
			break;
		}
		case StepOperation::JoinType::JoinNode:{
			for(unsigned i = 0; i < stepoperation->join_node_->edges_->size(); ++ i){
				cout << "\tedge[" << i << "]:" << endl;
				cout << "\t\ts[" << (*stepoperation->join_node_->edges_)[i].s_ << "]" << ((*stepoperation->join_node_->edges_constant_info_)[i].s_constant_ ? "const" : "var") << "    ";
				cout << "p[" << (*stepoperation->join_node_->edges_)[i].p_ << "]" << ((*stepoperation->join_node_->edges_constant_info_)[i].p_constant_ ? "const" : "var") << "    ";
				cout << "o[" << (*stepoperation->join_node_->edges_)[i].o_ << "]" << ((*stepoperation->join_node_->edges_constant_info_)[i].o_constant_ ? "const" : "var") << "    ";
				cout << JoinMethodToString((*stepoperation->join_node_->edges_)[i].join_method_) << endl;
			}
			cout << "\tnode id: " << stepoperation->join_node_->node_to_join_ << endl;
			break;
		}
		case StepOperation::JoinType::JoinTable:{
			cout << "\tpublic nodes is: ";
			for(auto &x : (*stepoperation->join_table_->public_variables_))
				cout << x << "    ";
			cout << endl;
			break;
		}
		case StepOperation::JoinType::JoinTwoNodes:{
			cout << "\tedge:" << endl;
			cout << "\t\ts[" << stepoperation->join_two_node_->edges_.s_ << "]" << (stepoperation->join_two_node_->edges_constant_info_.s_constant_ ? "const" : "var") << "    ";
			cout << "p[" << stepoperation->join_two_node_->edges_.p_ << "]" << (stepoperation->join_two_node_->edges_constant_info_.p_constant_ ? "const" : "var") << "    ";
			cout << "o[" << stepoperation->join_two_node_->edges_.o_ << "]" << (stepoperation->join_two_node_->edges_constant_info_.o_constant_ ? "const" : "var") << endl;

			cout << "\tnodes id: " << stepoperation->join_two_node_->node_to_join_1_ << "    "
			<< stepoperation->join_two_node_->node_to_join_2_ << endl;
			break;
		}
		case StepOperation::JoinType::GetAllTriples:{
			for(unsigned i = 0; i < stepoperation->join_node_->edges_->size(); ++ i){
				cout << "\tedge[" << i << "]:" << endl;
				cout << "\t\ts[" << (*stepoperation->join_node_->edges_)[i].s_ << "]" << ((*stepoperation->join_node_->edges_constant_info_)[i].s_constant_ ? "const" : "var") << "    ";
				cout << "p[" << (*stepoperation->join_node_->edges_)[i].p_ << "]" << ((*stepoperation->join_node_->edges_constant_info_)[i].p_constant_ ? "const" : "var") << "    ";
				cout << "o[" << (*stepoperation->join_node_->edges_)[i].o_ << "]" << ((*stepoperation->join_node_->edges_constant_info_)[i].o_constant_ ? "const" : "var") << "    ";
				cout << JoinMethodToString((*stepoperation->join_node_->edges_)[i].join_method_) << endl;
			}
			cout << "\tget all triples from database via p2so" << endl;
			break;
		}
		default:{
			cout << "error! unknown JoinType!" << endl;
			exit(-1);
		}

	}

}

void PlanTree::print(BGPQuery* bgpquery) {
	cout << "Plan: " << endl;
	print_tree_node(root_node, bgpquery);
}