// os345mmu.c - LC-3 Memory Management Unit


#include "os345.h"

// ***********************************************************************
// mmu variables

// LC-3 memory
unsigned short int memory[LC3_MAX_MEMORY];

// statistics
int memAccess;            // memory accesses
int memHits;            // memory hits
int memPageFaults;          // memory faults
int nextPage;            // swap page size
int pageReads;           // page reads
int pageWrites;            // page writes
int c1, c2;

int getFrame(const int);
int get_clock_frame(int);
void swap_frame_out(int idx);
static int get_available_frame(void);
extern TCB tcb[];         // task control block
extern int curTask;         // current task #

int getFrame(const int notme) {
  int frame = get_available_frame();
  if (frame >= 0) {
    return frame;
  }

  frame = get_clock_frame(notme);

  // run clock
  // printf("\nWe're toast!!!!!!!!!!!!");

  return frame;
}


int get_clock_frame(int val) {
  int frame;
  int max_wrap = 20;
  for(;max_wrap; c2 += 2, c1 = 0) {
    int upta, upte1;
    if (c2 >= LC3_RPT_END) {
      c2 = LC3_RPT;
      max_wrap--;
    }

    int rpte1 = memory[c2];
    if (DEFINED(rpte1) && REFERENCED(rpte1)) {
      memory[c2] = rpte1 = CLEAR_REF(rpte1);
    } else if (DEFINED(rpte1)) {
      upta = FRAME(rpte1) << 6;

      for (int i = c1 % 64; i < 64; i += 2, c1 = i % 64) {
        upte1 = memory[upta + i];
        if (PINNED(upte1) || FRAME(upte1) == val) {
          continue;
        }

        if (DEFINED(upte1) && REFERENCED(upte1)) {
          // clear reference
          memory[c2] = rpte1 = SET_PINNED(rpte1);
          memory[upta + i] = upte1 = CLEAR_REF(upte1);
        } else if (DEFINED(upte1)) {
          memory[c2] = rpte1 = SET_DIRTY(rpte1);
          frame = FRAME(upte1);
          swap_frame_out(upta + i);
          c1 += 2;
          return frame;
        }
      }
      c1 = 0;
      if (!REFERENCED(rpte1) && !PINNED(rpte1) && FRAME(rpte1) != val) {
        frame = FRAME(rpte1);
        swap_frame_out(c2);
        c2 += 2;
        return frame;
      } else {
        memory[c2] = rpte1 = CLEAR_PINNED(rpte1);
      }
    }
  }

  return 1;
}


void swap_frame_out(int idx) {
  int a = memory[idx];
  int b = memory[idx + 1];

  if (DIRTY(a) && PAGED(b)) {
    accessPage(SWAPPAGE(b), FRAME(a), PAGE_OLD_WRITE);
  } else if (!PAGED(b)) {
    memory[idx + 1] = b = SET_PAGED(nextPage);
    accessPage(nextPage, FRAME(a), PAGE_NEW_WRITE);
  }
  memory[idx] = 0;
}


// **************************************************************************
// **************************************************************************
// LC3 Memory Management Unit
// Virtual Memory Process
// **************************************************************************
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023) (2^10)
//     / / / /     /                 / _________page defined
//    / / / /     /                 / /       __page # (0-4096) (2^12)
//   / / / /     /                 / /       /
//  / / / /     /                / /       /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p

#define MMU_ENABLE  1

unsigned short int *getMemAdr(int va, int rwFlg) {
  unsigned short int pa;

  // turn off virtual addressing for system RAM
  if (va < 0x3000) {
    return &memory[va];
  }

  memAccess += 2;

#if MMU_ENABLE
  int rpta = tcb[curTask].RPT + RPTI(va);   // root page table address
  int rpte1 = memory[rpta];         // FDRP__ffffffffff
  int rpte2 = memory[rpta+1];         // S___pppppppppppp
  if (DEFINED(rpte1)) {
    // rpte defined
    memHits++;
  } else {
    // rpte undefined
    // TODO: use getFrame to return a new UPT frame from available memory and initialize all user page table entries
    memPageFaults++;
    int rptFrame = getFrame(-1);
    rpte1 = SET_DEFINED(rptFrame);
    if (PAGED(rpte2)) {
      accessPage(SWAPPAGE(rpte2), rptFrame, PAGE_READ);
    } else {
      memset(&memory[(rptFrame << 6)], 0, 128);
    }
    // va = getFrame();
  }
  memory[rpta] = SET_REF(rpte1);      // set rpt frame access bit
  memory[rpta+1] = rpte2;

  int upta = (FRAME(rpte1) << 6) + UPTI(va);  // user page table address
  int upte1 = memory[upta];           // FDRP__ffffffffff
  int upte2 = memory[upta+1];         // S___pppppppppppp
  if (DEFINED(upte1)) {
    // upte defined
    // printf("upte defined\n");
    memHits++;
  } else {
    // upte undefined
    // use getFrame to return a new data frame from available memory
    memPageFaults++;
    int uptFrame = getFrame(FRAME(memory[rpta]));
    memory[rpta] = SET_REF(SET_DIRTY(rpte1));
    upte1 = SET_DEFINED(uptFrame);

    if (PAGED(upte2)) {
      accessPage(SWAPPAGE(upte2), uptFrame, PAGE_READ);
    } else {
      memset(&memory[(uptFrame << 6)], 0xf025, 128);
    }
  }

  if (rwFlg) {
    upte1 = SET_DIRTY(upte1);
  }

  memory[upta] = SET_REF(upte1);      // set upt frame access bit
  memory[upta + 1] = upte2;
  return &memory[(FRAME(upte1) << 6) + FRAMEOFFSET(va)];
#else
  return &memory[va];
#endif
} // end getMemAdr


// **************************************************************************
// **************************************************************************
// set frames available from sf to ef
//    flg = 0 -> clear all others
//        = 1 -> just add bits
//
/**
 *  @param sf - start frame
 *  @param ef - end frame
 */
void setFrameTableBits(int flg, int sf, int ef) {
  int data;
  int adr = LC3_FBT - 1;             // index to frame bit table
  int fmask = 0x0001;              // bit mask

  // 1024 frames in LC-3 memory
  for (int i = 0; i < LC3_FRAMES; i++) {
    if (fmask & 0x0001) {
      fmask = 0x8000;
      adr++;
      data = flg ? MEMWORD(adr) : 0;
    } else {
      fmask = fmask >> 1;
    }
    // allocate frame if in range
    if (i >= sf && i < ef) {
      data = data | fmask;
    }
    MEMWORD(adr) = data;
  }
} // end setFrameTableBits


// **************************************************************************
// get frame from frame bit table (else return -1)
static int get_available_frame() {
  int data;
  int adr = LC3_FBT - 1;        // index to frame bit table
  int fmask = 0x0001;         // bit mask

  for (int i = 0; i < LC3_FRAMES; i++) {    // look thru all frames
    if (fmask & 0x0001) {
      fmask = 0x8000;       // move to next work
      adr++;
      data = MEMWORD(adr);
    } else {
      fmask = fmask >> 1;   // next frame
    }
    // deallocate frame and return frame #
    if (data & fmask) {
      MEMWORD(adr) = data & ~fmask;
      return i;
    }
  }
  return -1;
} // end get_available_frame



// **************************************************************************
// read/write to swap space
int accessPage(int pnum, int frame, int rwnFlg) {
  static unsigned short int swapMemory[LC3_MAX_SWAP_MEMORY];

  if ((nextPage >= LC3_MAX_PAGE) || (pnum >= LC3_MAX_PAGE)) {
    printf("\nVirtual Memory Space Exceeded!  (%d)", LC3_MAX_PAGE);
    exit(-4);
  }
  switch (rwnFlg) {
  case PAGE_INIT:                       // init paging
    nextPage = 0;           // disk swap space size
    return 0;

  case PAGE_GET_SIZE:                     // return swap size
    return nextPage;

  case PAGE_GET_READS:                    // return swap reads
    return pageReads;

  case PAGE_GET_WRITES:                    // return swap writes
    return pageWrites;

  case PAGE_GET_ADR:                      // return page address
    return (int)(&swapMemory[pnum << 6]);

  case PAGE_NEW_WRITE:                   // new write (Drops thru to write old)
    pnum = nextPage++;

  case PAGE_OLD_WRITE:                   // write
    //printf("\n    (%d) Write frame %d (memory[%04x]) to page %d", p.PID, frame, frame<<6, pnum);
    memcpy(&swapMemory[pnum << 6], &memory[frame << 6], 1 << 7);
    pageWrites++;
    return pnum;

  case PAGE_READ:                     // read
    //printf("\n    (%d) Read page %d into frame %d (memory[%04x])", p.PID, pnum, frame, frame<<6);
    memcpy(&memory[frame << 6], &swapMemory[pnum << 6], 1 << 7);
    pageReads++;
    return pnum;

  case PAGE_FREE:                   // free page
    printf("\nPAGE_FREE not implemented");
    memset(&memory[(frame << 6)], 0xf025, 128);
    break;
  }
  return pnum;
} // end accessPage
