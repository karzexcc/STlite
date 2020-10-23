#include "utility.hpp"
#include "exceptions.hpp"
#include <cstring>
#include <utility>
#include <cstdio>
#include <limits>
#include <cassert>
#include <iostream>

#ifndef SJTU_DEQUE
#define SJTU_DEQUE
  
namespace sjtu {

    template <typename T1>
    void swap(T1 &a, T1&b) {
        T1 c = b;
        c = a;
        a = c;
    }

    // using block list to implement the deque.
    template<class Tp>
    class deque {
    public:
#ifndef DEBUG
        static constexpr size_t max_size = 1024;
        static constexpr size_t half = 512;
        static constexpr size_t init_position = 345;
        static constexpr size_t min_size = 300;
        static constexpr size_t sizeof_T = sizeof(Tp);
#else
        static constexpr size_t max_size = 10;
        static constexpr size_t half = 5;
        static constexpr size_t init_position = 3;
        static constexpr size_t min_size = 3;
        static constexpr size_t sizeof_T = sizeof(Tp);
#endif

        friend class Block;
        friend class iterator;
        friend class const_iterator;

        struct T {
            Tp *v;
            T(): v(nullptr) {}
            T(const Tp& v): v(new Tp(v)) {}
            Tp& operator()() const {
                return v;
            }
            T& operator = (const Tp& other) {
                if (!v) {
                    v = new Tp(other);
                } else {
                    *v = other;
                }
                return *this;
            }
            T& operator = (const T& other) {
                if (v) {
                    *v = *(other.v);
                } else {
                    v = new Tp(*(other.v));
                }
                return *this;
            }
            ~T() {
                if (v) delete v;
            }
        };

        

        // the block of the blockList.
        struct Block {
            T* data;
            size_t start, end;
            Block *prev, *next;

            Block(Block *next) {
                data = new T[max_size];
                this->prev = nullptr;
                this->next = next;
                start = init_position;
                end = init_position;
            }

            Block() {
                data = nullptr;
                prev = next = nullptr;
                start = end = init_position;
            }

            Block(const Block& other) {
                if (this != &other) {
                    start = other.start;
                    end = other.end;
                    data = new T[max_size];
                    for (int i = start; i < end; ++i) {
                        data[i] = other.data[i];
                    }
                    prev = next = nullptr;
                }
            }

            Block& operator = (const Block& other) {
                // DO NOT copy prev, next here.
                // this will be handled outside the class Block.
                data = new T[max_size];
                start = other.start;
                end = other.end;
                for (int i = start; i < end; ++i) {
                    data[i] = other.data[i];
                }
                return *this;
            }

            Block& operator = (Block &&other) {
                if (this != &other) {
                    data = other.data;
                    start = other.start;
                    end = other.end;
                    other.data = nullptr;
                }
                return *this;
            }

            ~Block() {
                if (data) {
                    delete []data;
                }
            }

            Block(Block* other, size_t st, size_t ed): prev(nullptr), next(nullptr) {
                // move other blocks' data from st to ed to the new block.
                // mainly for split blocks.

                data = new T[max_size];
                start = init_position;
                end = init_position + (ed - st);
                for (int i = st; i < ed; ++i) {
#ifdef DEBUG
                    printf("odt[%d]: %d\n", i, other->data[i]);
#endif
                    
                    data[init_position - st + i] = other->data[i];
                }
            }

            Block(Block* left, Block* right) {
                //merge left and right.
                data = new T[max_size];
                size_t szl = left->size(), szr = right->size();
                start = init_position;
                end = init_position + szl + szr;
                for (int i = left->start; i < left->end; ++i) {
                    data[i - left->start + start] = left->data[i];
                }
                for (int i = right->start; i < right->end; ++i) {
                    data[i - right->start + szl + start] = right->data[i];
                }
                prev = next = nullptr;
            }

            int size() const {
                return end - start;
            }

            bool empty() const {
                return size() == 0;
            }

            Tp& get(size_t k) const {
                return *(data[start + k].v);
            }

            void move_forward(size_t x) {
                for (int i = end + x - 1; i >= start + x; --i) {
                    data[i] = data[i - x];
                }
                start += x;
                end += x;
            }

            void move_backward(size_t x) {
                for (int i = start - x; i < end - x; ++i) {
                    data[i] = data[i + x];
                }
                start -= x;
                end -= x;
            }

            void insert_to(const T&x, size_t pos) {
                pos += start;
                if (pos == start) {
                    data[--start] = x;
                }
                else if (pos == end) {
                    data[end++] = x;
                }
                else {
                    end++;
                    for (int i = end - 1; i > pos; --i) {
                        data[i] = data[i-1];
                    }
                    data[pos] = x;
                }
            }

            void remove(size_t pos) {
                pos += start;
                if (pos == start) {
                    delete data[start].v;
                    data[start].v = nullptr;
                    ++start;
                }
                else if (pos == end-1) {
                    --end;
                    delete data[end].v;
                    data[end].v = nullptr;
                }
                else {
                    swap(data[pos].v, data[end-1].v);
                    swap(*(data[pos].v), *(data[end-1].v));
                    for (int i = pos; i < end - 1; ++i) {
                        data[i] = data[i+1];
                    }
                    delete data[end-1].v;
                    data[end-1].v = nullptr;
                    --end;
                }
            }

        } *head, *tail;

        size_t total_size;
        size_t n_blocks;

        Block* split_block(Block* x) {
            // split x into l and r.
            // return the new block l.
            n_blocks++;
            size_t middle = x->start + (x->end - x->start) / 2;
            Block *new_left = new Block(x, x->start, middle);
            Block *new_right = new Block(x, middle, x->end);
            new_left->next = new_right;
            new_right->prev = new_left;

            Block *prev = x->prev, *next = x->next;
            if (prev) {
                prev->next = new_left;
                new_left->prev = prev;
            }
            if (next) {
                next->prev = new_right;
                new_right->next = next;
            }
            if (head == x) {
                head = new_left;
            }
            delete x;
            return new_left;
        }

        Block* merge(Block* l, Block* r) {
            //merge the block l and block r.
            //return the new block.
            --n_blocks;
            Block *new_block = new Block(l, r);
            new_block->prev = l->prev;
            if (l->prev) l->prev->next = new_block;
            new_block->next = r->next;
            if (r->next) r->next->prev = new_block;
            if (head == l) {
                head = new_block;
            }
            delete l;
            delete r;
            return new_block;
        }

        Tp& access(size_t k) const {
            auto p = head;
            while (k >= p->size()) {
                k -= p->size();
                p = p->next;
            }
            return p->get(k);
        }

        Block* access_p(size_t& pos, Block *q , bool _throw = true) const {
            if (pos > total_size && _throw) {
                throw index_out_of_bound();
            }
            auto p = q;
            size_t sz;
            while (p && pos > (sz = p->size())) {
                pos -= sz;
                p = p->next;
            }
            return p;
        }

        Block* access_p_le(size_t& pos, Block *q , bool _throw = true) const {
            if (pos > total_size && _throw) {
                throw index_out_of_bound();
            }
            auto p = q;
            size_t sz;
            while (p && pos >= (sz = p->size())) {
                pos -= sz;
                p = p->next;
            }
            return p;
        }

        Block* try_merge(Block* p) {
            if (n_blocks > 1 && p->size() < min_size) {
                // satisfy: block number > 1
                // size of the block p is less than the min_size(300).
                size_t size_l = p->prev ? p->prev->size() : 0;
                size_t size_r = p->next ? p->next->size() : 0;
                // select from its prev and next.
                // merge to the minimal one if it's possible.

                if (size_l == 0) {
                    if (size_r != 0 && size_r + p->size() <= max_size - init_position) {
                        return merge(p, p->next);
                    }
                }
                else if (size_r == 0) {
                    if (size_l != 0 && size_l + p->size() <= max_size - init_position) {
                        return merge(p->prev, p);
                    }
                }
                else if (size_l > size_r) {
                    if (size_r + p->size() <= max_size - init_position) {
                        return merge(p, p->next);
                    }
                }
                else if (size_l + p->size() <= max_size - init_position) {
                    return merge(p->prev, p);
                }
            }

            return p;
        }

        Block* try_merge(Block* p, int &offset) {
            if (n_blocks > 1 && p->size() < min_size) {
                // satisfy: block number > 1
                // size of the block p is less than the min_size(300).
                size_t size_l = p->prev ? p->prev->size() : 0;
                size_t size_r = p->next ? p->next->size() : 0;
                // select from its prev and next.
                // merge to the minimal one if it's possible.

                if (size_l == 0) {
                    if (size_r != 0 && size_r + p->size() <= max_size - init_position) {
                        return merge(p, p->next);
                    }
                }
                else if (size_r == 0) {
                    if (size_l != 0 && size_l + p->size() <= max_size - init_position) {
                        offset = p->prev->size();
                        return merge(p->prev, p);
                    }
                }
                else if (size_l > size_r) {
                    if (size_r + p->size() <= max_size - init_position) {
                        return merge(p, p->next);
                    }
                }
                else if (size_l + p->size() <= max_size - init_position) {
                    offset = p->prev->size();
                    return merge(p->prev, p);
                }
            }

            return p;
        }

        Block* try_remove_chunk(Block *p) {
            
            if (p->start == p->end && p != tail) {
                Block *tmp = p->next;
                if (p == head) {
                    if (p->next == tail)
                        return p;
                    else 
                        head = tmp;
                } else {
                    p->prev->next = p->next;
                }
                p->next->prev = p->prev;
                delete p;
                if (tmp == tail) {
                    tmp = tmp->prev;
                }
                return tmp;
            }

            return p;
        }

        Block* try_remove_chunk(Block *p, int &offset) {
            
            if (p->start == p->end && p != tail) {
                Block *tmp = p->next;
                
                if (p == head) {
                    if (p->next == tail)
                        return p;
                    else 
                        head = tmp;
                } else {
                    p->prev->next = p->next;
                }
                offset ++;
                p->next->prev = p->prev;
                delete p;
                if (tmp == tail) {
                    tmp = tmp->prev;
                    offset = tmp->size();
                }
                return tmp;
            }

            return p;
        }

        void remove(size_t pos) {
            Block *p;
            if (pos == total_size - 1) {
                p = tail->prev;
                pos = tail->prev->size() - 1;
            } else {
                p = access_p_le(pos, head);
            }
            --total_size;
            p->remove(pos);
            try_remove_chunk(p);
            try_merge(p);
        }

        size_t calc_offset(Block *p, size_t offset) const {
            int ret = 0;
            for (Block *q = head; q != p; q = q->next) {
                ret += q->size();
            }
            return ret + offset;
        }

        void insert_front(const T& x, Block* p) {
            if (p->start == 0) {
                    //left full
                if (p->size() <= half) {
                    // if the size of the block is less than half,
                    // then the right size contains very few data.
                    // thus we move the data forward.
                    p->move_forward(init_position);
                    p->insert_to(x, 0);
                } else {
                    // else, we split it into two new Block.
                    Block* new_left = split_block(p);
                    new_left->insert_to(x, 0);
                }
            } else {
                p->insert_to(x, 0);
            }
        }

        void insert_back(const T& x, Block* p) {
            if (p->end == max_size) {
                //right full
                if (p->size() <= half) {
                    p->move_backward(init_position);
                    p->insert_to(x, p->size());
                } else {
                    Block *new_right = split_block(p)->next;
                    new_right->insert_to(x, new_right->size());
                }
            } else {
                p->insert_to(x, p->size());
            }
        }

        void __insert(const Tp& x, size_t pos) {
            // insert to the before of pos.
            auto p = access_p(pos, head);
            ++total_size;

            if (pos == 0) {
                //insert to the front.
                if (p != tail) {
                    insert_front(x, p);
                } else {
                    p = new Block(tail);
                    p->insert_to(x, 0);
                    ++n_blocks;
                    p->prev = tail->prev;
                    tail->prev->next = p;
                    tail->prev = p;
                }
            }
            else if (pos == p->size()) {
                //insert to the back.
                insert_back(x, p);
            }
            else {
                if (p->size() == max_size) {
                    // full: split the block. and insert to some place of 
                    // the new left or the new right block.
                    auto q = split_block(p);
                    if (pos >= half) {
                        q = q->next;
                        pos -= half;
                    }
                    q->insert_to(x, pos);
                } else {
                    p->insert_to(x, pos);
                }
            }
        }

        void remove_from_head() {
            auto p = head;
            while (p && p != tail) {
                auto q = p->next;
                delete p;
                p = q;
            }
        }


    public:
        int nb() const {
            return n_blocks;
        }
        class const_iterator;

        class iterator {
            friend class deque;
            friend class const_iterator;
        private:
            Block *cur;
            deque *belong;
            size_t index;
            /**
             * TODO add data members
             *   just add whatever you want.
             */
            iterator(deque* belong, Block* b, size_t index): belong(belong), cur(b), index(index) {}
        public:
            /**
             * return a new iterator which pointer n-next elements
             *   even if there are not enough elements, the behaviour is **undefined**.
             * as well as operator-
             */
            iterator():cur(nullptr), belong(nullptr) {}
            iterator(const iterator& other): cur(other.cur), belong(other.belong), index(other.index) {}
            iterator(const const_iterator& other): cur(other.cur), belong(other.belong), index(other.index) {}

            iterator move_right(int n) const {
                Block *p = cur;
                int new_index = index;
                if (p == belong->tail->prev) {
                    return iterator(belong, p, index + n);
                }
                if (n < (int)p->end - new_index) {
                    new_index += n;
                } else {
                    n -= p->end - new_index;
                    p = p->next;
                    while (n > 0 && n >= p->size()) {
                        n -= p->size();
                        p = p->next;
                    } 
                    new_index = n + p->start;
                }
                if (p == belong->tail) {
                    return belong->end();
                }
                return iterator(belong, p, new_index);
            }

            iterator move_left(int n) const {
                Block *p = cur;
                if (p == belong->head) {
                    return iterator(belong, p, index - n);
                }
                int new_index = index;
                if (new_index - n >= (int)p->start) {
                    new_index -= n;
                } else {
                    n -= (new_index - p->start + 1);
                    p = cur->prev;
                    new_index = cur->end - 1;
                    while (n > 0 && n >= p->size()) {
                        n -= p->size();
                        p = p->prev;
                    }
                    new_index = p->end - n - 1;
                }
                return iterator(belong, p, new_index);
            }

            iterator operator+(int n) const { return n > 0 ? move_right(n) : move_left(-n); }

            iterator operator-(int n) const { return n < 0 ? move_right(-n) : move_left(n); }

            // return th distance between two iterator,
            // if these two iterators points to different vectors, throw invaild_iterator.
            int operator-(const iterator &rhs) const {
                if (belong != rhs.belong) {
                    throw invalid_iterator();
                }
                int l1 = belong->calc_offset(cur, index - cur->start);
                int l2 = belong->calc_offset(rhs.cur, rhs.index - rhs.cur->start);
                return l1 - l2;
            }

            int operator-(const const_iterator &rhs) const {
                if (belong != rhs.belong) {
                    throw invalid_iterator();
                }
                int l1 = belong->calc_offset(cur, index - cur->start);
                int l2 = belong->calc_offset(rhs.cur, rhs.index - rhs.cur->start);
                return l1 - l2;
            }

            iterator& operator+=(int n) { return *this = (*this + n); }
            iterator& operator-=(int n) { return *this = (*this - n); }
            iterator &operator--() { return *this -= 1; }
            iterator &operator++() { return *this += 1; }

            iterator operator++(int) {
                iterator tmp = *this;
                *this += 1;
                return tmp;
            }

            iterator operator--(int) {
                iterator tmp = *this;
                *this -= 1;
                return tmp;
            }

            

            Tp &operator*() const {
                if (!cur) {
                    throw invalid_iterator();
                }
                if (cur->start > index || cur->end <= index) {
                    throw invalid_iterator();
                }
                return *(cur->data[index].v);
            }

            bool operator==(const iterator &rhs) const {
                return belong == rhs.belong
                       && cur == rhs.cur
                       && index == rhs.index;
            }

            bool operator==(const const_iterator &rhs) const {
                return belong == rhs.belong
                       && cur == rhs.cur
                       && index == rhs.index;
            }

            bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
            bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
            Tp *operator->() const noexcept { return cur->data[index].v; }
        };

        class const_iterator {
            friend class deque;
            friend class iterator;

            Block *cur;
            const deque *belong;
            size_t index;
            const_iterator(const deque* belong, Block* b, const size_t index): belong(belong), cur(b), index(index) {}
        public:
            const_iterator() : cur(nullptr), belong(nullptr), index(0) {}
            const_iterator(const iterator& other): cur(other.cur), belong(other.belong), index(other.index) {}
            const_iterator(const const_iterator& other): cur(other.cur), belong(other.belong), index(other.index) {}

            const_iterator move_right(int n) const {
                Block *p = cur;
                int new_index = index;
                if (p == belong->tail->prev) {
                    return const_iterator(belong, p, index + n);
                }
                if (n < (int)p->end - new_index) {
                    new_index += n;
                } else {
                    n -= p->end - new_index;
                    p = p->next;
                    while (n > 0 && n >= p->size()) {
                        n -= p->size();
                        p = p->next;
                    } 
                    new_index = n + p->start;
                }
                if (p == belong->tail) {
                    return belong->cend();
                }
                return const_iterator(belong, p, new_index);
            }

            const_iterator move_left(int n) const {
                Block *p = cur;
                if (p == belong->head) {
                    return const_iterator(belong, p, index - n);
                }
                int new_index = index;
                if (new_index - n >= (int)p->start) {
                    new_index -= n;
                } else {
                    n -= (new_index - p->start + 1);
                    p = cur->prev;
                    new_index = cur->end - 1;
                    while (n > 0 && n >= p->size()) {
                        n -= p->size();
                        p = p->prev;
                    }
                    new_index = p->end - n - 1;
                }
                return const_iterator(belong, p, new_index);
            }

            const_iterator operator+(int n) const { return n > 0 ? move_right(n) : move_left(-n); }
            const_iterator operator-(int n) const { return n < 0 ? move_right(-n) : move_left(n); }

            int operator-(const const_iterator &rhs) const {
                if (belong != rhs.belong) {
                    throw invalid_iterator();
                }
                int l1 = belong->calc_offset(cur, index - cur->start);
                int l2 = belong->calc_offset(rhs.cur, rhs.index - rhs.cur->start);
                return l1 - l2;
            }

            int operator-(const iterator &rhs) const {
                if (belong != rhs.belong) {
                    throw invalid_iterator();
                }
                int l1 = belong->calc_offset(cur, index - cur->start);
                int l2 = belong->calc_offset(rhs.cur, rhs.index - rhs.cur->start);
                return l1 - l2;
            }
            
            const Tp &operator*() const { return *(cur->data[index].v); }
            const Tp *operator->() const noexcept { return cur->data[index].v; }

            bool operator==(const iterator &rhs) const {
                return belong == rhs.belong
                       && cur == rhs.cur
                       && index == rhs.index;
            }

            bool operator==(const const_iterator &rhs) const {
                return belong == rhs.belong
                       && cur == rhs.cur
                       && index == rhs.index;
            }

            bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
            bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
            const_iterator& operator+=(int n) { return *this = (*this + n); }
            const_iterator& operator-=(int n) { return *this = (*this - n); }
            const_iterator &operator++() { return *this += 1; }
            const_iterator &operator--() { return *this -= 1; }

            const_iterator operator++(int) {
                iterator tmp = *this;
                *this += 1;
                return tmp;
            }

            

            const_iterator operator--(int) {
                iterator tmp = *this;
                *this -= 1;
                return tmp;
            }

            
        };

        deque() {
            total_size = 0;
            n_blocks = 0;
            tail = new Block();
            head = new Block(tail);
            tail->prev = head;
        }

        deque(const deque &other) {
            tail = new Block();
            total_size = other.total_size;
            n_blocks = other.n_blocks;

            head = new Block(*(other.head));
            auto q = head;

            for (auto p = other.head->next; p != other.tail; p = p->next) {
                q->next = new Block(*p);
                q->next->prev = q;
                q = q->next;
            }

            tail->prev = q;
            q->next = tail;
		}

        ~deque() {
            remove_from_head();
            delete tail;
		}

        deque &operator=(const deque &other) {
            if (this == &other) {
                return *this;
            }
            n_blocks = other.n_blocks;
            total_size = other.total_size;
            if (!tail)
                tail = new Block();
            remove_from_head();
            head = new Block(*(other.head));
            auto p = head, q = other.head->next;
            while (q != other.tail) {
                auto r = new Block(*q);
                p->next = r;
                r->prev = p;
                p = r;
                q = q->next;
            }
            tail->prev = p;
            p->next = tail;
            return *this;
		}	

        Tp &at(const size_t &pos) {
            if (pos >= total_size) {
                throw index_out_of_bound();
            }
            return access(pos);
		}

        const Tp &at(const size_t &pos) const {
            if (pos >= total_size) {
                throw index_out_of_bound();
            }
            return access(pos);
		}

        Tp &operator[](const size_t &pos) {
            if (pos >= total_size) {
                throw index_out_of_bound();
            }
            return access(pos);
		}

        const Tp &operator[](const size_t &pos) const {
            if (pos >= total_size) {
                throw index_out_of_bound();
            }
            return access(pos);
		}

        const Tp &front() const {
            if (head == nullptr || head->start == head->end) {
                throw container_is_empty();
            }
            return *(head->data[head->start].v);
		}

        const Tp &back() const {
            if (head == nullptr || head->start == head->end) {
                throw container_is_empty();
            }
            return *(tail->prev->data[tail->prev->end - 1].v);
        }

        iterator begin() {
            return iterator(this, head, head->start);
        }

        const_iterator cbegin() const {
            return const_iterator(this, head, head->start);
        }

        iterator end() {
            return iterator(this, tail->prev, tail->prev->end);
        }

        const_iterator cend() const {
            return const_iterator(this, tail->prev, tail->prev->end);
        }

        bool empty() const {
            return total_size == 0;
		}

        size_t size() const {
            return total_size;
		}

        void clear() {
            n_blocks = 0;
            total_size = 0;
            auto p = head;
            while (p && p != tail) {
                auto q = p->next;
                delete p;
                p = q;
            }
            head = new Block(tail);
            tail->prev = head;
        }

        iterator insert(iterator pos, const Tp &value) {
            if (pos.belong != this || !pos.cur || pos.cur == tail || pos.index > pos.cur->end || pos.index < pos.cur->start) {
                throw invalid_iterator();
            }

            ++total_size;
            
            if (pos.cur->end == max_size) {
                int index_saved = pos.index;
                int start_saved = pos.cur->start;
                Block* new_p;
                if (pos.cur->size() >= half) {
                    new_p = split_block(pos.cur);
                } else {
                    pos.cur->move_backward(half);
                    new_p = pos.cur;
                }
                int new_index = index_saved - start_saved;
                if (new_index >= new_p->size()) {
                    new_index -= new_p->size();
                    new_p = new_p->next;
                }
                new_p->insert_to(value, new_index);
                return iterator(this, new_p, new_index + new_p->start);
            } 
            else if (pos.index == pos.cur->start && pos.cur->start == 0) {
                Block* new_p;
                if (pos.cur->size() >= half) {
                    new_p = split_block(pos.cur);
                } else {
                    pos.cur->move_forward(half);
                    new_p = pos.cur;
                }
                new_p->insert_to(value, 0);
                return iterator(this, new_p, new_p->start);
            } else {
                bool insert_to_front = pos.index == pos.cur->start;
                pos.cur->insert_to(value, pos.index - pos.cur->start);
                return insert_to_front
                       ? iterator(this, pos.cur, pos.index - 1)
                       : iterator(this, pos.cur, pos.index);
            }
        }

        iterator erase(iterator pos) {
            if (empty()) {
                throw container_is_empty();
            }
            if (pos.belong != this || !pos.cur || pos.cur == tail || pos.index > pos.cur->end || pos.index < pos.cur->start) {
                throw invalid_iterator();
            }
            --total_size;
            int new_index = pos.index - pos.cur->start;
            int offset = 0;
            pos.cur->remove(new_index);
            auto p = try_remove_chunk(pos.cur, offset);
            p = try_merge(p, offset);
            new_index += offset;

            if (new_index >= p->size()) {
                if (p->next != tail) {
                    new_index -= p->size();
                    p = p->next;
                }
            }

            return iterator(this, p, new_index + p->start);
        }

        void push_back(const Tp &value) {
            ++total_size;
            insert_back(value, tail->prev);
        }

        void pop_back() {
            if (total_size == 0) 
                throw container_is_empty();
            remove(total_size - 1);
        }

        void push_front(const Tp &value) { __insert(value, 0); }

        void pop_front() {
            if (total_size == 0) 
                throw container_is_empty();
            remove(0);
        }
    };
}

#endif
