/*
 * JOOS is Copyright (C) 1997 Laurie Hendren & Michael I. Schwartzbach
 *
 * Reproduction of all or part of this software is permitted for
 * educational or research use on condition that this copyright notice is
 * included in any copy. This software comes with no warranty of any
 * kind. In no event will the authors be liable for any damages resulting from
 * use of this software.
 *
 * email: hendren@cs.mcgill.ca, mis@brics.dk
 */

/* iload x        iload x        iload x
 * ldc 0          ldc 1          ldc 2
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */

int simplify_multiplication_right(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) && 
      is_ldc_int(next(*c),&k) && 
      is_imul(next(next(*c)))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * astore x
 * pop
 * -------->
 * astore x
 */
int simplify_astore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_astore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEastore(x,NULL));
  }
  return 0;
}

/* iload x
 * ldc k   (0<=k<=127)
 * iadd
 * istore x
 * --------->
 * iinc x k
 */ 
int positive_increment(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_iadd(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      x==y && 0<=k && k<=127) {
     return replace(c,4,makeCODEiinc(x,k,NULL));
  }
  return 0;
}

/* goto L1
 * ...
 * L1:
 * goto L2
 * ...
 * L2:
 * --------->
 * goto L2
 * ...
 * L1:    (reference count reduced by 1)
 * goto L2
 * ...
 * L2:    (reference count increased by 1)  
 */
int simplify_goto_goto(CODE **c)
{ int l1,l2;
  if (is_goto(*c,&l1) && is_goto(next(destination(l1)),&l2) && l1>l2) {
     droplabel(l1);
     copylabel(l2);
     return replace(c,1,makeCODEgoto(l2,NULL));
  }
  return 0;
}

/*
 * cmp true1 (not only positive comparisons, any comparisons)
 * iconst_0
 * goto stop_2
 * true1:
 * iconst_1
 * stop_2:
 * ifeq stop_0
 * ...
 * stop_0:
 * ---------->
 * !cmp stop_0 (the opposite comparison of cmp on the first line)
 * ...
 * stop_0:
 */
int simplify_conditional(CODE **c)
{
  int l1,l2, l3, l4;
  if ( is_if_acmpeq(*c, &l1) || is_if_acmpne(*c,&l1) || is_if_icmpeq(*c, &l1) || 
      is_if_icmpge(*c, &l1) || is_if_icmpgt(*c, &l1) || is_if_icmple(*c, &l1) ||
      is_if_icmplt(*c, &l1) || is_if_icmpne(*c, &l1) || is_ifeq(*c, &l1) ||
      is_ifne(*c, &l1) || is_ifnonnull(*c, &l1) || is_ifnull(*c, &l1))
      {
        if(is_ldc_int(next(*c), &l2) && l2 == 0 && is_goto(next(next(*c)), &l2) && is_ifeq(next(destination(l2)), &l3))
        {
          if(is_ldc_int(next(destination(l1)), &l4) && l4 == 1 && is_label(next(next(destination(l1))), &l4) && l4 == l2)
          {
            droplabel(l1);
            droplabel(l2);
            if(is_if_acmpeq(*c, &l1))
            {
              return replace(c, 7, makeCODEif_acmpne(l3, NULL));
            }
            else if(is_if_acmpne(*c,&l1))
            {
              return replace(c, 7, makeCODEif_acmpeq(l3, NULL));
            }
            else if(is_if_icmpeq(*c, &l1))
            {
              return replace(c, 7, makeCODEif_icmpne(l3, NULL));
            }
            else if(is_if_icmpge(*c, &l1))
            {
              return replace(c, 7, makeCODEif_icmplt(l3, NULL));
            }
            else if(is_if_icmpgt(*c, &l1))
            {
              return replace(c, 7, makeCODEif_icmple(l3, NULL));
            }
            else if(is_if_icmple(*c, &l1))
            {
              return replace(c, 7, makeCODEif_icmpgt(l3, NULL));
            }
            else if(is_if_icmplt(*c, &l1))
            {
              return replace(c, 7, makeCODEif_icmpge(l3, NULL));
            }
            else if(is_if_icmpne(*c, &l1))
            {
              return replace(c, 7, makeCODEif_icmpeq(l3, NULL));
            }
            else if(is_ifeq(*c, &l1))
            {
              return replace(c, 7, makeCODEifne(l3, NULL));
            }
            else if(is_ifne(*c, &l1))
            {
              return replace(c, 7, makeCODEifeq(l3, NULL));
            }
            else if(is_ifnonnull(*c, &l1))
            {
              return replace(c, 7, makeCODEifnull(l3, NULL));
            }
            else{
              return replace(c, 7, makeCODEifnonnull(l3, NULL));
            }
          }
         
        }
        
      }
  return 0;
}

/* 
	Label: (dead label)
	--------->
	(nothing)
*/
int remove_dead_label(CODE **c) {
	int label;
	if (is_label(*c, &label) && deadlabel(label)){
		return kill_line(c);
	}
	return 0;
}

/*
  goto Label1
  ...
Label1:
Label2:
----------------->
  goto Label2
  ...
Label1: (reference count decrements, eventually dies)
Label2: (reference count increments)
*/
int collapse_labels(CODE **c)
{
  int label1, label2;
  
  if (
	is_goto(*c, &label1)
	&& is_label(next(destination(label1)), &label2)
	&& label1>label2
	)
	{
		droplabel(label1);
		copylabel(label2);
		return replace(c, 1, makeCODEgoto(label2, NULL));
	}
  return 0;
}


/*
	goto Label
	... (one line of instruction that is not a label)
------------------->
	goto Label
*/
int remove_unreachable(CODE **c)
{
	int label, anotherLabel;
	
	if (
		is_goto(*c, &label)
		&& !is_label(next(*c), &anotherLabel)
	)
		return replace(c, 2, makeCODEgoto(label, NULL));
	return 0;
}


/*
	goto Label
Label:
--------------->
Label: (reference count decrements)
*/
int fuse_goto(CODE **c)
{
	int label1, label2;
	if (is_goto(*c, &label1) && is_label(next(*c), &label2) && label1 == label2)
	{
		droplabel(label1);
		return kill_line(c);
	}
	return 0;
}

void init_patterns(void) {
	ADD_PATTERN(simplify_multiplication_right);
	ADD_PATTERN(simplify_astore);
	ADD_PATTERN(positive_increment);
	ADD_PATTERN(simplify_goto_goto);
	ADD_PATTERN(simplify_conditional);
	ADD_PATTERN(remove_dead_label);
	/*ADD_PATTERN(collapse_labels);*/
	ADD_PATTERN(remove_unreachable);
	ADD_PATTERN(fuse_goto);
}
