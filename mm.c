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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 매크로 작성
/* Basic constants and macros */
// 워드, 헤더, 푸터의 사이즈를 4
#define WSIZE 4
// 더블 워드 사이즈를 8
#define DSIZE 8

// 힙을 확장 시 지정 - 4바이트
#define DHUNKSIZE (1<<12)

// 최대값 가져온다.
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word*/
// 헤더와 푸터에 저장할 수 있는 값 리턴
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
// 크기와 할당 비트를 통합해서 헤더와 푸터에 저장할 수 있는 값을 리턴한다.
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/*Read the size and allocated fields from address p */
// 주소 p의 헤더와 푸터의 크기와 할당 비트를 리턴한다.
#define GET_SIZE(p) (GET(p) & ~0x7) // 뒤의 3비트를 제외하고 읽어 온다.
#define GET_ALLOC(p) (GET(p) & 0x1) // 할당 가용을 확인한다.

/* Given block ptr bp, compute address of its header and footer */
// 각 블록의 헤더와 푸터를 가리키는 포인터를 리턴한다.
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
// 다음과 이전 블록 포인터를 각각 리턴한다.
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
// 매크로 정의 완료


// 할당기 자체는 사용자가 자신의 응용에서 컴파일하고 링크할 수 있는 소스 파일에 포함한다.
extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);

/* 
 * mm_init - initialize the malloc package.
 */
// 힙에 가용한 가상메모리를 큰 더블 워드 정렬된 바이트의 배열로 모델한 것.
// mem_heap과 mem_brk 사이의 바이트들은 할당된 가상메모리를 나타낸다.
// mem_brk 다음에 오는 바이트들은 미할당 가상메모리를 나타낸다.
// 할당기는 mem_sbrk 함수를 호출해서 추가적인 힙 메모리를 요청하며, 
// 이것은 힙을 축소하라는 요청을 거부하는 것만 제외하고는 
// 시스템의 sbrk함수와 동일한 의미뿐만 아니라 동일한 인터페이스를 갖는다.
// 할당기 자체는 사용자 자신의 응용에서 컴파일하고 링크할 수 있는 소스파일(mm.c)에 포함된다.
// 할당기는 세 개의 함수를 응용 프로그램에 알려준다.
int mm_init(void)
{
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














