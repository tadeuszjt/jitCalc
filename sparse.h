#ifndef SPARSE_H
#define SPARSE_H

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <vector>

/* Represents a sparse vector of elements for constant-time insert/delete. */


template <typename T>
class Sparse {
public:
    class Key {
    public:
        Key(size_t idx) : index(idx) {}
        Key(const Key &key) : index(key.index) {}
        Key() : index(~(size_t)0x0) {}
        void operator=(const Key &key) { index = key.index; }
        size_t index;
    };

    Key insert(const T& elem) {
        size_t index = 0;

        if (emptyIndices.size() > 0) {
            index = emptyIndices.back();
            elements[index] = elem;
            emptyIndices.pop_back();
        } else {
            index = elements.size();
            elements.push_back(elem);
        }

        return Key(index);
    }


    Key insert() {
        size_t index = 0;

        if (emptyIndices.size() > 0) {
            index = emptyIndices.back();

            elements[index].~T();
            memset(&elements[index], 0, sizeof(T));
            new (&elements[index]) T();

            emptyIndices.pop_back();
        } else {
            index = elements.size();
            elements.emplace_back();
        }

        return Key(index);
    }

    void remove(Key key) {
        assert(elements.size() > 0);
        assert(key.index < elements.size());

        if (key.index == elements.size() - 1) {
            elements.pop_back();
        } else {
            emptyIndices.push_back(key.index);
        }
    }

    T& at(Key key) {
        assert(key.index < elements.size());
        return elements[key.index];
    }

    size_t size() {
        return elements.size() - emptyIndices.size();
    }

    void clear() {
         emptyIndices.clear();
         elements.clear();
    }


private:

    std::vector<size_t> emptyIndices;
    std::vector<T> elements;
};



#endif
