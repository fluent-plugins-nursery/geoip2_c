#ifndef __RB_COMPAT_H__
#define __RB_COMPAT_H__

#include <ruby.h>

#ifndef HAVE_RB_SYM2STR
#  define SYM2STR(name) (rb_id2str(SYM2ID(name)))
#else
#  define SYM2STR(name) (rb_sym2str(name))
#endif

#endif
