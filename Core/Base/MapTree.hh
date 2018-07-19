#pragma once

#include <assert.h>
#include <stddef.h>
#include <vector>
#include <array>
#include <utility>

namespace Ares
{

/// A tree of nodes that each contain a `T` key, can contain an `U` value, and
/// can have any amount of child nodes.
/// Can be indexed by (`T`, `T`, ...) sequences to retrieve the `U` value.
template <typename T, typename U>
class MapTree
{
public:
    // FIXME `static constexpr const size_t INVALID = -1;` triggers an "undefined reference" error,
    //       but integer type constant should be able to be declared inline?
    enum : size_t { INVALID = size_t(-1) };

private:
    /// A single node of the map.
    struct Node
    {
        /// The `T` key associated to this node.
        T key;

        /// An backwards offset in nodes (1..n) to the parent of this node.
        /// `INVALID` if this is the root node.
        size_t parentOffset;

        /// An offset in nodes (1..n) to the next sibling of this node.
        /// `INVALID` if the node has no sibling **after it** (maybe siblings at all).
        size_t nextSiblingOffset; ///< The offset in nodes to the next sibling nodes;

        /// The depth of this node inside of the node tree.
        /// 0 for the root node.
        size_t depth;

        /// The index of the value associated to this node in `values_`.
        /// `INVALID` if this node has no value associated to it.
        size_t valueIndex;

        Node(T key,
             size_t parentOffset, size_t nextSiblingOffset, size_t depth,
             size_t valueIndex)
            : key(key),
              parentOffset(parentOffset), nextSiblingOffset(nextSiblingOffset), depth(depth),
              valueIndex(valueIndex)
        {
        }
    };

    std::vector<Node> nodes_;
    std::vector<U> values_;


    /// Returns the index of the parent node of the node at `nodeIndex`.
    /// Undefined for the root node!
    inline size_t parentNodeIndex(size_t nodeIndex)
    {
        return nodeIndex - nodes_[nodeIndex].parentOffset;
    }

    /// Returns the index of the child of the node at `nodeIndex` that has the
    /// given key, or `INVALID` if no child with that key was found.
    size_t childNodeIndex(size_t nodeI, const T& key)
    {
        // If this node has a first child, its index will be `childNodeIndex`
        size_t childNodeI = nodeI + 1;

        bool keyChildFound = false; // Will become `true` if a child with key `key` is found

        // While there are nodes to iterate and those nodes are children of the
        // node at `nodeIndex`
        while(childNodeI < nodes_.size() - 1
              && parentNodeIndex(childNodeI) == nodeI)
        {
            Node& childNode = nodes_[childNodeI];

            keyChildFound |= childNode.key == key;

            if(keyChildFound || childNode.nextSiblingOffset == INVALID)
            {
                // Either we just found the child we were looking for or this
                // was the last child (offset to next sibling = INVALID); break
                // out of this loop
                break;
            }
            // Else go to the next sibling of `childNode`
            childNodeI += childNode.nextSiblingOffset;
        }

        return keyChildFound ? childNodeI : INVALID;
    }

    // Adds a node child of the given parent node. Returns the index of the
    // newly-added child node.
    // **WARNING**: Does not check if a child with the same key already exists,
    //              so make sure that it does not or a duplicate will be added!
    size_t addChildNode(size_t nodeI, const T& key)
    {
        size_t childI = nodeI + 1;
        bool hadAChild = childI < nodes_.size() && parentNodeIndex(childI) == nodeI;

        // Insert the node's new child right beside the node itself, moving the
        // any other children further away from the parent
        nodes_.emplace(nodes_.begin() + childI,
                       key,
                       1, // Parent offset
                       hadAChild ? 1 : INVALID, // Next sibling offset
                       nodes_[nodeI].depth + 1, // Node depth
                       INVALID); // Value index

        // Walk through all nodes after the newly-inserted child node; if their
        // `parentOffset` crosses the boundary between the nodes before the new
        // node and the nodes after it we need to increase their `parentOffset`
        // since the new node has been pushed in the middle now.
        for(size_t i = childI + 1; i < nodes_.size(); i ++)
        {
           if(nodes_[i].parentOffset >= (i - childI))
           {
               nodes_[i].parentOffset ++;
           }
        }

        // Walk all ancestor nodes upwards up to the root node and increase their
        // `nextSiblingOffset` (since now the new child is inbetween each of them
        // and their next sibling)
        size_t parentI = nodeI;
        while(parentI != 0)
        {
            if(nodes_[parentI].nextSiblingOffset != INVALID)
            {
                nodes_[parentI].nextSiblingOffset ++;
            }
            parentI = parentNodeIndex(parentI);
        }

        return childI;
    }

    /// Gets a reference to the value currently assigned to the given node, or
    /// default-constructs one linking it to the node and returns a reference
    /// to it.
    U& getOrAddValue(size_t nodeI)
    {
        size_t& nodeValueI = nodes_[nodeI].valueIndex;
        if(nodeValueI == INVALID)
        {
            // If that node has no value, default-construct a new one to associate
            // to it
            values_.emplace_back();
            nodeValueI = values_.size() - 1;
        }
        return values_[nodeValueI];
    }

public:
    /// Creates a new maptree with no nodes in it.
    MapTree()
    {
        clear();
    }

    /// Returns the number of nodes currently in the map tree.
    inline size_t nNodes() const
    {
        return nodes_.size();
    }

    /// Clears the map.
    /// This will clear all nodes in the map, their keys and their values, and insert
    /// a new empty implicit root node.
    void clear()
    {
        nodes_.clear();
        values_.clear();

        // Add the root node at index (and depth) 0.
        // Its `T` key is default-constructed as the root's key is unused
        // and its parent is set as `INVALID` since root is like Batman.
        nodes_.emplace_back(T(), // Key
                            INVALID, // Parent offset
                            INVALID, // Next sibling offset
                            0, // Depth
                            INVALID); // Value index
    }


    friend class iterator;

    /// An iterator over all nodes of a map tree.
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<T, U>;
        using reference = std::pair<const T&, U&>;
        using pointer = std::pair<const T*, U*>;
        friend class MapTree;

    private:
        MapTree* map_;
        size_t nodeI_;

        iterator(MapTree* map, size_t nodeIndex)
            : map_(map), nodeI_(nodeIndex)
        {
        }

    public:
        iterator(const iterator& toCopy)
        {
            (void)operator=(toCopy);
        }

        iterator& operator=(const iterator& toCopy)
        {
            // Copy data
            map_ = toCopy.map_;
            nodeI_ = toCopy.nodeI_;

            return *this;
        }

        iterator(iterator&& toMove)
        {
            (void)operator=(std::move(toMove));
        }

        iterator& operator=(iterator&& toMove)
        {
            // Move data over
            map_ = toMove.map_;
            nodeI_ = toMove.nodeI_;

            // Invalidate the moved instance
            toMove.nodeI_ = INVALID;

            return *this;
        }

        /// Returns `true` if the iterator is not `INVALID`.
        inline operator bool() const
        {
            return nodeI_ != INVALID;
        }


        inline bool operator==(const iterator& other) const
        {
            return nodeI_ == other.nodeI_ && map_ == other.map_;
        }

        inline bool operator!=(const iterator& other) const
        {
            return nodeI_ != other.nodeI_ || map_ != other.map_;
        }


        /// Returns the key associated to this node.
        /// **ASSERTS**: iterator is not invalid or out of bounds
        inline const T& key() const
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            return map_->nodes_[nodeI_].key;
        }

        /// Returns a R/W reference to the value associated to this node.
        /// If no value was associated, default-constructs one for the node and
        /// returns a reference to it.
        /// **ASSERTS**: iterator is not invalid or out of bounds
        inline U& value()
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            return map_->getOrAddValue(nodeI_);
        }

        /// Shorthand for `value() = newValue`.
        inline iterator& operator=(const U& newValue)
        {
            value() = newValue;
            return *this;
        }
        inline iterator& operator=(U&& newValue)
        {
            value() = std::move(newValue);
            return *this;
        }


        /// Returns the depth of this iterator in the node tree.
        /// The implicit root node has a depth of 0, its children 1, etc...
        /// **ASSERTS**: iterator is not invalid or out of bounds
        inline size_t depth() const
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            return map_->nodes_[nodeI_].depth;
        }

        /// Returns the index of the iterated node in the map.
        /// `INVALID` if the node. The index may also be out of bounds for the
        /// parent map.
        // TODO Describe the index based on `operator++`
        inline size_t index() const
        {
            return nodeI_;
        }


        /// Returns an iterator pointing to the **next** sibling of this node.
        /// `INVALID` if this node has no siblings **after it**.
        /// **ASSERTS**: iterator is not invalid or out of bounds
        iterator nextSibling() const
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            size_t nextSiblingOffset = map_->nodes_[nodeI_].nextSiblingOffset;
            if(nextSiblingOffset != INVALID)
            {
                return iterator(map_, nodeI_ + nextSiblingOffset);
            }
            else
            {
                // No more siblings
                return iterator(map_, INVALID);
            }
        }

        /// Returns an iterator pointing to the first child of this node.
        /// `INVALID` if this node is the root node.
        /// **ASSERTS**: iterator is not invalid or out of bounds
        iterator firstChild() const
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            size_t firstChildI = nodeI_ + 1;
            if(map_->nodes_.size() > firstChildI
               && map_->parentNodeIndex(firstChildI) == nodeI_)
            {
                // Actually found the first child
                return iterator(map_, firstChildI);
            }
            else
            {
                // No child found
                return iterator(map_, INVALID);
            }
        }

        /// Returns an iterator pointing to the parent of this node.
        /// `INVALID` if this node is the root node.
        /// **ASSERTS**: iterator is not invalid or out of bounds
        inline iterator parent() const
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            size_t parentI = map_->parentNodeIndex(nodeI_); // (`INVALID` for root)
            return iterator(map_, parentI);
        }

        /// Gets an iterator to the node in the tree at [key0, key1, key2... keyN]
        /// starting from this node.
        /// Returns `INVALID` if any of the keys is not found while walking down
        /// the node tree.
        /// **ASSERTS**: iterator is not invalid or out of bounds
        template <typename... Ts>
        inline iterator get(const T& key0, const Ts&... keys)
        {
            iterator child = get(key0);
            if(child.nodeI_ != INVALID)
            {
                return child.get(keys...);
            }
            else
            {
                return iterator(map_, INVALID);
            }
        }
        inline iterator get(const T& key0)
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            size_t childI = map_->childNodeIndex(nodeI_, key0);
            return iterator(map_, childI); // `INVALID` if the child was not found
        }

        /// Same as `get()`, but if any key is missing in the node tree a node
        /// for (with a default-constructed value and no children) will be created.
        /// **ASSERTS**: iterator is not invalid or out of bounds
        template <typename... Ts>
        inline iterator at(const T& key0, const Ts&... keys)
        {
            iterator child = at(key0);
            return child.at(keys...);
        }
        inline iterator at(const T& key0)
        {
            assert(nodeI_ != INVALID && nodeI_ < map_->nodes_.size());

            size_t childI = map_->childNodeIndex(nodeI_, key0);
            if(childI == INVALID)
            {
                childI = map_->addChildNode(nodeI_, key0);
            }
            return iterator(map_, childI);
        }

        // TODO Describe what incrementing an iterator does
        inline iterator& operator++() // preincrement
        {
            if(nodeI_ == INVALID)
            {
                return *this;
            }

            nodeI_ ++;

            return *this;
        }

        inline iterator operator++(int) // postincrement
        {
            iterator old = *this;
            (void)operator++();
            return old;
        }
    };


    /// Returns an iterator pointing to the implicit root node of the map.
    /// The treemap can be iterated between `begin()` and `end()` like a standard
    /// STL container.
    inline iterator begin()
    {
        return iterator(this, 0);
    }

    /// Returns an iterator pointing to one-past-the-end in the map's nodes list.
    /// The treemap can be iterated between `begin()` and `end()` like a standard
    /// STL container.
    inline iterator end()
    {
        return iterator(this, nodes_.size()); // One-past-the-end in the node list
    }

    /// Convenience function, equivalent to `begin().get(keys...)`.
    template <typename... Ts>
    inline iterator get(const Ts&... keys)
    {
        return begin().get(keys...);
    }

    /// Convenience function, equivalent to `begin().at(keys...)`.
    template <typename... Ts>
    inline iterator at(const Ts&... keys)
    {
        return begin().at(keys...);
    }
};

}
