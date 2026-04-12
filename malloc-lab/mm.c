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
    "2team",
    /* First member's full name */
    "Kyumin Kim",
    /* First member's email address */
    "kkm010412@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

// /* single word (4) or double word (8) alignment */
// #define ALIGNMENT 8

// /* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 추가
static char *heap_listp;   //힙의 포인터

/* Basic constants and macros */
#define WSIZE 4 //워드 사이즈 /* Word and header/footer size (bytes) */ 
#define DSIZE 8 //더블 워드 사이즈
#define CHUNKSIZE (1<<12)   //4KB 2^12(비트 왼쪽시프트 12번)
//ㄴ초기 free 블록 생성, 메모리 할당시 공간이 부족할때 사용

#define MAX(x, y) ((x) >(y) ? (x) : (y))
//ㄴ둘중 큰거 비교 (free 블록 부족해서 힙메모리 확장 할때 얼만큼 확장할지 결정용)

/* Pack a size and allocated bit into a word */
//헤더, 풋터에 크기, 가용여부 저장
#define PACK(size, alloc) ((size) | (alloc))    

/* Read and write a word at address p*/
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

//p가 헤더 또는 풋터여야 함.
#define GET_SIZE(p) (GET(p) & ~0x7) //뒤에 alloc 지우고 size만 가져옴
#define GET_ALLOC(p) (GET(p) & 0x1) //alloc만 뽑아오기

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char *)(bp) - WSIZE)  //현재 블록의 헤더
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
//ㄴ 현재 블록의 풋터(size만큼 이동 후 다음블록 헤더를 건너뛰어서(-DSIZE) 풋터로)
/* Given block ptr bp, compute address of next and previous blocks */
//현재 블록의 헤더 확인해서 다음 블록으로
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE((char*)(bp)- WSIZE))
//이전 블록의 풋터 확인해서 이전 블록으로
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE((char*)(bp)- DSIZE))
//---추가 끝
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    //비어있는 힙 생성
    if ((heap_listp = mem_sbrk(4 *WSIZE)) == (void *)-1)
        return -1;
    //힙에 allignment padding 추가
    PUT(heap_listp, 0); //allignment 패딩
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));//프롤로그 헤더
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));//프롤로그 풋터
    PUT(heap_listp + (3+WSIZE), PACK(0, 1)); //에필로그 헤더

    heap_listp += (2*WSIZE);    //힙 포인터를 프롤로그 풋터로 이동

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)    
    //워드 갯수만큼 extend_heap에 넣어서 힙 확장
        return -1;
    return 0;
}
static void *extend_heap(size_t words)
{
    //블록 포인터, 페이로드의 첫번째 바이트를 가리킴. 블록 조작, 순회의 기준점.
    char *bp;   
    //크기 size_t는 해당 시스템에서 최대크기의 데이터를 표현하는 타입(stdio.h에 정의)
    size_t size;

    
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);    //brk 증가 sbrk함수
    if (p == (void *)-1)
        return NULL;
    else
    {
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