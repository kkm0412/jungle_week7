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
//현재 블록 포인터 기준 헤더와 풋터 좌표 가져오는 함수
#define HDRP(bp)    ((char *)(bp) - WSIZE)  //현재 블록의 헤더
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
//ㄴ 현재 블록의 풋터(size만큼 이동 후 다음블록 헤더를 건너뛰어서(-DSIZE) 풋터로)
/* Given block ptr bp, compute address of next and previous blocks */
//현재 블록의 헤더 확인해서 다음 블록으로
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE((char*)(bp)- WSIZE))
//이전 블록의 풋터 확인해서 이전 블록으로
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE((char*)(bp)- DSIZE))
//---추가 끝

//함수 프로토타입 선언
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)   //힙 초기화 하는 함수
{
    //비어있는 힙 생성
    if ((heap_listp = mem_sbrk(4 *WSIZE)) == (void *)-1)
        return -1;
    //힙에 allignment padding 추가
    PUT(heap_listp, 0); //allignment 패딩
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));//프롤로그 헤더
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));//프롤로그 풋터
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); //에필로그 헤더

    heap_listp += (2*WSIZE);    //힙 포인터를 프롤로그 풋터로 이동

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)    
    //워드 갯수만큼 extend_heap에 넣어서 힙 확장
        return -1;
    return 0;
}
static void *extend_heap(size_t words)  //힙 자체를 확장하는 함수
{
    //블록 포인터, 페이로드의 첫번째 바이트를 가리킴. 블록 조작, 순회의 기준점.
    char *bp;   
    //크기 size_t는 해당 시스템에서 최대크기의 데이터를 표현하는 타입(stdio.h에 정의)
    size_t size;

    size = (words %2) ? (words+1) *WSIZE : words * WSIZE;
    //짝수로 하는 이유: DSIZE(8바이트)로 맞추기 위해서. double같은 타입은 8byte이기 때문에
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;    //size만큼 힙이 증가 안했으면 오류 뱉어내기
    
    //늘린 만큼 free 블록 채우고 에필로그 헤더 추가하기
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    return coalesce(bp);    //통합 블록 리턴

}

/*
 * mm_free - Freeing a block does nothing.
 */
//가리키는 블록을 free시키는 함수
void mm_free(void *bp)  
{
    size_t size = GET_SIZE(HDRP(bp));   //현 블록 사이즈 구하고
    //헤더랑 풋터에 Allocation 0으로
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);   //주변 블록이랑 합치기
}

//합친 뒤에 결과로 현재 블록의 payload 주소를 반환하는 함수
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    //이전 다음 모두 할당일때
    if(prev_alloc && next_alloc){
        return bp;
    }
    //다음 블록 free일때
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    //이전 블록 free일때
    else if(!prev_alloc && next_alloc){
        size+= GET_SIZE((HDRP(PREV_BLKP(bp))));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    //이전 다음 모두 free일때
    else{
        size += GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))) ;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// void *mm_malloc(size_t size)
// {
//     int newsize = ALIGN(size + SIZE_T_SIZE);
//     void *p = mem_sbrk(newsize);    //brk 증가 sbrk함수
//     if (p == (void *)-1)
//         return NULL;
//     else
//     {
//         *(size_t *)p = size;
//         return (void *)((char *)p + SIZE_T_SIZE);
//     }
// }
//
//실제 블록을 할당하는 함수
void *mm_malloc(size_t size)
{
    size_t asize;   //실제 적용할 사이즈
    size_t extendsize;  //free블록 부족시 늘릴 힙 사이즈

    char *bp;

    if(size==0) //size 0일때 바로 거부
        return NULL;
    
    if(size<= DSIZE)    //size가 더블워드보다 작으면 16바이트 적용? TODO: 여기 최적화 하기
        asize = 2 *DSIZE;
    else
        asize = DSIZE * ((size +DSIZE + (DSIZE-1)) / DSIZE);

    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);   //아마도 실제 위치 찾아서 할당하는 용도
        return bp;
    }

    //맞는 fit이 없으면
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

    
static void *find_fit(size_t asize)//codex로 생성
{
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;
    }

    return NULL;
}
//해당 free 블록 위치에 할당하기
static void place(void *bp, size_t asize)   //codex로 생성
{
    size_t csize = GET_SIZE(HDRP(bp));

    //free 블록이 충분히 남을 경우 남는공간 free블록으로
    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *oldptr = bp;
    void *newptr;
    size_t oldSize; //현재블록 사이즈
    size_t asize;   //실제 할당할 사이즈
    size_t copySize;    //현재 블록의 payload 사이즈 
    size_t nextSize;
    size_t totalSize;

    if (bp == NULL)
        return mm_malloc(size);

    if (size == 0) {
        mm_free(bp);
        return NULL;
    }

    oldSize = GET_SIZE(HDRP(oldptr));
    if(size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    copySize = oldSize - DSIZE;
    

    //헤더, 풋터 제외하고 데이터 만큼만 기존 크기 가져옴
    //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    //수정하기

    void *nextbp = NEXT_BLKP(oldptr);
    //만약 다음 블록이 free이고 블록 크기가 충분할때
    nextSize = GET_SIZE(HDRP(nextbp));
    totalSize = oldSize + nextSize;
    if (asize <= totalSize && !GET_ALLOC(HDRP(nextbp))){
        //확장하고 남는 공간은 free 블록으로
        if ((totalSize - asize) >= (2 * DSIZE)) {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            nextbp = NEXT_BLKP(bp);
            PUT(HDRP(nextbp), PACK(totalSize - asize, 0));
            PUT(FTRP(nextbp), PACK(totalSize - asize, 0));
        }
        else {
            PUT(HDRP(bp), PACK(totalSize, 1));
            PUT(FTRP(bp), PACK(totalSize, 1));
        }
        return oldptr;
    }
    //그렇지 않을 경우
    newptr = mm_malloc(size);   //사이즈만큼 할당
    if (newptr == NULL) //만약 새로운 포인터가 할당 안될시에
        return NULL;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
