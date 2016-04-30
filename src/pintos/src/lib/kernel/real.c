#include "real.h"

#define Q 14

static int
value_of_real (struct real v)
{
  return v.bits >> 1;
}

static int
sign_of_real (struct real v)
{
  return v.bits & 1;
}

static struct real
real_of_value_sign (int value, int sign)
{
  struct real result;
  result.bits = (value << 1) ^ sign;
  return result;
}


struct real
real_of_int (int x)
{
  int v = (x > 0) ? x : -x;
  int sign = (x > 0) ? 0 : 1;
  return real_of_value_sign (v << Q, sign);
}

int
int_of_real (struct real v)
{
  return sign_of_real (v) == 0 ? value_of_real (v) >> Q : -(value_of_real (v) >> Q);
}

struct real
real_add (struct real lhs, struct real rhs)
{
  if (sign_of_real (lhs) == sign_of_real (rhs))
  {
    return real_of_value_sign (value_of_real (lhs) + value_of_real (rhs),
                               sign_of_real (lhs));
  }
  else
  {
    return real_of_value_sign (value_of_real (lhs) - value_of_real (rhs),
                               sign_of_real (lhs));
  }
}

struct real
real_sub (struct real lhs, struct real rhs)
{
  return real_add (lhs, real_minus (rhs));
}

struct real
real_mult (struct real lhs, struct real rhs)
{
  return real_of_value_sign ((((long long) value_of_real (lhs)) *
                              ((long long) value_of_real (rhs))) >> Q,
                             sign_of_real (lhs) ^ sign_of_real (rhs));
}

struct real
real_div (struct real lhs, struct real rhs)
{
  return real_of_value_sign ((((long long) value_of_real (lhs)) << Q) /
                             ((long long) value_of_real (rhs)),
                             sign_of_real (lhs) ^ sign_of_real (rhs));
}

struct real
real_minus (struct real v)
{
    return real_of_value_sign (value_of_real (v),
                               sign_of_real (v) ^ 1);
}

bool
real_less_func (struct real lhs, struct real rhs)
{
  if (sign_of_real (lhs) != sign_of_real (rhs))
  {
    return sign_of_real (lhs) == 1;
  }
  else
  {
    return (value_of_real (lhs) < value_of_real (rhs)) ^ sign_of_real (lhs);
  }
}
