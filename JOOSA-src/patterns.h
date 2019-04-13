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
 * ldc_int 0      ldc_int 1      ldc_int 2
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */

int simplify_multiplication_right(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) && 
      is_ldc_int(next(*c),&k)&& 
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

/*
	dup
	istore x
	pop
	-------->
	istore x
*/
int simplify_istore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_istore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEistore(x,NULL));
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

/*
	iload x
	ldc k (0<=k<=127)
	isub
	istore x
	----------->
	iinc x -k 
*/
int negative_increment(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_isub(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      x==y && 0<=k && k<=127) {
     return replace(c,4,makeCODEiinc(x,-k,NULL));
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
            replace(next(destination(l1)), 1, NULL);
            replace(next(destination(l2)), 1, NULL);
            droplabel(l1);
            droplabel(l2);
            if(is_if_acmpeq(*c, &l1))
            {
              return replace(c, 3, makeCODEif_acmpne(l3, NULL));
            }
            else if(is_if_acmpne(*c,&l1))
            {
              return replace(c, 3, makeCODEif_acmpeq(l3, NULL));
            }
            else if(is_if_icmpeq(*c, &l1))
            {
              return replace(c, 3, makeCODEif_icmpne(l3, NULL));
            }
            else if(is_if_icmpge(*c, &l1))
            {
              return replace(c, 3, makeCODEif_icmplt(l3, NULL));
            }
            else if(is_if_icmpgt(*c, &l1))
            {
              return replace(c, 3, makeCODEif_icmple(l3, NULL));
            }
            else if(is_if_icmple(*c, &l1))
            {
              return replace(c, 3, makeCODEif_icmpgt(l3, NULL));
            }
            else if(is_if_icmplt(*c, &l1))
            {
              return replace(c, 3, makeCODEif_icmpge(l3, NULL));
            }
            else if(is_if_icmpne(*c, &l1))
            {
              return replace(c, 3, makeCODEif_icmpeq(l3, NULL));
            }
            else if(is_ifeq(*c, &l1))
            {
              return replace(c, 3, makeCODEifne(l3, NULL));
            }
            else if(is_ifne(*c, &l1))
            {
              return replace(c, 3, makeCODEifeq(l3, NULL));
            }
            else if(is_ifnonnull(*c, &l1))
            {
              return replace(c, 3, makeCODEifnull(l3, NULL));
            }
            else{
              return replace(c, 3, makeCODEifnonnull(l3, NULL));
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
  jump Label1 (any jump)
  ...
Label1:
Label2:
----------------->
  jump Label2 (same jump)
  ...
Label1: (reference count decrements, eventually dies)
Label2: (reference count increments)
*/
int collapse_labels(CODE **c)
{
  int label1, label2;

  if (
	uses_label(*c, &label1)
	&& is_label(next(destination(label1)), &label2)
	&& label1>label2
	)
	{
		droplabel(label1);
		copylabel(label2);

    /* goto */
    if (is_goto(*c, &label1))
		  return replace(c, 1, makeCODEgoto(label2, NULL));
    /* ifeq */
    if (is_ifeq(*c, &label1))
      return replace(c, 1, makeCODEifeq(label2, NULL));
    /* ifne */
    if (is_ifne(*c, &label1))
      return replace(c, 1, makeCODEifne(label2, NULL));
    /* if_acmpeq */
    if (is_if_acmpeq(*c, &label1))
      return replace(c, 1, makeCODEif_acmpeq(label2, NULL));
    /* if_acmpne */
    if (is_if_acmpne(*c, &label1))
      return replace(c, 1, makeCODEif_acmpne(label2, NULL));
    /* if_ifnull */
    if (is_ifnull(*c, &label1))
      return replace(c, 1, makeCODEifnull(label2, NULL));
    /* if_ifnonnull */
    if (is_ifnonnull(*c, &label1))
      return replace(c, 1, makeCODEifnonnull(label2, NULL));
    /* if_icmpeq */
    if (is_if_icmpeq(*c, &label1))
      return replace(c, 1, makeCODEif_icmpeq(label2, NULL));
    /* if_icmpgt */
    if (is_if_icmpgt(*c, &label1))
      return replace(c, 1, makeCODEif_icmpgt(label2, NULL));
    /* if_icmplt */
    if (is_if_icmplt(*c, &label1))
      return replace(c, 1, makeCODEif_icmplt(label2, NULL));
    /* if_icmple */
    if (is_if_icmple(*c, &label1))
      return replace(c, 1, makeCODEif_icmple(label2, NULL));
    /* if_icmpge */
    if (is_if_icmpge(*c, &label1))
      return replace(c, 1, makeCODEif_icmpge(label2, NULL));
    /* if_icmpne */
    if (is_if_icmpne(*c, &label1))
      return replace(c, 1, makeCODEif_icmpne(label2, NULL));

    return 0;
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

/*
	iinc x 0
	----------->
	(nothing)
*/
int zero_increment(CODE **c)
{
	int x, amount;
	if(is_iinc(*c, &x, &amount) && amount == 0) {
		return kill_line(c);
	}
	return 0;
}

/*
	iinc x k 
	iinc x m 
	-------->
	iinc x (k+m) (-128 <= k+m <= 127)
*/
int merge_increment(CODE **c)
{
	int x1, x2, k, m; /* x1 == x2 is checked later */
	if (is_iinc(*c, &x1, &k) &&
		is_iinc(next(*c), &x2, &m) &&
		x1 == x2 &&
		-128 <= k + m &&
		k + m <= 127)
		return replace(c, 2, makeCODEiinc(x1, k+m, NULL));
	return 0;
}

/* iload z / iconst z
 * istore y
 * iload x
 * istore y
 * -------->
 * iload x
 * istore y
*/
int redundant_store(CODE **c){
  int x, y1, y2 z;
  if((is_iload(*c, &z) || is_ldc_int(*c, &z)) &&  
     is_istore(next(*c), &y1) &&
     is_iload(next(next(*c)), &x) &&
     is_istore(next(next(next(*c), &y2))) && y1 == y2){
        return replace(c, 4, makeCODEiload(x, makeCODEistore(y, NULL)));
     }
     return 0;
}

/*
 *  iconst a
 *  iconst b
 *  iadd / isub / imul / idiv
 *  -------->
 *  iconst a+b / iconst a-b / iconst a*b / iconst a/b
*/
int fuse_const_operations(CODE **c){
  int a, b;
  if(is_ldc_int(*c, &a) && is_ldc_int(next(*c), &b)){
    if (is_imul(next(next(*c)))){
      return replace(c, 3, makeCODE(ldc_int(a*b)));
    } else if(is_iadd(next(next(*c)))){
      return replace(c, 3, makeCODE(ldc_int(a+b)));
    } else if(is_isub(next(next(*c)))){
      return replace(c, 3, makeCODE(ldc_int(a-b)));
    } else if(is_idiv(next(next(*c)))){
      return replace(c, 3, makeCODE(ldc_int(a/b)));
    }
  }
  return 0; 
    
}



/*
	ireturn		return		areturn
	nop			nop			nop
	------>		------->	-------->
	ireturn		return		areturn
	
	
	nop
	nop
	------>
	nop
	
	
	nop
	Label:
	------>
	Label:
*/
int simplify_nop(CODE **c)
{
	int label;
	
	/* nop after a return */
	if (is_ireturn(*c) &&
		is_nop(next(*c)))
		return replace(c, 2, makeCODEireturn(NULL));
		
	else if (is_return(*c) &&
		is_nop(next(*c)))
		return replace(c, 2, makeCODEreturn(NULL));
		
	else if (is_areturn(*c) &&
		is_nop(next(*c)))
		return replace(c, 2, makeCODEareturn(NULL));
	
	/* repeated nop */
	else if (is_nop(*c) && is_nop(next(*c)))
		return replace(c, 2, makeCODEnop(NULL));
	
	
	/* nop before a label */
	if (is_nop(*c) && is_label(next(*c), &label))
		return replace(c, 2, makeCODElabel(label, NULL));
	
	return 0;
}



void init_patterns(void) {
	ADD_PATTERN(simplify_multiplication_right);
	ADD_PATTERN(simplify_astore);
	ADD_PATTERN(positive_increment);
	ADD_PATTERN(simplify_goto_goto);
	/*ADD_PATTERN(simplify_conditional);*/
	ADD_PATTERN(remove_dead_label);
	ADD_PATTERN(collapse_labels);
	ADD_PATTERN(remove_unreachable);
	ADD_PATTERN(fuse_goto);
	ADD_PATTERN(zero_increment);
	ADD_PATTERN(simplify_istore);
  ADD_PATTERN(redundant_store);
  ADD_PATTERN(fuse_const_operations);
	ADD_PATTERN(negative_increment);
	ADD_PATTERN(merge_increment);
	ADD_PATTERN(simplify_nop);
	ADD_PATTERN(redundant_store);
}
