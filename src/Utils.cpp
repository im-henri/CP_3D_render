#include "Utils.hpp"

void bubble_sort(uint_fix16_t a[], int n) {
    for (int j = n; j > 1; --j)
        for (int i = 1; i < j; ++i)
            if (a[i - 1].fix16 < a[i].fix16)
                swap(a[i - 1], a[i]);
}