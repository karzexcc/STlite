#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include <cassert>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template <typename Tp>
void swap(const Tp& a, const Tp& b) {
	Tp c = a;
	a = b;
	b = c;
}

template <class T1, class T2>
pair<T1, T2> make_pair(const T1 &a, const T2 &b) {
	return pair<T1, T2>(a, b);
}
	
template<
	class Key,
	class T,
	class Compare = std::less<Key>
> class map {
public:
	using value_type = pair<const Key, T>;
private:
	friend class iterator;
	friend class const_iterator;

	enum color_t {
		BLACK = 0,
		RED
	};

	struct tree_node;

	void rot_right(tree_node *y) {
		tree_node* x = y->left, *p = y->parent;
		y->left = x->right;
		if (x->right) x->right->parent = y;
		y->parent = x;
		x->right = y;
		x->parent = p;
		if (y == p->parent) 
			p->parent = x;
		else if (y == p->left)   
			p->left = x;
		else if (y == p->right)  
			p->right = x;
	}

	void rot_left(tree_node *x) {
		tree_node* y = x->right, *p = x->parent;
		x->right = y->left;
		if (y->left) y->left->parent = x;
		x->parent = y;
		y->left = x;
		y->parent = p;
		if (x == p->parent) 
			p->parent = y;
		else if (x == p->left)   
			p->left = y;
		else if (x == p->right)  
			p->right = y;
	}

	void tree_insert_rebalance(tree_node *x) {
		while (x != header->parent && x->parent->color == RED) {
			tree_node *p = x->parent, *g = x->parent->parent;
			if (p == g->left) {
				// parent is grandparent's left.
				tree_node *u = g->right;
				if (u && u->color == RED) {
					// uncle exists and color is RED.
					u->color = p->color = BLACK;
					g->color = RED;
					x = g;
				} else {
					//uncle not exists or color is BLACK.
					if (x == p->right) {
						x = p;
						rot_left(x);
					}
					x->parent->color = BLACK;
					x->parent->parent->color = RED;
					rot_right(x->parent->parent);
				}
			}
			else {
				// parent is grandparent's right.
				tree_node *u = g->left;
				if (u && u->color == RED) {
					// uncle exists and color is RED.
					u->color = p->color = BLACK;
					g->color = RED;
					x = g;
				} else {
					//uncle not exists or color is BLACK.
					if (x == p->left) {
						x = p;
						rot_right(x);
					}
					x->parent->color = BLACK;
					x->parent->parent->color = RED;
					rot_left(x->parent->parent);
				}
			}
		}
		header->parent->color = BLACK;
	}

	bool is_red(tree_node *x) { return x && x->color == RED; }
	bool is_black(tree_node *x) { return !is_red(x); }

	void tree_delete_rebalance(tree_node *x, tree_node *parent) {
		color_t color = x ? x->color : BLACK;

		while (x != header->parent && color == BLACK) {
			if (x == parent->left) {
				tree_node *w = parent->right;
				if (is_red(w)) {
					w->color = BLACK;
					parent->color = RED;
					rot_left(parent);
					w = parent->right;
				}
				if (w && is_black(w->left) && is_black(w->right)) {
					w->color = RED;
					x = parent;
				}
				else {
					if (w && is_black(w->right)) {
						w->left->color = BLACK;
						w->color = RED;
						rot_right(w);
						w = parent->right;
					}
					if (w) w->color = parent->color;
					parent->color = BLACK;
					if (w && w->right) w->right->color = BLACK;
					rot_left(parent);
					x = header->parent;
				}
			}
			else {
				tree_node *w = parent->left;
				if (w->color == RED) {
					w->color = BLACK;
					parent->color = RED;
					rot_right(parent);
					w = parent->left;
				}
				if (w && is_black(w->right) && is_black(w->left)) {
					w->color = RED;
					x = parent;
				}
				else {
					if (w && is_black(w->left)) {
						w->right->color = BLACK;
						w->color = RED;
						rot_left(w);
						w = parent->left;
					}
					if (w) w->color = parent->color;
					parent->color = BLACK;
					if (w && w->left) w->left->color = BLACK;
					rot_right(parent);
					x = header->parent;
				}
			}
			if (x) {
				color = x->color;
				parent = x->parent;
			} else {
				color = BLACK;
				parent = header->parent;
			}
			
		}
		if (x) x->color = BLACK;
	}

	struct tree_node {
		value_type* v;
		tree_node *left, *right, *parent;
		color_t color;
		tree_node(): left(nullptr), right(nullptr), parent(nullptr), v(nullptr), color(BLACK) {}
		tree_node(const Key& key, const T& value, const color_t &color = BLACK, tree_node *p = nullptr, tree_node *l = nullptr, tree_node *r = nullptr):
			v(new value_type(key, value)), left(l), right(r), color(color), parent(p) {}
		~tree_node() {
			if (v) delete v;
		}
	};

	tree_node *header; 
	size_t tree_size;

	tree_node* retrieve_succ(tree_node *p) const {
		tree_node *q = p;
		q = q->right;
		while (q->left) q = q->left;
		return q;
	}

	tree_node* retrieve_pred(tree_node *p) const {
		tree_node *q = p;
		q = q->left;
		while (q->right) q = q->right;
		return q;
	}

	pair<tree_node*, bool> tree_get_or_create(const Key& key) {
		tree_node *p = header->parent, *parent = header;
		bool from_left;

        if (!p) {
            auto new_node = new tree_node(key, T(), BLACK, parent);
            header->parent = header->left = header->right = new_node;
			tree_size++;
			return make_pair(new_node, true);
        }

		while (p) {
			parent = p;
			if (Compare()(key, p->v->first)) {
				p = p->left;
				from_left = true;
			}
			else if (Compare()(p->v->first, key)) {
				p = p->right;
				from_left = false;
			} else {
				return make_pair(p, false);
			}
		}

		tree_size++;
        auto new_node = new tree_node(key, T(), RED, parent);
        if (from_left) {
            parent->left = new_node;
            if (parent == header->left) {
                header->left = new_node;
            }
        } else {
            parent->right = new_node;
            if (parent == header->right) {
                header->right = new_node;
            }
        }

		tree_insert_rebalance(new_node);
        return make_pair(new_node, true);
	}

	tree_node* tree_access(const Key& key) const {
		tree_node *p = header->parent;
		while (p) {
			if (Compare()(key, p->v->first)) 
				p = p->left;
			else if (Compare()(p->v->first, key))
				p = p->right;
			else 
				return p;
		}
		return nullptr;
	}

	void remove_tree_all(tree_node *root) {
		if (root == nullptr) return;
		remove_tree_all(root->left);
		remove_tree_all(root->right);
		delete root;
	}

	void Transplant(tree_node *p, tree_node *q) {
		if (header->parent == p)      
			header->parent = q;
		else if (p->parent->left == p)
			p->parent->left = q;
		else p->parent->right = q;
			
		if (q) 
			q->parent = p->parent;
	}

	void header_replace(tree_node *p) {
		if (p == header->left) {
			if (p->right) header->left = retrieve_succ(p);
			else header->left = p->parent;
		}
		if (p == header->right) {
			if (p->left) header->right = retrieve_pred(p);
			else header->right = p->parent;
		}
	}

	void tree_remove(tree_node* p) {
		tree_node *x = nullptr;
		color_t orig_color = p->color;
		tree_node *parent;
		bool from_left = true;
		--tree_size;
		
		if (p->left == nullptr) {
			x = p->right;
			from_left = false;
			header_replace(p);
			Transplant(p, x);
			parent = p->parent;
		}
		else if (p->right == nullptr) {
			x = p->left;
			header_replace(p);
			Transplant(p, x);
			parent = p->parent;
		}
		else {
			tree_node *y = retrieve_succ(p);
			orig_color = y->color;
			x = y->right;
			if (y->parent != p) {
				Transplant(y, x);
				y->right = p->right;
				y->right->parent = y;
				parent = y->parent;
			} else {
				parent = y;
			}
			Transplant(p, y);
			y->left = p->left;
			y->left->parent = y;
			y->color = p->color;
		}

		delete p;

		if (orig_color == BLACK) {
			tree_delete_rebalance(x, parent);
		}
	}
	

	bool tree_increasment(tree_node *&ptr) const {
		if (ptr == header) {
			return false;
		} else {
			if (ptr->right) {
				ptr = retrieve_succ(ptr);
			} else {
				Key key_ptr = ptr->v->first;
				ptr = ptr->parent;
				while (ptr != header && Compare()(ptr->v->first, key_ptr)) {
					ptr = ptr->parent;
				}
			}
			return true;
		}
	}

	bool tree_decreasement(tree_node *&ptr) const {
		if (ptr == header->left) {
			return false;
		} else {
			if (ptr == header) {
				ptr = header->right;
				return ptr == header ? false : true;
			}
			if (ptr->left) {
				ptr = retrieve_pred(ptr);
			} else {
				Key key_ptr = ptr->v->first;
				ptr = ptr->parent;
				while (Compare()(key_ptr, ptr->v->first)) {
					ptr = ptr->parent;
				}
			}
			return true;
		}
	}

	tree_node* tree_copy(tree_node* this_from, tree_node *other_p, const map &other) {
		if (other_p == nullptr) {
			return nullptr;
		}
		tree_node* this_p;
		
		this_p = new tree_node(other_p->v->first, other_p->v->second, other_p->color, this_from);
		this_p->left = tree_copy(this_p, other_p->left, other);
		this_p->right = tree_copy(this_p, other_p->right, other);

		if (other_p == other.header->right)
			header->right = this_p;
        if (other_p == other.header->left) 
			header->left = this_p;

		return this_p;
	}

	void new_header() {
		header = new tree_node();
		header->left = header->right = header;
		header->parent = nullptr;
	}
public:
	class const_iterator;
	class iterator {
		friend class map;
		map *mp_belong;
		tree_node *ptr;
		iterator(map *mp_belong, tree_node *ptr) : mp_belong(mp_belong), ptr(ptr) {}

	public:
		iterator(): mp_belong(nullptr), ptr(nullptr) {} 
		iterator(const iterator &other): mp_belong(other.mp_belong), ptr(other.ptr) {}
		iterator(const const_iterator &other): mp_belong(other.mp_belong), ptr(other.ptr) {}

		iterator operator++(int) {
			auto tmp = ptr;
			if (mp_belong->tree_increasment(ptr)) 
				return iterator(mp_belong, tmp);
			throw invalid_iterator();
		}

		iterator & operator++() {
			if (mp_belong->tree_increasment(ptr)) 
				return *this;
			throw invalid_iterator();
		}

		iterator operator--(int) {
			auto tmp = ptr;
			if (mp_belong->tree_decreasement(ptr))
				return iterator(mp_belong, tmp);
			throw invalid_iterator();
		}

		iterator & operator--() {
			if (mp_belong->tree_decreasement(ptr)) 
				return *this;
			throw invalid_iterator();
		}

		
		bool operator==(const iterator &rhs) const {
			return mp_belong == rhs.mp_belong 
			       && ptr == rhs.ptr;
		}
		bool operator==(const const_iterator &rhs) const {
			return mp_belong == rhs.mp_belong
			       && ptr == rhs.ptr;
		}

		value_type & operator*() const { return *(ptr->v); }
		bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
		bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
		value_type* operator->() const noexcept { return ptr->v; }
	};
	class const_iterator {
		friend class map;
		const map *mp_belong;
		tree_node *ptr;
		const_iterator(const map* mp_belong, tree_node* ptr): mp_belong(mp_belong), ptr(ptr) {}
	public:
		const_iterator() : mp_belong(nullptr), ptr(nullptr) {} 
		const_iterator(const const_iterator &other): mp_belong(other.mp_belong), ptr(other.ptr) {}
		const_iterator(const iterator &other) : mp_belong(other.mp_belong), ptr(other.ptr) {}

		const_iterator operator++(int) {
			auto tmp = ptr;
			if (mp_belong->tree_increasment(ptr)) 
				return const_iterator(mp_belong, tmp);
			throw invalid_iterator();
		}

		const_iterator & operator++() {
			if (mp_belong->tree_increasment(ptr)) 
				return *this;
			throw invalid_iterator();
		}

		const_iterator operator--(int) {
			auto tmp = ptr;
			if (mp_belong->tree_decreasement(ptr))
				return const_iterator(mp_belong, tmp);
			throw invalid_iterator();
		}

		const_iterator & operator--() {
			if (mp_belong->tree_decreasement(ptr)) 
				return *this;
			throw invalid_iterator();
		}

		
		bool operator==(const iterator &rhs) const {
			return mp_belong == rhs.mp_belong 
				&& ptr == rhs.ptr;
		}
		bool operator==(const const_iterator &rhs) const {
			return mp_belong == rhs.mp_belong
				&& ptr == rhs.ptr;
		}

		value_type & operator*() const { return *(ptr->v); }
		bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
		bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
		value_type* operator->() const noexcept { return ptr->v; }
	};

	map() : tree_size(0) { new_header(); }

	map(const map &other) {
		if (other.empty()) {
			new_header();
			tree_size = 0;
		} else {
            new_header();
			header->parent = tree_copy(header, other.header->parent, other);
			tree_size = other.tree_size;
		}
	}

	int G(tree_node *root) const {
		if (!root) return 0;
		int aa = G(root->left);
		int bb = G(root->right);
		return aa > bb ? aa : bb + 1;
	}

	int get_depth() const {
		// debug
		return G(header->parent);
	}

	map & operator=(const map &other) {
		if (this != &other) {
			if (header)
				remove_tree_all(header->parent);
			else 
				new_header();
            header->parent = tree_copy(header, other.header->parent, other);
            tree_size = other.tree_size;
		}
		return *this;
	}

	~map() {
		remove_tree_all(header->parent);
		if (header) delete header;
	}

	T & at(const Key &key) {
		tree_node* res = tree_access(key);
		if (res) return res->v->second;
		throw index_out_of_bound();
	}
	const T & at(const Key &key) const {
		tree_node* res = tree_access(key);
		if (res) return res->v->second;
		throw index_out_of_bound();
	}

	T & operator[](const Key &key) {
		auto result = tree_get_or_create(key);
		return result.first->v->second;
	}

	const T & operator[](const Key &key) const {
		tree_node* res = tree_access(key);
		if (res) return res->v->second;
		throw index_out_of_bound();
	}

	iterator begin() { return iterator(this, header->left); }
	const_iterator cbegin() const { return const_iterator(this, header->left); }
	iterator end() { return iterator(this, header); }
	const_iterator cend() const { return const_iterator(this, header); }

	bool empty() const { return tree_size == 0; }
	size_t size() const { return tree_size; }
	size_t count(const Key &key) const { return tree_access(key) ? 1 : 0; }

	void clear() {
		remove_tree_all(header->parent);
		header->left = header->right = header;
		header->parent = nullptr;
		tree_size = 0;
	}

	pair<iterator, bool> insert(const value_type &value) {
		pair<tree_node*, bool> result = tree_get_or_create(value.first);
		if (result.second) 
			result.first->v->second = value.second;
		return make_pair(iterator(this, result.first), result.second);
	}

	void erase(iterator pos) {
		if (pos.mp_belong != this || pos.ptr == header) 
			throw invalid_iterator();
		tree_remove(pos.ptr);
	}

	iterator find(const Key &key) {
		tree_node *tmp = tree_access(key);
		return iterator(this, tmp ? tmp : header);
	}
	const_iterator find(const Key &key) const {
		tree_node *tmp = tree_access(key);
		return const_iterator(this, tmp ? tmp : header);
	}
};

}

#endif
