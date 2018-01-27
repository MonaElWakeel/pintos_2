/* this file contains the implementation of fixed point numbers 
   and the operations that use them.
*/
#include <stdio.h>
#include <string.h>
#include "threads/real.h"
#define F (1 << 14)

// convert from integer to real number
struct real int_to_real(int x){
	struct real real_number;
	real_number.value = x*F;
    	return real_number;
}

// convert from real number to integer
int real_to_int(struct real x){
	int y = x.value/F;
     	return y;
}

// convert from real to the nearest integer
int real_to_int_rounding(struct real x){
        if(x.value>= 0){
		return(x.value+F/2)/F;
	}else{
		return(x.value-F/2)/F;
        }
}

//add two real numbers.
struct real add (struct real x, struct real y){
   struct real result_add;
   int result = x.value + y.value ;
   result_add.value = result;
   return result_add;

}

//add integer with real number
struct real add_mix(struct real x , int n){
   struct real result_add;
   struct real first;
   first = int_to_real(n);
   result_add = add(x,first);
   return result_add;
}

//subtract two real numbers
struct real subtract (struct real x, struct real y){
   struct real result_add;
   int result = x.value - y.value ;
   result_add.value = result;
   return result_add;
}

//subtract integer with real number
struct real subtract_mix(struct real x , int n){
   struct real result_add;
   struct real first;
   first = int_to_real(n);
   result_add = subtract(x,first);
   return result_add;
}

//multiply integer with real number
struct real multiply (struct real x, int n){
   struct real result_multiply;
   result_multiply.value = x.value*n;
   return result_multiply;
}

//multiply two real number
struct real multiply_approx (struct real x, struct real y){
   struct real result_multiply;
   result_multiply.value = ((int64_t) x.value) * y.value / F;
   return result_multiply;
}

//divide real number by integer
struct real divide (struct real  x, int n){
  struct real result_divide;
   result_divide.value = x.value/n;
   return result_divide;
}

//divide two real number
struct real divide_approx (struct real  x, struct real y){
  struct real result_divide;
   result_divide.value = ((int64_t) x.value) * F / y.value;
   return result_divide;
}
