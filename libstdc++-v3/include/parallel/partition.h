// -*- C++ -*-

// Copyright (C) 2007 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the terms
// of the GNU General Public License as published by the Free Software
// Foundation; either version 2, or (at your option) any later
// version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this library; see the file COPYING.  If not, write to
// the Free Software Foundation, 59 Temple Place - Suite 330, Boston,
// MA 02111-1307, USA.

// As a special exception, you may use this file as part of a free
// software library without restriction.  Specifically, if other files
// instantiate templates or use macros or inline functions from this
// file, or you compile this file and link it with other files to
// produce an executable, this file does not by itself cause the
// resulting executable to be covered by the GNU General Public
// License.  This exception does not however invalidate any other
// reasons why the executable file might be covered by the GNU General
// Public License.

/** @file parallel/partition.h
 *  @brief Parallel implementation of std::partition(),
 *  std::nth_element(), and std::partial_sort().
 *  This file is a GNU parallel extension to the Standard C++ Library.
 */

// Written by Johannes Singler and Felix Putze.

#ifndef _GLIBCXX_PARALLEL_PARTITION_H
#define _GLIBCXX_PARALLEL_PARTITION_H 1

#include <parallel/basic_iterator.h>
#include <parallel/sort.h>
#include <bits/stl_algo.h>
#include <parallel/parallel.h>

/** @brief Decide whether to declare certain variable volatile in this file. */
#define _GLIBCXX_VOLATILE volatile

namespace __gnu_parallel
{
  /** @brief Parallel implementation of std::partition.
   *  @param begin Begin iterator of input sequence to split.
   *  @param end End iterator of input sequence to split.
   *  @param pred Partition predicate, possibly including some kind of pivot.
   *  @param max_num_threads Maximum number of threads to use for this task.
   *  @return Number of elements not fulfilling the predicate. */
  template<typename RandomAccessIterator, typename Predicate>
  inline typename std::iterator_traits<RandomAccessIterator>::difference_type
  parallel_partition(RandomAccessIterator begin, RandomAccessIterator end,
		     Predicate pred, thread_index_t max_num_threads)
  {
    typedef std::iterator_traits<RandomAccessIterator> traits_type;
    typedef typename traits_type::value_type value_type;
    typedef typename traits_type::difference_type difference_type;

    difference_type n = end - begin;

    _GLIBCXX_CALL(n)

    // Shared.
    _GLIBCXX_VOLATILE difference_type left = 0, right = n - 1;
    _GLIBCXX_VOLATILE difference_type leftover_left, leftover_right, leftnew, rightnew;
    bool* reserved_left, * reserved_right;

    reserved_left = new bool[max_num_threads];
    reserved_right = new bool[max_num_threads];

    difference_type chunk_size;
    if (Settings::partition_chunk_share > 0.0)
      chunk_size = std::max((difference_type)Settings::partition_chunk_size, (difference_type)((double)n * Settings::partition_chunk_share / (double)max_num_threads));
    else
      chunk_size = Settings::partition_chunk_size;

    // At least good for two processors.
    while (right - left + 1 >= 2 * max_num_threads * chunk_size)
      {
	difference_type num_chunks = (right - left + 1) / chunk_size;
	thread_index_t num_threads = (int)std::min((difference_type)max_num_threads, num_chunks / 2);

	for (int r = 0; r < num_threads; r++)
	  {
	    reserved_left[r] = false;
	    reserved_right[r] = false;
	  }
	leftover_left = 0;
	leftover_right = 0;

#pragma omp parallel num_threads(num_threads)
	{
	  // Private.
	  difference_type thread_left, thread_left_border, thread_right, thread_right_border;
	  thread_left = left + 1;

	  // Just to satisfy the condition below.
	  thread_left_border = thread_left - 1;
	  thread_right = n - 1;
	  thread_right_border = thread_right + 1;

	  bool iam_finished = false;
	  while (!iam_finished)
	    {
	      if (thread_left > thread_left_border)
#pragma omp critical
		{
		  if (left + (chunk_size - 1) > right)
		    iam_finished = true;
		  else
		    {
		      thread_left = left;
		      thread_left_border = left + (chunk_size - 1);
		      left += chunk_size;
		    }
		}

	      if (thread_right < thread_right_border)
#pragma omp critical
		{
		  if (left > right - (chunk_size - 1))
		    iam_finished = true;
		  else
		    {
		      thread_right = right;
		      thread_right_border = right - (chunk_size - 1);
		      right -= chunk_size;
		    }
		}

	      if (iam_finished)
		break;

	      // Swap as usual.
	      while (thread_left < thread_right)
		{
		  while (pred(begin[thread_left]) && thread_left <= thread_left_border)
		    thread_left++;
		  while (!pred(begin[thread_right]) && thread_right >= thread_right_border)
		    thread_right--;

		  if (thread_left > thread_left_border || thread_right < thread_right_border)
		    // Fetch new chunk(s).
		    break;

		  std::swap(begin[thread_left], begin[thread_right]);
		  thread_left++;
		  thread_right--;
		}
	    }

	  // Now swap the leftover chunks to the right places.
	  if (thread_left <= thread_left_border)
#pragma omp atomic
	    leftover_left++;
	  if (thread_right >= thread_right_border)
#pragma omp atomic
	    leftover_right++;

#pragma omp barrier

#pragma omp single
	  {
	    leftnew = left - leftover_left * chunk_size;
	    rightnew = right + leftover_right * chunk_size;
	  }

#pragma omp barrier

	  // <=> thread_left_border + (chunk_size - 1) >= leftnew
	  if (thread_left <= thread_left_border
	      && thread_left_border >= leftnew)
	    {
	      // Chunk already in place, reserve spot.
	      reserved_left[(left - (thread_left_border + 1)) / chunk_size] = true;
	    }

	  // <=> thread_right_border - (chunk_size - 1) <= rightnew
	  if (thread_right >= thread_right_border
	      && thread_right_border <= rightnew)
	    {
	      // Chunk already in place, reserve spot.
	      reserved_right[((thread_right_border - 1) - right) / chunk_size] = true;
	    }

#pragma omp barrier

	  if (thread_left <= thread_left_border && thread_left_border < leftnew)
	    {
	      // Find spot and swap.
	      difference_type swapstart = -1;
#pragma omp critical
	      {
		for (int r = 0; r < leftover_left; r++)
		  if (!reserved_left[r])
		    {
		      reserved_left[r] = true;
		      swapstart = left - (r + 1) * chunk_size;
		      break;
		    }
	      }

#if _GLIBCXX_ASSERTIONS
	      _GLIBCXX_PARALLEL_ASSERT(swapstart != -1);
#endif

	      std::swap_ranges(begin + thread_left_border - (chunk_size - 1), begin + thread_left_border + 1, begin + swapstart);
	    }

	  if (thread_right >= thread_right_border
	      && thread_right_border > rightnew)
	    {
	      // Find spot and swap
	      difference_type swapstart = -1;
#pragma omp critical
	      {
		for (int r = 0; r < leftover_right; r++)
		  if (!reserved_right[r])
		    {
		      reserved_right[r] = true;
		      swapstart = right + r * chunk_size + 1;
		      break;
		    }
	      }

#if _GLIBCXX_ASSERTIONS
	      _GLIBCXX_PARALLEL_ASSERT(swapstart != -1);
#endif

	      std::swap_ranges(begin + thread_right_border, begin + thread_right_border + chunk_size, begin + swapstart);
	    }
#if _GLIBCXX_ASSERTIONS
#pragma omp barrier

#pragma omp single
	  {
	    for (int r = 0; r < leftover_left; r++)
	      _GLIBCXX_PARALLEL_ASSERT(reserved_left[r]);
	    for (int r = 0; r < leftover_right; r++)
	      _GLIBCXX_PARALLEL_ASSERT(reserved_right[r]);
	  }

#pragma omp barrier
#endif

#pragma omp barrier
	  left = leftnew;
	  right = rightnew;
	}
      }	// end "recursion"

    difference_type final_left = left, final_right = right;

    while (final_left < final_right)
      {
	// Go right until key is geq than pivot.
	while (pred(begin[final_left]) && final_left < final_right)
	  final_left++;

	// Go left until key is less than pivot.
	while (!pred(begin[final_right]) && final_left < final_right)
	  final_right--;

	if (final_left == final_right)
	  break;
	std::swap(begin[final_left], begin[final_right]);
	final_left++;
	final_right--;
      }

    // All elements on the left side are < piv, all elements on the
    // right are >= piv
    delete[] reserved_left;
    delete[] reserved_right;

    // Element "between" final_left and final_right might not have
    // been regarded yet
    if (final_left < n && !pred(begin[final_left]))
      // Really swapped.
      return final_left;
    else
      return final_left + 1;
  }

  /** 
   *  @brief Parallel implementation of std::nth_element().
   *  @param begin Begin iterator of input sequence.
   *  @param nth Iterator of element that must be in position afterwards.
   *  @param end End iterator of input sequence.
   *  @param comp Comparator. 
   */
  template<typename RandomAccessIterator, typename Comparator>
  void 
  parallel_nth_element(RandomAccessIterator begin, RandomAccessIterator nth, RandomAccessIterator end, Comparator comp)
  {
    typedef std::iterator_traits<RandomAccessIterator> traits_type;
    typedef typename traits_type::value_type value_type;
    typedef typename traits_type::difference_type difference_type;

    _GLIBCXX_CALL(end - begin)

    RandomAccessIterator split;
    value_type pivot;
    random_number rng;

    difference_type minimum_length = std::max<difference_type>(2, Settings::partition_minimal_n);

    // Break if input range to small.
    while (static_cast<sequence_index_t>(end - begin) >= minimum_length)
      {
	difference_type n = end - begin;

	RandomAccessIterator pivot_pos = begin +  rng(n);

	// Swap pivot_pos value to end.
	if (pivot_pos != (end - 1))
	  std::swap(*pivot_pos, *(end - 1));
	pivot_pos = end - 1;

	// XXX Comparator must have first_value_type, second_value_type, result_type
	// Comparator == __gnu_parallel::lexicographic<S, int, __gnu_parallel::less<S, S> > 
	// pivot_pos == std::pair<S, int>*
	// XXX binder2nd only for RandomAccessIterators??
	__gnu_parallel::binder2nd<Comparator, value_type, value_type, bool> pred(comp, *pivot_pos);

	// Divide, leave pivot unchanged in last place.
	RandomAccessIterator split_pos1, split_pos2;
	split_pos1 = begin + parallel_partition(begin, end - 1, pred, get_max_threads());

	// Left side: < pivot_pos; right side: >= pivot_pos

	// Swap pivot back to middle.
	if (split_pos1 != pivot_pos)
	  std::swap(*split_pos1, *pivot_pos);
	pivot_pos = split_pos1;

	// In case all elements are equal, split_pos1 == 0
	if ((split_pos1 + 1 - begin) < (n >> 7) || (end - split_pos1) < (n >> 7))
	  {
	    // Very unequal split, one part smaller than one 128th
	    // elements not stricly larger than the pivot.
	    __gnu_parallel::unary_negate<__gnu_parallel::binder1st<Comparator, value_type, value_type, bool>, value_type> pred(__gnu_parallel::binder1st<Comparator, value_type, value_type, bool>(comp, *pivot_pos));

	    // Find other end of pivot-equal range.
	    split_pos2 = __gnu_sequential::partition(split_pos1 + 1, end, pred);
	  }
	else
	  // Only skip the pivot.
	  split_pos2 = split_pos1 + 1;

	// Compare iterators.
	if (split_pos2 <= nth)
	  begin = split_pos2;
	else if (nth < split_pos1)
	  end = split_pos1;
	else
	  break;
      }

    // Only at most Settings::partition_minimal_n elements left.
    __gnu_sequential::sort(begin, end, comp);
  }

  /** @brief Parallel implementation of std::partial_sort().
   *  @param begin Begin iterator of input sequence.
   *  @param middle Sort until this position.
   *  @param end End iterator of input sequence.
   *  @param comp Comparator. */
  template<typename RandomAccessIterator, typename Comparator>
  void
  parallel_partial_sort(RandomAccessIterator begin, RandomAccessIterator middle, RandomAccessIterator end, Comparator comp)
  {
    parallel_nth_element(begin, middle, end, comp);
    std::sort(begin, middle, comp);
  }

}	//namespace __gnu_parallel

#undef _GLIBCXX_VOLATILE

#endif
