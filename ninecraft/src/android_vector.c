#include <ninecraft/android_vector.h>
#include <hybris/dlfcn.h>
#include <ninecraft/android_alloc.h>
#include <string.h>

size_t android_vector$_M_compute_next_size(android_vector *__this, size_t __n) {
    size_t __size = (__this->_M_finish - __this->_M_start) / 24;
    if (__n > STLPORT_MAX_SIZE - __size) {
        puts("Invalid vector length");
        abort();
    }
    size_t __len = __size + MAX(__n, __size) + 1;
    if (__len < __size) {
        __len = STLPORT_MAX_SIZE;
    }
    return __len;
}

uintptr_t android_vector$__uninitialized_fill(uintptr_t __first, size_t __n, uintptr_t __x, void *handle) {
    while (__n > 0) {
        android_string_clone((android_string_t *)__first, (android_string_t *)__x, handle);
        --__n;
        __first += 24;
    }
    return __first;
}

uintptr_t android_vector$__uninitialized_move(uintptr_t __first, uintptr_t __last, uintptr_t __result) {
    uint32_t __cur = (uint32_t)__result;
    uint32_t diff = ((uint32_t)__last - (uint32_t)__first) / 24;
    for (uint32_t __n = diff; __n > 0; --__n) {
        android_string_move_src((android_string_t *)__cur, (android_string_t *)__first);
        __first += 24;
        __cur += 24;
    }
    return __cur;
}

void android_vector$_M_insert_overflow_aux(android_vector *__this, void *__pos, void *__x, void *reserved, size_t __fill_len, bool __atend, void *handle) {
    size_t __len = android_vector$_M_compute_next_size(__this, __fill_len);
    size_t __size = __len * 24;
    uintptr_t __new_start = (uintptr_t)android_alloc_allocate((uint32_t *)&__size, handle);
    uintptr_t __new_finish = android_vector$__uninitialized_move(__this->_M_start, (uintptr_t)__pos, __new_start);
    if (__fill_len == 1) {
        android_string_clone((android_string_t *)__new_finish, (android_string_t *)__x, handle);
        __new_finish += 24;
    } else {
        __new_finish = android_vector$__uninitialized_fill(__new_finish, __fill_len, (uintptr_t)__x, handle);
    }
    if (!__atend) {
        __new_finish = android_vector$__uninitialized_move((uintptr_t)__pos, __this->_M_finish, __new_finish);
    }
    if (__this->_M_start) {
        android_alloc_deallocate((void *)__this->_M_start, (__this->_M_end_of_storage - __this->_M_start) & 0xFFFFFFF8, handle);
    }
    __this->_M_start = __new_start;
    __this->_M_finish = __new_finish;
    __this->_M_end_of_storage = __new_start + 24 * __len;
}

void android_vector$push_back(android_vector *this_vector, android_string_t *item, void *handle) {
    if (this_vector->_M_finish == this_vector->_M_end_of_storage) {
        android_vector$_M_insert_overflow_aux(this_vector, (void *)this_vector->_M_finish, item, 0, 1, 1, handle);
    } else {
        if (this_vector->_M_finish) {
            android_string_clone((android_string_t *)this_vector->_M_finish, item, handle);
        }
        this_vector->_M_finish += 24;
    }
}

size_t android_vector$_M_compute_next_size_2(android_vector *__this, size_t __n) {
    size_t __size = (__this->_M_finish - __this->_M_start) >> 3;
    if (__n > 0x1FFFFFFF - __size) {
        puts("Invalid vector length");
        abort();
    }
    size_t __len = __size + MAX(__n, __size) + 1;
    if (__len < __size) {
        __len = 0x1FFFFFFF;
    }
    return __len;
}

uintptr_t android_vector$__uninitialized_fill_2(uintptr_t __first, size_t __n, uintptr_t __x) {
    while (__n > 0) {
        memcpy((void *)__first, (void *)__x, 8);
        --__n;
        __first += 8;
    }
    return __first;
}

uintptr_t android_vector$__uninitialized_move_2(uintptr_t __first, uintptr_t __last, uintptr_t __result) {
    uint32_t __cur = (uint32_t)__result;
    uint32_t diff = (uint32_t)(((uint32_t)__last - (uint32_t)__first) >> 3);
    for (uint32_t __n = diff; __n > 0; --__n) {
        memcpy((void *)__cur, (void *)__first, 8);
        __first += 8;
        __cur += 8;
    }
    return __cur;
}

void android_vector$_M_insert_overflow_aux_2(android_vector *__this, void *__pos, void *__x, void *reserved, size_t __fill_len, bool __atend, void *handle) {
    size_t __len = android_vector$_M_compute_next_size_2(__this, __fill_len);
    size_t __size = __len * 8;
    uintptr_t __new_start = (uintptr_t)android_alloc_allocate((uint32_t *)&__size, handle);
    uintptr_t __new_finish = android_vector$__uninitialized_move_2(__this->_M_start, (uintptr_t)__pos, __new_start);
    if (__fill_len == 1) {
        memcpy((void *)__new_finish, __x, 8);
        __new_finish += 8;
    } else {
        __new_finish = android_vector$__uninitialized_fill_2(__new_finish, __fill_len, (uintptr_t)__x);
    }
    if (!__atend) {
        __new_finish = android_vector$__uninitialized_move_2((uintptr_t)__pos, __this->_M_finish, __new_finish);
    }
    if (__this->_M_start) {
        android_alloc_deallocate((void *)__this->_M_start, (__this->_M_end_of_storage - __this->_M_start) & 0xFFFFFFF8, handle);
    }
    __this->_M_start = __new_start;
    __this->_M_finish = __new_finish;
    __this->_M_end_of_storage = __new_start + 8 * __len;
}

void android_vector$push_back_2(android_vector *this_vector, void *item, void *handle) {
    if (this_vector->_M_finish == this_vector->_M_end_of_storage) {
        android_vector$_M_insert_overflow_aux_2(this_vector, (void *)this_vector->_M_finish, item, 0, 1, 1, handle);
    } else {
        if (this_vector->_M_finish) {
            memcpy((void *)this_vector->_M_finish, item, 8);
        }
        this_vector->_M_finish += 8;
    }
}


size_t android_vector$_M_compute_next_size_3(android_vector *__this, size_t __n) {
    size_t __size = (__this->_M_finish - __this->_M_start) / 12;
    if (__n > 0x15555555 - __size) {
        puts("Invalid vector length");
        abort();
    }
    size_t __len = __size + MAX(__n, __size) + 1;
    if (__len < __size) {
        __len = 0x15555555;
    }
    return __len;
}

uintptr_t android_vector$__uninitialized_fill_3(uintptr_t __first, size_t __n, uintptr_t __x) {
    while (__n > 0) {
        memcpy((void *)__first, (void *)__x, 12);
        --__n;
        __first += 12;
    }
    return __first;
}

uintptr_t android_vector$__uninitialized_move_3(uintptr_t __first, uintptr_t __last, uintptr_t __result) {
    uint32_t __cur = (uint32_t)__result;
    uint32_t diff = (uint32_t)(((uint32_t)__last - (uint32_t)__first) / 12);
    for (uint32_t __n = diff; __n > 0; --__n) {
        memcpy((void *)__cur, (void *)__first, 12);
        __first += 12;
        __cur += 12;
    }
    return __cur;
}

void android_vector$_M_insert_overflow_aux_3(android_vector *__this, void *__pos, void *__x, void *reserved, size_t __fill_len, bool __atend, void *handle) {
    size_t __len = android_vector$_M_compute_next_size_3(__this, __fill_len);
    size_t __size = __len * 12;
    uintptr_t __new_start = (uintptr_t)android_alloc_allocate((uint32_t *)&__size, handle);
    uintptr_t __new_finish = android_vector$__uninitialized_move_3(__this->_M_start, (uintptr_t)__pos, __new_start);
    if (__fill_len == 1) {
        memcpy((void *)__new_finish, __x, 12);
        __new_finish += 12;
    } else {
        __new_finish = android_vector$__uninitialized_fill_3(__new_finish, __fill_len, (uintptr_t)__x);
    }
    if (!__atend) {
        __new_finish = android_vector$__uninitialized_move_3((uintptr_t)__pos, __this->_M_finish, __new_finish);
    }
    if (__this->_M_start) {
        android_alloc_deallocate((void *)__this->_M_start, (__this->_M_end_of_storage - __this->_M_start) & 0xFFFFFFF8, handle);
    }
    __this->_M_start = __new_start;
    __this->_M_finish = __new_finish;
    __this->_M_end_of_storage = __new_start + 12 * __len;
}

void android_vector$push_back_3(android_vector *this_vector, void *item, void *handle) {
    if (this_vector->_M_finish == this_vector->_M_end_of_storage) {
        android_vector$_M_insert_overflow_aux_3(this_vector, (void *)this_vector->_M_finish, item, 0, 1, 1, handle);
    } else {
        if (this_vector->_M_finish) {
            memcpy((void *)this_vector->_M_finish, item, 12);
        }
        this_vector->_M_finish += 12;
    }
}