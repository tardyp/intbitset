// $Id$

// This file is part of CDS Invenio.
// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 CERN.
//
// CDS Invenio is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// CDS Invenio is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CDS Invenio; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.


#include <stdlib.h>
#include <string.h>

#include "intbitset.h"

const int wordbytesize = sizeof(word_t);
const int wordbitsize = sizeof(word_t) * 8;


IntBitSet *intBitSetCreate(register const int size, const bool_t universe) {
    register word_t *base;
    const register word_t *end;
    IntBitSet *ret = malloc(sizeof(IntBitSet));
    // At least one word -> the one who represent the universe
    ret->allocated = (size / wordbitsize + 1);
    ret->size = 0; // universe
    ret->universe = universe ? (word_t) ~0 : 0;
    if (universe) {
        base = ret->bitset = malloc(ret->allocated * wordbytesize);
        end = base + ret->allocated;
        for (; base < end; ++base)
            *base = (word_t) ~0;
        ret->tot = -1;
    } else {
        ret->bitset = calloc(ret->allocated, wordbytesize);
        ret->tot = 0;
    }
    return ret;
}

IntBitSet *intBitSetResetFromBuffer(IntBitSet *const bitset, const void *const buf, const int bufsize) {
    bitset->allocated = bufsize/wordbytesize;
    bitset->bitset = realloc(bitset->bitset, bufsize);
    bitset->tot = -1;
    bitset->size = -1;
    memcpy(bitset->bitset, buf, bufsize);
    bitset->universe = *(bitset->bitset + bitset->allocated - 1);
    return bitset;
}

IntBitSet *intBitSetReset(IntBitSet *const bitset) {
    register word_t *base = bitset->bitset;
    const register word_t *end = bitset->bitset+bitset->allocated;
    for (; base<end; ++base)
        *base = bitset->universe;
    bitset->tot = bitset->universe ? -1 : 0;
    bitset->size = 0;
    return bitset;
}


IntBitSet *intBitSetCreateFromBuffer(const void *const buf, const int bufsize) {
    IntBitSet *ret = malloc(sizeof(IntBitSet));
    ret->allocated = bufsize/wordbytesize;
    ret->bitset = malloc(bufsize);
    ret->size = -1;
    ret->tot = -1;
    memcpy(ret->bitset, buf, bufsize);
    ret->universe = ret->bitset[ret->allocated - 1];
    return ret;
}


void intBitSetDestroy(IntBitSet *const bitset) {
    free(bitset->bitset);
    free(bitset);
}

IntBitSet *intBitSetClone(const IntBitSet * const bitset) {
    IntBitSet *ret = malloc(sizeof(IntBitSet));
    ret->size = bitset->size;
    ret->tot = bitset->tot;
    ret->universe = bitset->universe;
    ret->allocated = bitset->allocated;
    ret->bitset = malloc(bitset->allocated * wordbytesize);
    memcpy(ret->bitset, bitset->bitset, bitset->allocated * wordbytesize);
    return ret;
}

int intBitSetGetSize(IntBitSet * const bitset) {
    register word_t *base;
    register word_t *end;
    if (bitset->size >= 0)
        return bitset->size;
    base = bitset->bitset;
    end = bitset->bitset + bitset->allocated - 2;
    for (; base < end && *end == bitset->universe; --end);
    bitset->size = ((int) (end - base) + 1);
    return bitset->size;
}

int intBitSetGetTot(IntBitSet *const bitset) {
    register word_t* base;
    register int i;
    register int tot;
    register word_t *end;
    if (bitset->universe)
        return -1;
    if (bitset->tot < 0) {
        end = bitset->bitset + bitset->allocated;
        tot = 0;
        for (base = bitset->bitset; base < end; ++base)
            if (*base)
                for (i=0; i<wordbitsize; ++i)
                    if ((*base & ((word_t) 1 << i)) != 0) {
                        ++tot;
                    }
        bitset->tot = tot;
    }
    return bitset->tot;
}

int intBitSetGetAllocated(const IntBitSet * const bitset) {
    return bitset->allocated;
}

void intBitSetResize(IntBitSet *const bitset, register const int allocated) {
    register word_t *base;
    register word_t *end;
    if (allocated > bitset->allocated)  {
        bitset->bitset = realloc(bitset->bitset, allocated * wordbytesize);
        base = bitset->bitset + bitset->allocated;
        end = bitset->bitset + allocated;
        for (; base<end; ++base)
            *(base) = bitset->universe;
        bitset->allocated = allocated;
    }
}

bool_t intBitSetIsInElem(const IntBitSet * const bitset, register const int elem) {
    return ((elem < bitset->allocated * wordbitsize) ?
            (bitset->bitset[elem / wordbitsize] & ((word_t) 1 << ((word_t)elem % (word_t)wordbitsize))) != 0 : bitset->universe != 0);
}

void intBitSetAddElem(IntBitSet *const bitset, register const int elem) {
    if (elem >= (bitset->allocated - 1) * wordbitsize)
        if (bitset->universe)
            return;
        else
            intBitSetResize(bitset, (elem + elem/10)/wordbitsize+2);
    bitset->bitset[elem / wordbitsize] |= ((word_t) 1 << (elem % wordbitsize));
    bitset->tot = -1;
    bitset->size = -1;
}

void intBitSetDelElem(IntBitSet *const bitset, register const int elem) {
    if (elem >= (bitset->allocated - 1) * wordbitsize)
        if (!bitset->universe)
            return;
        else
            intBitSetResize(bitset, (elem + elem/10)/wordbitsize+2);
    bitset->bitset[elem / wordbitsize] &= (word_t) ~((word_t) 1 << (elem % wordbitsize));
    bitset->tot = -1;
    bitset->size = -1;
}

bool_t intBitSetEmpty(const IntBitSet *const bitset) {
    register word_t *end;
    register word_t *base;
    if (bitset->universe) return 0;
    if (bitset->tot == 0) return 1;
    end = bitset->bitset + bitset->allocated;
    for (base = bitset->bitset; base < end; ++base)
        if (*base) return 0;
    return 1;
}

int intBitSetAdapt(IntBitSet *const x, IntBitSet *const y) {
    register int sizex = intBitSetGetSize(x);
    register int sizey = intBitSetGetSize(y);
    register int sizemax = (sizex > sizey) ? sizex : sizey;
    if (sizemax > x->allocated-1)
        intBitSetResize(x, sizemax+1);
    if (sizemax > y->allocated-1)
        intBitSetResize(y, sizemax+1);
    return sizemax+1;
}

IntBitSet *intBitSetUnion(IntBitSet *const x, IntBitSet *const y) {
    register word_t *xbase;
    register word_t *xend;
    register word_t *ybase;
    register word_t *retbase;
    register IntBitSet * ret = malloc(sizeof (IntBitSet));
    ret->allocated = intBitSetAdapt(x, y);
    xbase = x->bitset;
    xend = x->bitset+ret->allocated;
    ybase = y->bitset;
    retbase = ret->bitset = malloc(wordbytesize * ret->allocated);
    ret->size = -1;
    ret->tot = -1;
    for (; xbase < xend; ++xbase, ++ybase, ++retbase)
        *(retbase) = *(xbase) | *(ybase);
    ret->universe = x->universe | y->universe;
    return ret;
}

IntBitSet *intBitSetXor(IntBitSet *const x, IntBitSet *const y) {
    register word_t *xbase;
    register word_t *xend;
    register word_t *ybase;
    register word_t *retbase;
    register IntBitSet * ret = malloc(sizeof (IntBitSet));
    ret->allocated = intBitSetAdapt(x, y);
    xbase = x->bitset;
    xend = x->bitset+ret->allocated;
    ybase = y->bitset;
    retbase = ret->bitset = malloc(wordbytesize * ret->allocated);
    ret->size = -1;
    ret->tot = -1;
    for (; xbase < xend; ++xbase, ++ybase, ++retbase)
        *(retbase) = *(xbase) ^ *(ybase);
    ret->universe = x->universe ^ y->universe;
    return ret;
}

IntBitSet *intBitSetIntersection(IntBitSet *const x, IntBitSet *const y) {
    register word_t *xbase;
    register word_t *xend;
    register word_t *ybase;
    register word_t *retbase;
    register IntBitSet * ret = malloc(sizeof (IntBitSet));
    ret->allocated = intBitSetAdapt(x, y);
    xbase = x->bitset;
    xend = x->bitset+ret->allocated;
    ybase = y->bitset;
    retbase = ret->bitset = malloc(wordbytesize * ret->allocated);
    ret->size = -1;
    ret->tot = -1;
    for (; xbase < xend; ++xbase, ++ybase, ++retbase)
        *(retbase) = *(xbase) & *(ybase);
    ret->universe = x->universe & y->universe;
    return ret;
}

IntBitSet *intBitSetSub(IntBitSet *const x, IntBitSet *const y) {
    register word_t *xbase;
    register word_t *xend;
    register word_t *ybase;
    register word_t *retbase;
    register IntBitSet * ret = malloc(sizeof (IntBitSet));
    ret->allocated = intBitSetAdapt(x, y);
    xbase = x->bitset;
    xend = x->bitset+ret->allocated;
    ybase = y->bitset;
    retbase = ret->bitset = malloc(wordbytesize * ret->allocated);
    ret->size = -1;
    ret->tot = -1;
    for (; xbase < xend; ++xbase, ++ybase, ++retbase)
        *(retbase) = *(xbase) & ~*(ybase);
    ret->universe = x->universe & ~y->universe;
    return ret;
}

IntBitSet *intBitSetIUnion(IntBitSet *const dst, IntBitSet *const src) {
    register word_t *dstbase;
    register word_t *srcbase;
    register word_t *srcend;
    register int allocated = intBitSetAdapt(dst, src);
    dstbase = dst->bitset;
    srcbase = src->bitset;
    srcend = src->bitset + allocated;
    for (; srcbase < srcend; ++dstbase, ++srcbase)
        *dstbase |= *srcbase;
    dst->size = -1;
    dst->tot = -1;
    dst->universe |= src->universe;
    return dst;
}

IntBitSet *intBitSetIXor(IntBitSet *const dst, IntBitSet *const src) {
    register word_t *dstbase;
    register word_t *srcbase;
    register word_t *srcend;
    register int allocated = intBitSetAdapt(dst, src);
    dstbase = dst->bitset;
    srcbase = src->bitset;
    srcend = src->bitset + allocated;
    for (; srcbase < srcend; ++dstbase, ++srcbase)
        *dstbase ^= *srcbase;
    dst->size = -1;
    dst->tot = -1;
    dst->universe ^= src->universe;
    return dst;
}

IntBitSet *intBitSetIIntersection(IntBitSet *const dst, IntBitSet *const src) {
    register word_t *dstbase;
    register word_t *srcbase;
    register word_t *srcend;
    register int allocated = intBitSetAdapt(dst, src);
    dstbase = dst->bitset;
    srcbase = src->bitset;
    srcend = src->bitset + allocated;
    for (; srcbase < srcend; ++dstbase, ++srcbase)
        *dstbase &= *srcbase;
    dst->size = -1;
    dst->tot = -1;
    dst->universe &= src->universe;
    return dst;
}

IntBitSet *intBitSetISub(IntBitSet *const dst, IntBitSet *const src) {
    register word_t *dstbase;
    register word_t *srcbase;
    register word_t *srcend;
    register int allocated = intBitSetAdapt(dst, src);
    dstbase = dst->bitset;
    srcbase = src->bitset;
    srcend = src->bitset + allocated;
    for (; srcbase < srcend; ++dstbase, ++srcbase)
        *dstbase &= ~*srcbase;
    dst->size = -1;
    dst->tot = -1;
    dst->universe &= ~src->universe;
    return dst;
}

int intBitSetGetNext(const IntBitSet *const x, register int last) {
    register word_t* base = x->bitset + ++last / wordbitsize;
    register int i = last % wordbitsize;
    const register word_t *end = x->bitset + x->allocated;
    while(base < end) {
        if (*base)
            for (; i<wordbitsize; ++i)
                if ((*base & ((word_t) 1 << (word_t) i)) != 0)
                    return (int) i + (int) (base - x->bitset) * wordbitsize;
        i = 0;
        ++base;
    }
    return x->universe ? last : -2;
}

unsigned char intBitSetCmp(IntBitSet *const x, IntBitSet *const y) {
    register word_t *xbase;
    register word_t *xend;
    register word_t *ybase;
    register unsigned char ret = 0;
    register int allocated = intBitSetAdapt(x, y);
    xbase = x->bitset;
    xend = x->bitset+allocated;
    ybase = y->bitset;
    for (; ret != 3 && xbase<xend; ++xbase, ++ybase)
        ret |= (*ybase != (*xbase | *ybase)) * 2 + (*xbase != (*xbase | *ybase));
    ret |= (y->universe != (x->universe | y->universe)) * 2 + (x->universe != (x->universe | y->universe));
    return ret;
}
