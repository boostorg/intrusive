//
// Created by daniel on 27/04/2021.
//
#include <boost/intrusive/rbtree.hpp>
#include <boost/next_prior.hpp>

#include <cassert>
#include <vector>

namespace {

struct Node : boost::intrusive::set_base_hook<> {};

typedef boost::intrusive::rbtree<Node> Tree;
typedef typename Tree::node_traits NodeTraits;

NodeTraits::node_ptr get_parent(const Node& node) {
   return NodeTraits::get_parent(node.this_ptr());
}

NodeTraits::node_ptr get_left(const Node& node) {
   return NodeTraits::get_left(node.this_ptr());
}

NodeTraits::node_ptr get_right(const Node& node) {
   return NodeTraits::get_right(node.this_ptr());
}

Tree perfect_binary_tree_of_height_2(std::vector<Node>& node_buffer) {
   // Perfect binary tree of height 2
   //            3
   //          ╱   ╲
   //         1     5
   //        ╱ ╲   ╱ ╲
   //       0   2 4   6
   assert(node_buffer.size() == 7);
   Tree tree;

   const Tree::iterator it3 = tree.insert_before(tree.end(), node_buffer[3]);
   const Tree::iterator it1 = tree.insert_before(it3, node_buffer[1]);
   const Tree::iterator it5 = tree.insert_before(tree.end(), node_buffer[5]);
   tree.insert_before(it1, node_buffer[0]);
   tree.insert_before(it3, node_buffer[2]);
   tree.insert_before(it5, node_buffer[4]);
   tree.insert_before(tree.end(), node_buffer[6]);

   // Make sure we got the tree we expected
   assert(get_parent(node_buffer[0]) == node_buffer[1].this_ptr());
   assert(get_left(node_buffer[0]) == NULL);
   assert(get_right(node_buffer[0]) == NULL);
   assert(get_parent(node_buffer[1]) == node_buffer[3].this_ptr());
   assert(get_left(node_buffer[1]) == node_buffer[0].this_ptr());
   assert(get_right(node_buffer[1]) == node_buffer[2].this_ptr());
   assert(get_parent(node_buffer[2]) == node_buffer[1].this_ptr());
   assert(get_left(node_buffer[2]) == NULL);
   assert(get_right(node_buffer[2]) == NULL);
   assert(get_left(node_buffer[3]) == node_buffer[1].this_ptr());
   assert(get_right(node_buffer[3]) == node_buffer[5].this_ptr());
   assert(get_parent(node_buffer[4]) == node_buffer[5].this_ptr());
   assert(get_left(node_buffer[4]) == NULL);
   assert(get_right(node_buffer[4]) == NULL);
   assert(get_parent(node_buffer[5]) == node_buffer[3].this_ptr());
   assert(get_left(node_buffer[5]) == node_buffer[4].this_ptr());
   assert(get_right(node_buffer[5]) == node_buffer[6].this_ptr());
   assert(get_parent(node_buffer[6]) == node_buffer[5].this_ptr());
   assert(get_left(node_buffer[6]) == NULL);
   assert(get_right(node_buffer[6]) == NULL);

   return tree;
}

// Test that swaps node_buffer 0 and 4 and verifies the results
struct SwapUnrelatedLeafNodesTest {
   std::vector<Node>& node_buffer;
   Tree& tree;

   SwapUnrelatedLeafNodesTest(std::vector<Node>& node_buffer, Tree& tree)
      : node_buffer(node_buffer), tree(tree) {}

   void check() {
      assert(&*boost::next(tree.begin(), 0) == &node_buffer[4]);
      assert(&*boost::next(tree.begin(), 1) == &node_buffer[1]);
      assert(&*boost::next(tree.begin(), 2) == &node_buffer[2]);
      assert(&*boost::next(tree.begin(), 3) == &node_buffer[3]);
      assert(&*boost::next(tree.begin(), 4) == &node_buffer[0]);
      assert(&*boost::next(tree.begin(), 5) == &node_buffer[5]);
      assert(&*boost::next(tree.begin(), 6) == &node_buffer[6]);
      assert(get_parent(node_buffer[0]) == node_buffer[5].this_ptr());
      assert(get_left(node_buffer[0]) == NULL);
      assert(get_right(node_buffer[0]) == NULL);
      assert(get_left(node_buffer[1]) == node_buffer[4].this_ptr());
      assert(get_right(node_buffer[1]) == node_buffer[2].this_ptr());
      assert(get_parent(node_buffer[4]) == node_buffer[1].this_ptr());
      assert(get_left(node_buffer[4]) == NULL);
      assert(get_right(node_buffer[4]) == NULL);
      assert(get_left(node_buffer[5]) == node_buffer[0].this_ptr());
      assert(get_right(node_buffer[5]) == node_buffer[6].this_ptr());
   }

   void swap04() {
      node_buffer[0].swap_nodes(node_buffer[4]);
   }

   void swap40() {
      node_buffer[4].swap_nodes(node_buffer[0]);
   }
};

// Test that swaps node_buffer 0 and 2 and verifies the results
struct SwapSiblingLeafNodesTest {
   std::vector<Node>& node_buffer;
   Tree& tree;

   SwapSiblingLeafNodesTest(std::vector<Node>& node_buffer, Tree& tree)
      : node_buffer(node_buffer), tree(tree) {}

   void check() {
      assert(&*boost::next(tree.begin(), 0) == &node_buffer[2]);
      assert(&*boost::next(tree.begin(), 1) == &node_buffer[1]);
      assert(&*boost::next(tree.begin(), 2) == &node_buffer[0]);
      assert(&*boost::next(tree.begin(), 3) == &node_buffer[3]);
      assert(&*boost::next(tree.begin(), 4) == &node_buffer[4]);
      assert(&*boost::next(tree.begin(), 5) == &node_buffer[5]);
      assert(&*boost::next(tree.begin(), 6) == &node_buffer[6]);
      assert(get_parent(node_buffer[0]) == node_buffer[1].this_ptr());
      assert(get_left(node_buffer[0]) == NULL);
      assert(get_right(node_buffer[0]) == NULL);
      assert(get_left(node_buffer[1]) == node_buffer[2].this_ptr());
      assert(get_right(node_buffer[1]) == node_buffer[0].this_ptr());
      assert(get_parent(node_buffer[2]) == node_buffer[1].this_ptr());
      assert(get_left(node_buffer[2]) == NULL);
      assert(get_right(node_buffer[2]) == NULL);
   }

   void swap02() {
      node_buffer[0].swap_nodes(node_buffer[2]);
   }

   void swap20() {
      node_buffer[2].swap_nodes(node_buffer[0]);
   }
};

// Test that swaps node_buffer 1 and 5 and verifies the results
struct SwapSiblingNodesTest {
  std::vector<Node>& node_buffer;
  Tree& tree;

  SwapSiblingNodesTest(std::vector<Node>& node_buffer, Tree& tree)
      : node_buffer(node_buffer), tree(tree) {}

  void check() {
     assert(&*boost::next(tree.begin(), 0) == &node_buffer[0]);
     assert(&*boost::next(tree.begin(), 1) == &node_buffer[5]);
     assert(&*boost::next(tree.begin(), 2) == &node_buffer[2]);
     assert(&*boost::next(tree.begin(), 3) == &node_buffer[3]);
     assert(&*boost::next(tree.begin(), 4) == &node_buffer[4]);
     assert(&*boost::next(tree.begin(), 5) == &node_buffer[1]);
     assert(&*boost::next(tree.begin(), 6) == &node_buffer[6]);
     assert(get_parent(node_buffer[0]) == node_buffer[5].this_ptr());
     assert(get_parent(node_buffer[1]) == node_buffer[3].this_ptr());
     assert(get_left(node_buffer[1]) == node_buffer[4].this_ptr());
     assert(get_right(node_buffer[1]) == node_buffer[6].this_ptr());
     assert(get_parent(node_buffer[2]) == node_buffer[5].this_ptr());
     assert(get_left(node_buffer[3]) == node_buffer[5].this_ptr());
     assert(get_right(node_buffer[3]) == node_buffer[1].this_ptr());
     assert(get_parent(node_buffer[4]) == node_buffer[1].this_ptr());
     assert(get_parent(node_buffer[5]) == node_buffer[3].this_ptr());
     assert(get_left(node_buffer[5]) == node_buffer[0].this_ptr());
     assert(get_right(node_buffer[5]) == node_buffer[2].this_ptr());
     assert(get_parent(node_buffer[6]) == node_buffer[1].this_ptr());
  }

  void swap15() {
     node_buffer[1].swap_nodes(node_buffer[5]);
  }

  void swap51() {
     node_buffer[5].swap_nodes(node_buffer[1]);
  }
};

// Test that swaps node_buffer 0 and 1 and verifies the results
struct SwapWithLeftChildTest {
  std::vector<Node>& node_buffer;
  Tree& tree;

  SwapWithLeftChildTest(std::vector<Node>& node_buffer, Tree& tree)
      : node_buffer(node_buffer), tree(tree) {}

  void check() {
     assert(&*boost::next(tree.begin(), 0) == &node_buffer[1]);
     assert(&*boost::next(tree.begin(), 1) == &node_buffer[0]);
     assert(&*boost::next(tree.begin(), 2) == &node_buffer[2]);
     assert(&*boost::next(tree.begin(), 3) == &node_buffer[3]);
     assert(&*boost::next(tree.begin(), 4) == &node_buffer[4]);
     assert(&*boost::next(tree.begin(), 5) == &node_buffer[5]);
     assert(&*boost::next(tree.begin(), 6) == &node_buffer[6]);
     assert(get_parent(node_buffer[0]) == node_buffer[3].this_ptr());
     assert(get_left(node_buffer[0]) == node_buffer[1].this_ptr());
     assert(get_right(node_buffer[0]) == node_buffer[2].this_ptr());
     assert(get_parent(node_buffer[1]) == node_buffer[0].this_ptr());
     assert(get_left(node_buffer[1]) == NULL);
     assert(get_right(node_buffer[1]) == NULL);
     assert(get_parent(node_buffer[2]) == node_buffer[0].this_ptr());
     assert(get_left(node_buffer[2]) == NULL);
     assert(get_right(node_buffer[2]) == NULL);
  }

  void swap01() {
     node_buffer[0].swap_nodes(node_buffer[1]);
  }

  void swap10() {
     node_buffer[1].swap_nodes(node_buffer[0]);
  }
};

// Test that swaps node_buffer 1 and 2 and verifies the results
struct SwapWithRightChildTest {
  std::vector<Node>& node_buffer;
  Tree& tree;

  SwapWithRightChildTest(std::vector<Node>& node_buffer, Tree& tree)
      : node_buffer(node_buffer), tree(tree) {}

  void check() {
     assert(&*boost::next(tree.begin(), 0) == &node_buffer[0]);
     assert(&*boost::next(tree.begin(), 1) == &node_buffer[2]);
     assert(&*boost::next(tree.begin(), 2) == &node_buffer[1]);
     assert(&*boost::next(tree.begin(), 3) == &node_buffer[3]);
     assert(&*boost::next(tree.begin(), 4) == &node_buffer[4]);
     assert(&*boost::next(tree.begin(), 5) == &node_buffer[5]);
     assert(&*boost::next(tree.begin(), 6) == &node_buffer[6]);
     assert(get_parent(node_buffer[0]) == node_buffer[2].this_ptr());
     assert(get_left(node_buffer[0]) == NULL);
     assert(get_right(node_buffer[0]) == NULL);
     assert(get_parent(node_buffer[1]) == node_buffer[2].this_ptr());
     assert(get_left(node_buffer[1]) == NULL);
     assert(get_right(node_buffer[1]) == NULL);
     assert(get_parent(node_buffer[2]) == node_buffer[3].this_ptr());
     assert(get_left(node_buffer[2]) == node_buffer[0].this_ptr());
     assert(get_right(node_buffer[2]) == node_buffer[1].this_ptr());
  }

  void swap12() {
     node_buffer[1].swap_nodes(node_buffer[2]);
  }

  void swap21() {
     node_buffer[2].swap_nodes(node_buffer[1]);
  }
};

}

int main() {
   { // SwapUnrelatedLeafNodesTest 0 -> 4
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapUnrelatedLeafNodesTest test(node_buffer, tree);
      test.swap04();
      test.check();
   }

   { // SwapUnrelatedLeafNodesTest 0 <- 4
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapUnrelatedLeafNodesTest test(node_buffer, tree);
      test.swap40();
      test.check();
   }

   { // SwapSiblingLeafNodesTest 0 -> 2
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapSiblingLeafNodesTest test(node_buffer, tree);
      test.swap02();
      test.check();
   }

   { // SwapSiblingLeafNodesTest 0 <- 2
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapSiblingLeafNodesTest test(node_buffer, tree);
      test.swap20();
      test.check();
   }

   { // SwapSiblingNodesTest 1 -> 5
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapSiblingNodesTest test(node_buffer, tree);
      test.swap15();
      test.check();
   }

   { // SwapSiblingNodesTest 1 <- 5
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapSiblingNodesTest test(node_buffer, tree);
      test.swap51();
      test.check();
   }

   { // SwapWithLeftChildTest 0 -> 1
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapWithLeftChildTest test(node_buffer, tree);
      test.swap01();
      test.check();
   }

   { // SwapWithLeftChildTest 0 <- 1
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapWithLeftChildTest test(node_buffer, tree);
      test.swap10();
      test.check();
   }

   { // SwapWithRightChildTest 1 -> 2
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapWithRightChildTest test(node_buffer, tree);
      test.swap12();
      test.check();
   }

   { // SwapWithRightChildTest 1 <- 2
      std::vector<Node> node_buffer(7);
      Tree tree = perfect_binary_tree_of_height_2(node_buffer);
      SwapWithRightChildTest test(node_buffer, tree);
      test.swap21();
      test.check();
   }
}
