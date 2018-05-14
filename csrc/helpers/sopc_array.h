/*
 *  Copyright (C) 2018 Systerel and others.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file sopc_array.h
 * \brief A generic array implementation.
 */

#ifndef SOPC_ARRAY_H_
#define SOPC_ARRAY_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct _SOPC_Array SOPC_Array;

/**
 * \brief Type of functions used to free array values.
 *
 * The parameter passed to the function is a pointer to the item. That means
 * that if your array holds pointers, the free function will be passed a pointer
 * to the pointer held in the array.
 */
typedef void (*SOPC_Array_Free_Func)(void* data);

/**
 * \brief Type of functions used to compare items when sorting an array.
 *
 * The function should return a value less than 0 if a is less than b, 0 if a
 * and b have the same value, and greater than 0 if a is greater than b.
 *
 * The parameters passed to the function are pointers to the items. That means
 * that if your array holds pointers, the compare function will be passed
 * pointers to the pointers held in the array.
 */
typedef int (*SOPC_Array_Compare_Func)(const void* a, const void* b);

/**
 * \brief Creates a new array with a given capacity.
 *
 * \param element_size      The size of one element of the array.
 * \param initial_capacity  The initial capacity to allocate (can be 0).
 * \param free_func         A function to call on each array element when the
 *                          array is deleted. Can be \c NULL.
 * \return The created array in case of success, or \c NULL on memory allocation
 *         failure.
 *
 * The actual allocated size might be greater than the requested capacity.
 */
SOPC_Array* SOPC_Array_Create(size_t element_size, size_t initial_capacity, SOPC_Array_Free_Func free_func);

/**
 * \brief Makes a copy of an array.
 *
 * \param array  The array.
 *
 * \return The copied array in case of success, or \c NULL on memory allocation
 *         failure.
 */
SOPC_Array* SOPC_Array_Copy(const SOPC_Array* array);

/**
 * \brief Deletes an array.
 * \param array  The array.
 *
 * The free function passed in the constructor or via
 * \ref SOPC_Array_Set_Free_Func (if any) will be called for each element of the
 * array before the array is freed.
 */
void SOPC_Array_Delete(SOPC_Array* array);

/**
 * \brief Appends one value to an array
 *
 * \param array  The array.
 * \param val    The value to append.
 *
 * \return \c TRUE on success, \c FALSE on memory allocation failure.
 */
#define SOPC_Array_Append(array, val) SOPC_Array_Append_Values((array), &(val), 1)

/**
 * \brief Appends several values contiguous in memory to an array.
 *
 * \param array       The array.
 * \param data        The memory location of the first value.
 * \param n_elements  The number of values present in memory.
 *
 * \return \c TRUE on success, \c FALSE on memory allocation failure.
 */
bool SOPC_Array_Append_Values(SOPC_Array* array, void* data, size_t n_elements);

/**
 * \brief Gets a value from an array by its index.
 *
 * \param array  The array.
 * \param ty     The type of the value stored in the array.
 * \param index  The index of the value in the array.
 *
 * \return The value.
 */
#define SOPC_Array_Get(array, ty, index) (*((ty*) SOPC_Array_Get_Ptr((array), (index))))

/**
 * \brief Gets a pointer to a value in an array by its index.
 *
 * \param array  The array.
 * \param index  The index of the value.
 *
 * \return A pointer to the value.
 */
void* SOPC_Array_Get_Ptr(const SOPC_Array* array, size_t index);

/**
 * \brief Gets the number of elements in an array.
 *
 * \param array  The array.
 *
 * \return The number of elements in the array.
 */
size_t SOPC_Array_Size(const SOPC_Array* array);

/**
 * \brief Sorts the elements in an array.
 *
 * \param array         The array.
 * \param compare_func  The function to use to compare to elements.
 *
 * The sort is not guaranteed to be stable.
 */
void SOPC_Array_Sort(SOPC_Array* array, SOPC_Array_Compare_Func compare_func);

/**
 * \brief Returns the function used to clear the elements of an array on deletion.
 *
 * \param array  The array.
 *
 * \return  The function used to clear the elements when the array is deleted,
 *          or \c NULL if no such function is defined.
 */
SOPC_Array_Free_Func SOPC_Array_Get_Free_Func(SOPC_Array* array);

/**
 * \brief Sets the function used to clear the array elements when it is deleted.
 *
 * \param array  The array.
 *
 * \param func  The function to use for clearing elements, or \c NULL if no such
 *              function should be called.
 */
void SOPC_Array_Set_Free_Func(SOPC_Array* array, SOPC_Array_Free_Func func);

#endif /* SOPC_ARRAY_H_ */