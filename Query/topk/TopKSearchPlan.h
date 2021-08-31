//
// Created by Yuqi Zhou on 2021/8/30.
//
#include "../../Util/Util.h"
#include "../../KVstore/KVstore.h"
#include "./OrderedList.h"
#include "../../Query/SPARQLquery.h"
#include "../../Query/BasicQuery.h"
#include "../../Database/Statistics.h"
#include "../../Query/QueryTree.h"
#include "../../Query/IDList.h"
#include "../../Database/TableOperator.h"

#ifndef GSTOREGDB_QUERY_TOPK_TOPKSEARCHPLAN_H_
#define GSTOREGDB_QUERY_TOPK_TOPKSEARCHPLAN_H_


namespace TopKUtil{
enum class EdgeDirection{IN,OUT,NoEdge};

// something like FeedTwoNodes, but this one is only used in
// Top-K searching
struct TreeEdge{
  std::vector<bool> predicate_constant_;
  std::vector<TYPE_ENTITY_LITERAL_ID> predicate_ids_;
  std::vector<TopKUtil::EdgeDirection> directions_;
  void ChangeOrder();
};

}


// #define SHOW_SCORE
// only Vars
struct TopKTreeNode{
  int var_id;
  std::vector<std::shared_ptr<TopKUtil::TreeEdge>> tree_edges_;
  std::vector<TopKTreeNode*> descendents_;
  size_t descendents_fr_num_;
  size_t descendents_ow_num_;
};

/**
 * The General Top-k Search Plan Structure:
 * 1. filtering all entity vars
 * 2. build top-k iterators
 * 3. get top-k results
 * 3.1 check non-tree edges
 * 3.2 fill selected predicate vars
 */
class TopKSearchPlan {
 private:
  std::size_t total_vars_num_;
  static std::size_t CountDepth(map<TYPE_ENTITY_LITERAL_ID,vector<TYPE_ENTITY_LITERAL_ID>> &neighbours, TYPE_ENTITY_LITERAL_ID root_id, std::size_t total_vars_num);
  void AdjustOrder();
 public:
  explicit TopKSearchPlan(shared_ptr<BGPQuery> bgp_query, KVstore *kv_store, Statistics *statistics, QueryTree::Order expression,
                          shared_ptr<map<TYPE_ENTITY_LITERAL_ID,shared_ptr<IDList>>> id_caches);
  // The first tree to search
  TopKTreeNode* tree_root_;
  // Recursive delete
  ~TopKSearchPlan();
  // The Edges that left behind
  // It can be used in two ways:
  // 1 . when filtering, use non tree edges to early filter
  // 2 . when enumerating , use non tree edges to make sure correctness
  std::vector<StepOperation> non_tree_edges;
  // The predicate been selected,
  // We process these vars when all the entity vars have been filled
  std::vector<StepOperation> selected_predicate_edges;

  std::string DebugInfo();
};

#endif //GSTOREGDB_QUERY_TOPK_TOPKSEARCHPLAN_H_