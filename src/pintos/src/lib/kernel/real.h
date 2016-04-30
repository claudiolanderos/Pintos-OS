#ifndef __LIB_KERNEL_REAL_H
#define __LIB_KERNEL_REAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct real
  {
    int bits;
  };

struct real real_of_int (int x);
int int_of_real (struct real v);
struct real real_add (struct real lhs, struct real rhs);
struct real real_sub (struct real lhs, struct real rhs);
struct real real_mult (struct real lhs, struct real rhs);
struct real real_div (struct real lhs, struct real rhs);
struct real real_minus (struct real v);
bool real_less_func (struct real lhs, struct real rhs);

#endif /* lib/kernel/real.h */
