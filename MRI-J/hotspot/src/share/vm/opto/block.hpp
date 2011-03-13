/*
 * Copyright 1997-2007 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *  
 */
// This file is a derivative work resulting from (and including) modifications
// made by Azul Systems, Inc.  The date of such changes is 2010.
// Copyright 2010 Azul Systems, Inc.  All Rights Reserved.
//
// Please contact Azul Systems, Inc., 1600 Plymouth Street, Mountain View, 
// CA 94043 USA, or visit www.azulsystems.com if you need additional information 
// or have any questions.
#ifndef BLOCK_HPP
#define BLOCK_HPP


#include "c2_globals.hpp"
#include "node.hpp"

// Optimization - Graph Style

class Block;
class CFGLoop;
class MachCallNode;
class Matcher;
class RootNode;
class VectorSet;
struct Tarjan;

//------------------------------Block_Array------------------------------------
// Map dense integer indices to Blocks.  Uses classic doubling-array trick.
// Abstractly provides an infinite array of Block*'s, initialized to NULL.
// Note that the constructor just zeros things, and since I use Arena 
// allocation I do not need a destructor to reclaim storage.
class Block_Array : public ResourceObj {
  uint _size;                   // allocated size, as opposed to formal limit
  debug_only(uint _limit;)      // limit to formal domain
protected:
  void grow( uint i );          // Grow array node to fit

public:
  Block **_blocks;
ResourceArea*_arena;//Arena to allocate in

Block_Array(ResourceArea*a):_arena(a),_size(OptoBlockListSize){
    debug_only(_limit=0);
    _blocks = NEW_ARENA_ARRAY( a, Block *, OptoBlockListSize );
    for( int i = 0; i < OptoBlockListSize; i++ ) {
      _blocks[i] = NULL;
    }
  }
  Block *lookup( uint i ) const // Lookup, or NULL for not mapped
  { return (i<Max()) ? _blocks[i] : (Block*)NULL; }
  Block *operator[] ( uint i ) const // Lookup, or assert for not mapped
  { assert( i < Max(), "oob" ); return _blocks[i]; }
  // Extend the mapping: index i maps to Block *n.
  void map( uint i, Block *n ) { if( i>=Max() ) grow(i); _blocks[i] = n; }
  uint Max() const { debug_only(return _limit); return _size; }

};


class Block_List : public Block_Array {
public:
  uint _cnt;
  Block_List() : Block_Array(Thread::current()->resource_area()), _cnt(0) {}
  void push( Block *b ) { map(_cnt++,b); }
  Block *pop() { return _blocks[--_cnt]; }
  Block *rpop() { Block *b = _blocks[0]; _blocks[0]=_blocks[--_cnt]; return b;}
  void remove( uint i );
  void insert( uint i, Block *n );
  uint size() const { return _cnt; }
  void reset() { _cnt = 0; }
};


class CFGElement : public ResourceObj {
 public:
  float _freq; // Execution frequency (estimate)

  CFGElement() : _freq(0.0f) {}
  virtual bool is_block() { return false; }
  virtual bool is_loop()  { return false; }
  Block*   as_Block() { assert(is_block(), "must be block"); return (Block*)this; }
  CFGLoop* as_CFGLoop()  { assert(is_loop(),  "must be loop");  return (CFGLoop*)this;  }
};

//------------------------------Block------------------------------------------
// This class defines a Basic Block.
// Basic blocks are used during the output routines, and are not used during
// any optimization pass.  They are created late in the game.
class Block : public CFGElement {
 public:
  // Nodes in this block, in order
  Node_List _nodes;

  // Basic blocks have a Node which defines Control for all Nodes pinned in
  // this block.  This Node is a RegionNode.  Exception-causing Nodes
  // (division, subroutines) and Phi functions are always pinned.  Later,
  // every Node will get pinned to some block.
  Node *head() const { return _nodes[0]; }

  // CAUTION: num_preds() is ONE based, so that predecessor numbers match
  // input edges to Regions and Phis.
  uint num_preds() const { return head()->req(); }
  Node *pred(uint i) const { return head()->in(i); }

  // Array of successor blocks, same size as projs array
  Block_Array _succs;

  // Basic blocks have some number of Nodes which split control to all
  // following blocks.  These Nodes are always Projections.  The field in
  // the Projection and the block-ending Node determine which Block follows.
  uint _num_succs;

  // Basic blocks also carry all sorts of good old fashioned DFS information
  // used to find loops, loop nesting depth, dominators, etc.
  uint _pre_order;              // Pre-order DFS number

  // Dominator tree
  uint _dom_depth;              // Depth in dominator tree for fast LCA
  Block *_idom;                 // Immediate dominator block

  CFGLoop *_loop;               // Loop to which this block belongs
  uint _rpo;                    // Number in reverse post order walk 

  virtual bool is_block() { return true; }
  float succ_prob(uint i); // return probability of i'th successor

  Block* dom_lca(Block* that);  // Compute LCA in dominator tree.
#ifdef ASSERT
  bool dominates(Block* that) {
    int dom_diff = this->_dom_depth - that->_dom_depth;
    if (dom_diff > 0)  return false;
    for (; dom_diff < 0; dom_diff++)  that = that->_idom;
    return this == that;
  }
#endif

  // Report the alignment required by this block.  Must be a power of 2.
  // The previous block will insert nops to get this alignment.
  uint code_alignment();

  // BLOCK_FREQUENCY is a sentinel to mark uses of constant block frequencies.
  // It is currently also used to scale such frequencies relative to 
  // FreqCountInvocations relative to the old value of 1500.
#define BLOCK_FREQUENCY(f) ((f * (float) 1500) / FreqCountInvocations)

  // Register Pressure (estimate) for Splitting heuristic
  uint _reg_pressure;
  uint _ihrp_index;
  uint _freg_pressure;
  uint _fhrp_index;

  // Mark and visited bits for an LCA calculation in insert_anti_dependences.
  // Since they hold unique node indexes, they do not need reinitialization.
  node_idx_t _raise_LCA_mark;
  void    set_raise_LCA_mark(node_idx_t x)    { _raise_LCA_mark = x; }
  node_idx_t  raise_LCA_mark() const          { return _raise_LCA_mark; }
  node_idx_t _raise_LCA_visited;
  void    set_raise_LCA_visited(node_idx_t x) { _raise_LCA_visited = x; }
  node_idx_t  raise_LCA_visited() const       { return _raise_LCA_visited; }

  // Connector blocks. Connector blocks are basic blocks devoid of 
  // instructions, but may have relevant non-instruction Nodes, such as
  // Phis or MergeMems. Such blocks are discovered and marked during the
  // RemoveEmpty phase, and elided during Output.
  bool _connector;
  void set_connector() { _connector = true; }
  bool is_connector() const { return _connector; };

  // Create a new Block with given head Node.
  // Creates the (empty) predecessor arrays.
Block(ResourceArea*a,Node*headnode)
    : CFGElement(),
      _nodes(a),
      _succs(a), 
      _num_succs(0), 
      _pre_order(0), 
      _idom(0), 
      _loop(NULL),
      _reg_pressure(0), 
      _ihrp_index(1), 
      _freg_pressure(0), 
      _fhrp_index(1), 
      _connector(false) { 
    _nodes.push(headnode); 
  }

  // Index of 'end' Node
  uint end_idx() const {
    // %%%%% add a proj after every goto 
    // so (last->is_block_proj() != last) always, then simplify this code
    // This will not give correct end_idx for block 0 when it only contains root.
    int last_idx = _nodes.size() - 1;
    Node *last  = _nodes[last_idx];
    assert(last->is_block_proj() == last || last->is_block_proj() == _nodes[last_idx - _num_succs], "");
    return (last->is_block_proj() == last) ? last_idx : (last_idx - _num_succs);
  }

  // Basic blocks have a Node which ends them.  This Node determines which 
  // basic block follows this one in the program flow.  This Node is either an
  // IfNode, a GotoNode, a JmpNode, or a ReturnNode.
  Node *end() const { return _nodes[end_idx()]; }

  // Add an instruction to an existing block.  It must go after the head
  // instruction and before the end instruction.
  void add_inst( Node *n ) { _nodes.insert(end_idx(),n); }
  // Find node in block
  uint find_node( const Node *n ) const;
  // Find and remove n from block list
  void find_remove( const Node *n ); 

  // Schedule a call next in the block
  uint sched_call(Matcher &matcher, Block_Array &bbs, uint node_cnt, Node_List &worklist, int *ready_cnt, MachCallNode *mcall, VectorSet &next_call);

  // Perform basic-block local scheduling
  Node *select(PhaseCFG *cfg, Node_List &worklist, int *ready_cnt, VectorSet &next_call, uint sched_slot);
  void set_next_call( Node *n, VectorSet &next_call, Block_Array &bbs );
  void needed_for_next_call(Node *this_call, VectorSet &next_call, Block_Array &bbs);
  bool schedule_local(PhaseCFG *cfg, Matcher &m, int *ready_cnt, VectorSet &next_call);
  // Cleanup if any code lands between a Call and his Catch
  void call_catch_cleanup(Block_Array &bbs);
  // Detect implicit-null-check opportunities.  Basically, find NULL checks 
  // with suitable memory ops nearby.  Use the memory op to do the NULL check.
  // I can generate a memory op if there is not one nearby.
void implicit_null_check(PhaseCFG*cfg,Node*proj,Node*val);

  // Return the empty status of a block
  enum { not_empty, empty_with_goto, completely_empty };
  int is_Empty() const;

  // Forward through connectors
  Block* non_connector() {
    Block* s = this;
    while (s->is_connector()) {
      s = s->_succs[0];
    }
    return s;
  }

  // Successor block, after forwarding through connectors 
  Block* non_connector_successor(int i) const {
    return _succs[i]->non_connector();
  }
 
  // Examine block's code shape to predict if it is not commonly executed. 
  bool has_uncommon_code() const;

  // Use frequency calculations and code shape to predict if the block
  // is uncommon.
  bool is_uncommon( Block_Array &bbs ) const;

#ifndef PRODUCT
  // Debugging print of basic block
  void dump_bidx(const Block* orig) const;
  void dump_pred(const Block_Array *bbs, Block* orig) const;
  void dump_head( const Block_Array *bbs ) const;
  void dump( ) const;
  void dump( const Block_Array *bbs ) const;
#endif
};


//------------------------------PhaseCFG---------------------------------------
// Build an array of Basic Block pointers, one per Node.
class PhaseCFG : public Phase {
 private:
  // Build a proper looking cfg.  Return count of basic blocks
  uint build_cfg();

  // Perform DFS search.  
  // Setup 'vertex' as DFS to vertex mapping.  
  // Setup 'semi' as vertex to DFS mapping.  
  // Set 'parent' to DFS parent.             
  uint DFS( Tarjan *tarjan );

  // Set the basic block for pinned Nodes
  void schedule_pinned_nodes( VectorSet &visited );

  // I'll need a few machine-specific GotoNodes.  Clone from this one.
  MachNode *_goto;
  void insert_goto_at(uint block_no, uint succ_no);

  Block* insert_anti_dependences(Block* LCA, Node* load, bool verify = false);
  void verify_anti_dependences(Block* LCA, Node* load) {
    assert(LCA == _bbs[load->_idx], "should already be scheduled");
    insert_anti_dependences(LCA, load, true);
  }

 public:
PhaseCFG(ResourceArea*a,RootNode*r,Matcher&m);

  uint _num_blocks;             // Count of basic blocks
  Block_List _blocks;           // List of basic blocks  
  RootNode *_root;              // Root of whole program
  Block_Array _bbs;             // Map Nodes to owning Basic Block
  Block *_broot;                // Basic block of root
  uint _rpo_ctr;
  CFGLoop* _root_loop;

  MachNode *gto() const { return _goto; }

  // Per node latency estimation, valid only during GCM
  GrowableArray<uint> _node_latency;

#ifndef PRODUCT
  bool _trace_opto_pipelining;  // tracing flag
#endif

  // Build dominators
  void Dominators();

  // Estimate block frequencies based on IfNode probabilities
  void Estimate_Block_Frequency();

  // Global Code Motion.  See Click's PLDI95 paper.  Place Nodes in specific
  // basic blocks; i.e. _bbs now maps _idx for all Nodes to some Block.
  void GlobalCodeMotion( Matcher &m, uint unique, Node_List &proj_list );

  // Compute the (backwards) latency of a node from the uses
  void latency_from_uses(Node *n);

  // Compute the (backwards) latency of a node from a single use
  int latency_from_use(Node *n, const Node *def, Node *use);

  // Compute the (backwards) latency of a node from the uses of this instruction
  void partial_latency_of_defs(Node *n);
 
  // Schedule Nodes early in their basic blocks.
  bool schedule_early(VectorSet &visited, Node_List &roots);

  // For each node, find the latest block it can be scheduled into
  // and then select the cheapest block between the latest and earliest
  // block to place the node.
  void schedule_late(VectorSet &visited, Node_List &stack);

  // Pick a block between early and late that is a cheaper alternative
  // to late. Helper for schedule_late.
  Block* hoist_to_cheaper_block(Block* LCA, Block* early, Node* self);

  // Compute the instruction global latency with a backwards walk
  void ComputeLatenciesBackwards(VectorSet &visited, Node_List &stack);

  // Remove empty basic blocks
  void RemoveEmpty();
  bool MoveToNext(Block* bx, uint b_index);
  void MoveToEnd(Block* bx, uint b_index);

  // Check for NeverBranch at block end.  This needs to become a GOTO to the
  // true target.  NeverBranch are treated as a conditional branch that always
  // goes the same direction for most of the optimizer and are used to give a
  // fake exit path to infinite loops.  At this late stage they need to turn
  // into Goto's so that when you enter the infinite loop you indeed hang.
  void convert_NeverBranch_to_Goto(Block *b);

  CFGLoop* create_loop_tree();

  // Insert a node into a block, and update the _bbs
  void insert( Block *b, uint idx, Node *n ) { 
    b->_nodes.insert( idx, n ); 
    _bbs.map( n->_idx, b ); 
  } 

  // Helper function to insert a node into a block
  void schedule_node_into_block( Node *n, Block *b );

#ifndef PRODUCT
  bool trace_opto_pipelining() const { return _trace_opto_pipelining; }

  // Debugging print of CFG
  void dump( ) const;           // CFG only
  void _dump_cfg( const Node *end, VectorSet &visited  ) const;
  void verify() const;
  void dump_headers();
#else
  bool trace_opto_pipelining() const { return false; }
#endif
};


//------------------------------UnionFindInfo----------------------------------
// Map Block indices to a block-index for a cfg-cover.
// Array lookup in the optimized case.
class UnionFind : public ResourceObj {
  uint _cnt, _max;
  uint* _indices;
  ReallocMark _nesting;  // assertion check for reallocations
public:
  UnionFind( uint max );
  void reset( uint max );  // Reset to identity map for [0..max]

  uint lookup( uint nidx ) const {
    return _indices[nidx];
  }
  uint operator[] (uint nidx) const { return lookup(nidx); }

  void map( uint from_idx, uint to_idx ) {
    assert( from_idx < _cnt, "oob" );
    _indices[from_idx] = to_idx;
  }
  void extend( uint from_idx, uint to_idx );

  uint Size() const { return _cnt; }

  uint Find( uint idx ) {
    assert( idx < 65536, "Must fit into uint");
    uint uf_idx = lookup(idx);
    return (uf_idx == idx) ? uf_idx : Find_compress(idx);
  }
  uint Find_compress( uint idx );
  uint Find_const( uint idx ) const;
  void Union( uint idx1, uint idx2 );

};

//----------------------------BlockProbPair---------------------------
// Ordered pair of Node*.
class BlockProbPair VALUE_OBJ_CLASS_SPEC {
protected:
  Block* _target;      // block target
  float  _prob;        // probability of edge to block
public:
  BlockProbPair() : _target(NULL), _prob(0.0) {}
  BlockProbPair(Block* b, float p) : _target(b), _prob(p) {}

  Block* get_target() const { return _target; }
  float get_prob() const { return _prob; }
};

//------------------------------CFGLoop-------------------------------------------
class CFGLoop : public CFGElement {
  int _id;
  int _depth;
  CFGLoop *_parent;      // root of loop tree is the method level "pseudo" loop, it's parent is null
  CFGLoop *_sibling;     // null terminated list
  CFGLoop *_child;       // first child, use child's sibling to visit all immediately nested loops
  GrowableArray<CFGElement*> _members; // list of members of loop
  GrowableArray<BlockProbPair> _exits; // list of successor blocks and their probabilities
  float _exit_prob;       // probability any loop exit is taken on a single loop iteration
  void update_succ_freq(Block* b, float freq);

 public:
  CFGLoop(int id) :
    CFGElement(),
    _id(id),
    _depth(0),
    _parent(NULL),
    _sibling(NULL),
    _child(NULL),
    _exit_prob(1.0f) {}
  CFGLoop* parent() { return _parent; }
  void push_pred(Block* blk, int i, Block_List& worklist, Block_Array& node_to_blk);
  void add_member(CFGElement *s) { _members.push(s); }
  void add_nested_loop(CFGLoop* cl);
  Block* head() {
    assert(_members.at(0)->is_block(), "head must be a block");
    Block* hd = _members.at(0)->as_Block();
    assert(hd->_loop == this, "just checking");
    assert(hd->head()->is_Loop(), "must begin with loop head node");
    return hd;
  }
  Block* backedge_block(); // Return the block on the backedge of the loop (else NULL)
  void compute_loop_depth(int depth);
  void compute_freq(); // compute frequency with loop assuming head freq 1.0f
  void scale_freq();   // scale frequency by loop trip count (including outer loops)
  bool in_loop_nest(Block* b);
  float trip_count() const { return 1.0f / _exit_prob; }
  virtual bool is_loop()  { return true; }
  int id() { return _id; }

#ifndef PRODUCT
  void dump( ) const;
  void dump_tree() const;
#endif
};

#endif // BLOCK_HPP