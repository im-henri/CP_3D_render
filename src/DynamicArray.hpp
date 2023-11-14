#pragma once

// malloc & free based dynamically sized array

#ifndef PC
#   include <sdk/os/mem.hpp>
#else
#   include <cstdlib>
#   include <iostream>
#endif

// If we are using array then we can probably assume that there is going to be more than 1 item
// -> First malloc size.
#define DYNAMIC_ARRAY_INITAL_SIZE 2

template <typename T>
class DynamicArray {
private:
    T* array;
    unsigned int size;
    unsigned int capacity;

public:
    DynamicArray() : array(nullptr), size(0), capacity(0) { }

    bool push_back(const T& value)
    {
        // Check if size needs to be grown
        if (size == capacity) {
            unsigned int newCapacity = (capacity == 0) ? DYNAMIC_ARRAY_INITAL_SIZE : capacity * 2;
            bool success = reserve(newCapacity);
            if(success == false)
                return false;
        }
        array[size++] = value;
        return true;
    }

    bool reserve(unsigned int newCapacity)
    {
        // Requested capacity smaller than inital
        if (newCapacity <= capacity)
            return true; // Consider it as success

        // Create new array
        T* newArray = static_cast<T*>(malloc(newCapacity * sizeof(T)));
        if (!newArray)
            return false;

        // Copy data from old array to new
        if (array) {
            for (unsigned int i = 0; i < size; ++i)
                newArray[i] = array[i];
            // Free the old array
            free(array);
        }

        // Assign new array
        array = newArray;
        capacity = newCapacity;

        return true;
    }

    unsigned int getSize() const {
        return size;
    }

    // Warning: Not checking bounds -> Unsafe to access out of bounds!
    T& operator[](unsigned int index)
    {
        return array[index];
    }

    ~DynamicArray()
    {
        // Free the dynamically allocated memory
        if (array) {
            free(array);
        }
    }
};
