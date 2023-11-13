#pragma once
#include "RenderFP3D.hpp"

template <class T>
inline void swap(T& a, T& b) {
    T tmp = b;
    b = a;
    a = tmp;
}

void bubble_sort(uint_fix16_t a[], int n);
