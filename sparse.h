#ifndef SPARSE_H
#define SPARSE_H

#include <vector>

/* Represents a sparse vector of elements for constant-time insert/delete. */


template <typename T>
class Sparse {
public:
    class Key {
    public:
        Key(size_t index) : index(index) {}
        Key(const Key &key) : index(key.index) {}
        void operator=(const Key &key) { index = key.index; }
        size_t index;
    };


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
        assert(index < elements.size());

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


private:

    std::vector<size_t> emptyIndices;
    std::vector<T> elements;
};



#endif
