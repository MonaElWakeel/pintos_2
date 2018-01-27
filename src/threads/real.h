#ifndef THREADS_REAL_H
#define THREADS_REAL_H

struct real{
	int value;
};

/* convert from integer to real number*/
struct real int_to_real(int x);

/*convert from real number to integer*/
int real_to_int(struct real x);

/*convert from real to the nearest integer*/
int real_to_int_rounding(struct real x);

/*add two real numbers*/
struct real add (struct real x, struct real y);

/*add integer with real number*/
struct real add_mix(struct real x , int n);

/*subtract two real numbers*/
struct real subtract (struct real x, struct real y);

/*subtract integer with real number*/
struct real subtract_mix(struct real x , int n);

/*multiply integer with real number*/
struct real multiply (struct real x, int n);

/*multiply two real number*/
struct real multiply_approx (struct real x, struct real y);

/*divide real number by integer*/
struct real divide (struct real  x, int n);

/*divide two real number*/
struct real divide_approx (struct real  x, struct real y);
#endif /* threads/real.h */
