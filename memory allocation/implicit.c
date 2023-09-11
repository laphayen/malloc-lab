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
    "team 7",
    /* First member's full name */
    "Gichan Baek",
    /* First member's email address */
    "laphayen@gmail.com",
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
#define CHUNKSIZE (1<<12)

// 최대값 가져온다.
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
// 헤더와 푸터에 저장할 수 있는 값 리턴
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
// 크기와 할당 비트를 통합해서 헤더와 푸터에 저장할 수 있는 값을 리턴
// 변수 p로 전달된 메모리 위치에서 부호 없는 정수(unsigned int)를 읽는데 사용
// 가장 바깥쪽 *은 포인터를 역참조하여 실제로 p가 가리키는 메모리 위치에서 값을 가져온다.
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/*Read the size and allocated fields from address p */
// 주소 p의 헤더와 푸터의 크기와 할당 비트를 리턴한다.
#define GET_SIZE(p) (GET(p) & ~0x7) // 뒤의 3비트를 제외하고 읽어 온다.
#define GET_ALLOC(p) (GET(p) & 0x1) // 주소 p의 헤더, 푸터의 (할당 또는 가용)을 확인한다.

/* Given block ptr bp, compute address of its header and footer */
// 각 블록의 헤더와 푸터를 가리키는 포인터를 리턴한다.
#define HDRP(bp) ((char *)(bp) - WSIZE) // bp(payload의 시작 주소)에서 1워드 만큼 앞으로 주소 이동
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // bp 주소 + 블록사이즈 - 더블 워드 사이즈(8바이트)

/* Given block ptr bp, compute address of next and previous blocks */
// 다음과 이전 블록 포인터를 각각 리턴한다.
// 다음 블록의 블록 포인터 리턴
// bp에 블록 사이즈만큼 더해서 주소를 이동하면 다음 블록으로 이동
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
// 이전 블록의 블록 포인터 리턴
// bp에서 이전 푸터의 사이즈만큼 빼주면 이전 블록의 bp로 이동
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
// 매크로 정의 완료

// 전역 힙 변수 및 함수 선언
static void *heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
void *mm_malloc(size_t size);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
void *mm_realloc(void *ptr, size_t size);

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
    /* 초기 빈 힙을 생성한다. */
    /* mem_sbrk 함수는 힙  영역을 지정된 바이트 크기만큼 확장 
    // heap_listp에 4 워드 만큼의 메모리를 확장 - 32
    새로 할당된 힙 영역의 첫  번째 바이트를 가리키는 제네릭 포인터를 반환 */
    /* 초기 확장에 실패하면 (void *)-1이 반환되고
    초기화에 실패한 것을 나타낸다. */
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    };

    /* 프롤로그는 할당된 블록의 메타데이터를 저장하는 헤더와 푸터로 구성
    에필로그는 힙 영역의 끝을 나타내는 헤더로 구성 */
    /* 힙 영역의 정렬을 위한 패딩 값 설정 */
    // 처음 패딩 영역을 추가한다.
    PUT(heap_listp, 0);                             /* Alignment padding */
    /* 프롤로그의 헤더에는 블록의 크기와 할당 여부 정보 */
    // 프롤로그 헤더는 1워드 할당, DSIZE로 값을 넣는다.
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    /* Prologue header   */
    /* 프롤로그의 푸터는 헤더와 동일한 정보 */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    /* Prologue footer   */
    /* 에필로그의 헤더는 힙 영역의 끝을 나타내며, 크기는 0이며 항상 할당 */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        /* Epilogue header   */
    /* heap_listp 포인터를 프롤로그 부분 끝으로 이동 */
    heap_listp += (2*WSIZE); 
    /* CHUNKSIZE 바이트의 가용 블록으로 빈 힙 확장 */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL){
        // 실패 시 -1 반환
        return -1;
    }
    // 모든 초기화 단계가 성공적으로 수행되면 0을 반환한다.
    return 0;
}

static void *extend_heap(size_t words)
{
    // 요청한 크기를 인접 2워드의 배수(8바이트)로 반올림하여, 그 후에 추가적인 힙 공간 요청
    char *bp;
    size_t size;
    // 요청한 크기를 2워드의 배수로 반올림하고 추가 힙 공간을 요청함
    size = (words %2) ? (words+1)*WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // 힙은 더블 워드 경계에서 시작
    // extend_heap으로 가는 모든 호출은 rm zmrlrk ejqmf dnjemdml qotndls qmffhrdmf flxjs
    // mem_sbrk로의 모든 호출은 에필로그 블록의 헤더에 곧이어서 더블 워드 정렬된 메모리 블록을 리턴

    // 새 가용 블록의 헤더
    PUT(HDRP(bp), PACK(size, 0));  //free 블록의 header
    PUT(FTRP(bp), PACK(size, 0));  //free 블록의 footer
    // 마지막 워드는 새 에필로그 블록 헤더가 된다.
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epilogue header

    // 두 개의 가용 블록을 통합하기 위해 coalesce 함수를 호출하고 
    // 통합된 블록의 블록 포인터를 리턴한다.
    return coalesce(bp);
}

// 가용 블록 연결
// 할당된 블록을 합칠 수 있는 경우 4가지에 따라 메모리 연결
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // Case 1 - 이전과 다음 블록이 할당
    // 현재 블록만 가용 상태로 변경
    if (prev_alloc && next_alloc){
        return bp;
    }
    // Case 2 - 이전 블록은 할당, 다음 블록은 가용
    // 현재 블록과 다음 블록을 통합한다.
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    // Case 3 - 이전 블록은 가용, 다음 블록은 할당
    // 이전 블록과 현재 블록을 통합한다.
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    // Case 4 - 이전 블록과 다음 블록 가용
    // 이전, 현재, 다음 블록 통합
    else{
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP((bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    return bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// 요청한 size 만큼의 공간이 있는 메모리 블록을 할당
void *mm_malloc(size_t size)
{
    // 블록 크기 조정
    size_t asize;
    // 맞지 않는 경우 힙을 확장할 양
    size_t extendsize;
    char *bp;

    // 거짓 요청을 무시한다.
    if (size == 0)
    {
        return NULL;
    }

    // 오버헤드 및 정렬 요청을 포함하도록 블록 크기 조정
    // size를 바탕으로 헤더와 푸터의 공간 확보
    // 최소 16 바이트 크기의 블록을 구성
    if (size <= DSIZE)
    {
        asize = 2*DSIZE;
    }
    else{
        // 8바이트는 정렬조건을 만족하기 위해
        // 추가 8바이트는 헤더와 푸터 오버헤드를 위해서 확보
        asize = DSIZE*((size+(DSIZE) + (DSIZE-1)) / DSIZE);
    }

    // first fit 방식을 사용한다.
    // 가용 블록을 가용리스트에서 검색하고 할당기는 요청한 블록을 배치한다.
    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    // 맞는 블록을 찾기 못한다면 새로운 가용 블록으로 확장하고 배치한다.
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    // 헤더와 푸터를 0으로 할당한다.
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    // coalesce를 호출하여 가용 메모리를 연결한다.
    coalesce(ptr);
}

// 적절한 가용 블록을 검색하고 가용블록의 주소를 반환
static void *find_fit(size_t asize){
    //first fit 검색을 수행 - 리스트 처음부터 탐색하여 가용블록 찾기
    void *bp;
    //헤더의 사이즈가 0보다 크면 에필로그까지 탐색한다.
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) >0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}

// void bp* - 가용 블록의 주소, size_t asize - 가용 블록의 할당하는 사이즈
static void place(void *bp, size_t asize){
    // 맞는 블록을 찾으면 요청한 블록을 배치하고 초과부분을 분할한다.
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2*DSIZE)){
        //가용 블록에 사이즈 - 요청한 블록의 사이즈 각 더블워드*2 크거나 같을때
        //요청 블록을 넣고 남은 사이즈는 가용 블록으로 분할
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }else{
        //할당하고 남은 블록이 더블워드*2보다 작다며 나누지 않고 할당
        // 남은 블록이 더블워드*2보다 작은 경우는 데이터를 담을 수 없음
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
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
