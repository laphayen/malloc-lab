/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"
/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "SWjungle-2",
    /* First member's full name */
    "gitddabong",
    /* First member's email address */
    "gitddabong@naver.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
/* 주소를 워드(4)로 정렬할지 더블 워드(8)로 정렬 할지 결정한다 */
#define ALIGNMENT 8
/* 받은 주소가 워드의 배수가 되도록 만든다 */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/* 
 * mm_init - initialize the malloc package.
 */
/* 기본적인 상수와 매크로*/
#define WSIZE   4   //워드의 크기
#define DSIZE   8   //더블워드의 크기
#define CHUNKSIZE (1<<12)   //힙 영역을 한 번 늘릴 때 마다 늘려 줄 크기
#define MAX(x, y) ((x)>(y)?(x):(y))
/* 크기와 할당 상태를 1워드로 묶는다 */
#define PACK(size, alloc) ((size)|(alloc))
/* 주소 p에 있는 값을 읽고 쓴다 */
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
/* 주소 p에서 블록의 크기와 할당상태를 읽어온다 */
#define GET_SIZE(p)     (GET(p)&~0x7)
#define GET_ALLOC(p)    (GET(p)&0x1)
/* 블록 포인터 bp를 받으면, 그 블록의 헤더와 풋터 주소를 반환한다 */
#define HDRP(bp) ((char *)(bp) - WSIZE) 
/* 포인터가 배열[0]을 가리키고 있을 것이기 때문에 1워드만큼 앞으로 가면 헤더가 나온다 */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 
/* 포인터를 헤더로 옮겨서 사이즈를 가져온 다음, 포인터부터 시작해서 사이즈만큼 뒤로 간다, 
    그리고 헤더와 풋터를 제외한만큼(2워드)앞으로 간다 */
/* 블록 포인터 bp를 받으면, 그 이전 블록과 이후의 블록의 주소를 반환한다 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
/* 현재 블록의 헤더로 가서 사이즈 값을 받아 그 만큼 뒤로 간다 -> 다음 블록 시작으로 간다 */
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* 이전 블록의 풋터로 간 다음 사이즈 값을 받아 이전 블록의 시작으로 간다 */
#define PRED_P(bp)  (*(void **)(bp))
#define SUCC_P(bp)  (*(void **)((bp)+WSIZE))
static void *heap_listp;
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *p, size_t size);
static void list_add(void *p);
static void list_remove(void *p);
int mm_init(void) {   
    if((heap_listp = mem_sbrk(6*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(2*DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), heap_listp+(3*WSIZE));
    PUT(heap_listp + (3*WSIZE), heap_listp+(2*WSIZE));
    PUT(heap_listp + (4*WSIZE), PACK(2*DSIZE, 1));
    PUT(heap_listp + (5*WSIZE), PACK(0,1));
    heap_listp += 2*WSIZE;
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}
static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
    return coalesce(bp);
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;
    if (size == 0)
        return NULL;
    if (size<=DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size+(DSIZE)+(DSIZE-1))/DSIZE);
    
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}
static void *find_fit(size_t asize){
    void *ptr;
    for (ptr = SUCC_P(heap_listp); !GET_ALLOC(HDRP(ptr)) ; ptr = SUCC_P(ptr)){
        if (asize <= GET_SIZE(HDRP(ptr))){
            return ptr;
        }
    }
    return NULL;
}
static void place(void *p, size_t size){
    size_t free_block = GET_SIZE(HDRP(p));
    list_remove(p);
    if ((free_block-size)>=(2*DSIZE)){
        PUT(HDRP(p), PACK(size, 1));
        PUT(FTRP(p), PACK(size, 1));
        p = NEXT_BLKP(p);
        PUT(HDRP(p), PACK(free_block-size, 0));
        PUT(FTRP(p), PACK(free_block-size, 0));
        list_add(p);
    } else {
    PUT(HDRP(p), PACK(free_block, 1));
    PUT(FTRP(p), PACK(free_block, 1));
    }
}
static void list_add(void *p){
    SUCC_P(p) = SUCC_P(heap_listp);
    PRED_P(p) = heap_listp;
    PRED_P(SUCC_P(heap_listp)) = p;
    SUCC_P(heap_listp) = p;
}
static void list_remove(void *p){
    SUCC_P(PRED_P(p)) = SUCC_P(p);
    PRED_P(SUCC_P(p)) = PRED_P(p);
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && !next_alloc){
        list_remove(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc){
        list_remove(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else if (!prev_alloc && !next_alloc){
        list_remove(PREV_BLKP(bp));
        list_remove(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    list_add(bp);
    return bp;
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copy_size;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copy_size = GET_SIZE(HDRP(oldptr));
    if (size < copy_size)
      copy_size = size;
    memcpy(newptr, oldptr, copy_size);
    mm_free(oldptr);
    return newptr;
}