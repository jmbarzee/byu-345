// os345p4.c - Virtual Memory

#include "os345.h"

// ***********************************************************************
// project 4 variables
extern TCB tcb[];             // task control block
extern int curTask;           // current task #

extern int memAccess;
extern int memHits;
extern int memPageFaults;
extern int nextPage;
extern int pageReads;
extern int pageWrites;
extern int c1;
extern int c2;

extern unsigned short int memory[];
extern int getMemoryData(int);

// ***********************************************************************
// project 4 functions and tasks
static void load_lc3_file(char* string);
static void dump_memory(char *s, int sa, int ea);
static void dump_vmemory(char *s, int sa, int ea);
static void display_frame(int f);
static void display_rpt(int rptNum);
static void display_upt(int rptNum, int uptNum);
static void display_page(int pn);
static void display_pt(int pta, int badr, int inc);
static void look_vm(int va);

// *****************************************************************************
// project4 command
//
//
int P4_project4(int argc, char* argv[]) {       // project 5
  // initialize lc3 memory
  P4_initMemory(argc, argv);

  // start lc3 tasks
  load_lc3_file("memtest.hex");
  load_lc3_file("crawler.hex");

  load_lc3_file("memtest.hex");
  load_lc3_file("crawler.hex");

  load_lc3_file("memtest.hex");
  load_lc3_file("crawler.hex");

  return 0;
}


// **************************************************************************
// **************************************************************************
// ---------------------------------------------------------------------
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023)
//     / / / /     /                 /__________Swap page defined
//    / / / /     /                 //        __page # (0-4096)
//   / / / /     /                 //        /
//  / / / /     /                //        /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p

// ---------------------------------------------------------------------
// **************************************************************************
// dm <sa>,<ea>
int P4_dumpLC3Mem(int argc, char* argv[]) {
  printf("\nValidate arguments...");  // ?? validate arguments
  int sa = INTEGER(argv[1]);
  int ea = sa + 0x0040;

  dump_memory("LC-3 Memory", sa, ea);
  return 0;
} // end P4_dumpLC3Mem



// **************************************************************************
// **************************************************************************
// vma <a>
int P4_vmaccess(int argc, char* argv[]) {
  unsigned short int adr, rpt, upt;

  printf("\nValidate arguments...");  // ?? validate arguments
  adr = INTEGER(argv[1]);

  printf(" = %04lx", getMemAdr(adr, 1) - &MEMWORD(0));
  for (rpt = 0; rpt < 64; rpt += 2) {
    if (MEMWORD(rpt+TASK_RPT) || MEMWORD(rpt + TASK_RPT + 1)) {
      outPTE("  RPT  =", rpt + TASK_RPT);
      for(upt = 0; upt < 64; upt += 2) {
        if (DEFINED(MEMWORD(rpt+TASK_RPT)) &&
          (DEFINED(MEMWORD((FRAME(MEMWORD(rpt + TASK_RPT)) << 6)+upt))
          || PAGED(MEMWORD((FRAME(MEMWORD(rpt + TASK_RPT)) << 6)+upt+1)))) {
          outPTE("    UPT=", (FRAME(MEMWORD(rpt + TASK_RPT)) << 6)+upt);
        }
      }
    }
  }
  printf("\nPages = %d", accessPage(0, 0, PAGE_GET_SIZE));
  return 0;
} // end P4_vmaccess



// **************************************************************************
// **************************************************************************
// pm <#>  Display page frame
int P4_dumpPageMemory(int argc, char* argv[]) {
  printf("\nValidate arguments...");  // ?? validate arguments
  int page = INTEGER(argv[1]);

  display_page(page);
  return 0;
} // end P4_dumpPageMemory



// **************************************************************************
// **************************************************************************
// im <a>  Initialize LC-3 memory bound
int P4_initMemory(int argc, char* argv[]) {
  int highAdr = 0x8000;

  tcb[curTask].RPT = 0x2400;


  printf("\nValidate arguments...");  // ?? validate arguments
  if (!tcb[curTask].RPT) {
    printf("\nTask RPT Invalid!");
    return 1;
  }
  if (argc > 1) {
    highAdr = INTEGER(argv[1]);
  }
  if (highAdr < 0x3000) {
    highAdr = (highAdr << 6) + 0x3000;
  }
  if (highAdr > 0xf000) {
    highAdr = 0xf000;
  }
  printf("\nSetting upper memory limit to 0x%04x", highAdr);

  c2 = LC3_RPT;
  c1 = 0;

  // init LC3 memory
  initLC3Memory(LC3_MEM_FRAME, highAdr >> 6);
  printf("\nPhysical Address Space = %d frames (%0.1fkb)",
         (highAdr >> 6) - LC3_MEM_FRAME, ((highAdr >> 6) - LC3_MEM_FRAME) / 8.0);

  memAccess = 0;              // vm statistics
  memHits = 0;
  memPageFaults = 0;

  // accessPage(0, 0, PAGE_INIT);
  nextPage = 0;
  pageReads = 0;
  pageWrites = 0;

  return 0;
} // end P4_initMemory



// **************************************************************************
// **************************************************************************
// dvm <sa>,<ea>
// dump virtual lc-3 memory
int P4_dumpVirtualMem(int argc, char* argv[]) {
  printf("\nValidate arguments...");  // ?? validate arguments
  int sa = INTEGER(argv[1]);
  int ea = sa + 0x0040;

  dump_vmemory("LC-3 Virtual Memory", sa, ea);
  look_vm(sa);
  return 0;
} // end P4_dumpVirtualMem



// **************************************************************************
// **************************************************************************
// vms
int P4_virtualMemStats(int argc, char* argv[]) {
  int nextPage = accessPage(0, 0, PAGE_GET_SIZE);
  int pageReads = accessPage(0, 0, PAGE_GET_READS);
  int pageWrites = accessPage(0, 0, PAGE_GET_WRITES);
  double missRate = memAccess ? (((double)memPageFaults) / (double)memAccess) * 100.0 : 0;

  printf("\nMemory accesses = %d", memAccess);
  printf("\n           hits = %d", memHits);
  printf("\n         faults = %d", memPageFaults);
  printf("\n           rate = %f%%", missRate);
  printf("\n     Page reads = %d", pageReads);
  printf("\n    Page writes = %d", pageWrites);
  printf("\nSwap page count = %d (%d kb)", nextPage, nextPage>>3);
  return 0;
} // end P4_virtualMemStats


// **************************************************************************
// **************************************************************************
// dft
int P4_dumpFrameTable(int argc, char* argv[]) {
  dump_memory("Frame Bit Table", LC3_FBT, LC3_FBT+0x40);
  return 0;
} // end P4_dumpFrameTable



// **************************************************************************
// **************************************************************************
// dfm <frame>
int P4_dumpFrame(int argc, char* argv[]) {
  printf("\nValidate arguments...");  // ?? validate arguments
  int frame = INTEGER(argv[1]);

  display_frame(frame%LC3_FRAMES);
  return 0;
} // end P4_dumpFrame



// **************************************************************************
// **************************************************************************
// rpt <#>       Display process root page table
int P4_rootPageTable(int argc, char* argv[]) {
  printf("\nValidate arguments...");  // ?? validate arguments
  int rpt = INTEGER(argv[1]);

  display_rpt(rpt);
  return 0;
} // end P4_rootPageTable



// **************************************************************************
// **************************************************************************
// upt <p><#>    Display process user page table
int P4_userPageTable(int argc, char* argv[]) {
  printf("\nValidate arguments...");  // ?? validate arguments
  int rpt = INTEGER(argv[1]);
  int upt = INTEGER(argv[2]);

  display_upt(rpt, upt >> 11);
  return 0;
} // P4_userPageTable



// **************************************************************************
// **************************************************************************
static void display_frame(int f) {
   char mesg[128];
   sprintf(mesg, "Frame %d", f);
   dump_memory(mesg, f*LC3_FRAME_SIZE, (f+1)*LC3_FRAME_SIZE);
} // end display_frame



// **************************************************************************
// **************************************************************************
// display contents of Root Page Table rptNum
static void display_rpt(int rptNum) {
   display_pt(LC3_RPT + (rptNum << 6), 0, 1 << 11);
} // end display_rpt



// **************************************************************************
// **************************************************************************
// display contents of UPT
static void display_upt(int rptNum, int uptNum) {
  unsigned short int rpte, upt, upte1, upte2, uptba;
  rptNum &= BITS_3_0_MASK;
  uptNum &= BITS_4_0_MASK;

  // index to process <rptNum>'s rpt + <uptNum> index
  rpte = MEMWORD(((LC3_RPT + (rptNum<<6)) + uptNum*2));
  // calculate upt's base address
  uptba = uptNum<<11;
  if (DEFINED(rpte)) {
    upt = FRAME(rpte)<<6;
  } else {
    printf("\nUndefined!");
    return;
  }
  display_pt(upt, uptba, 1<<6);
} // end display_upt



// **************************************************************************
// **************************************************************************
// output page table entry
void outPTE(char* s, int pte) {
  // read pt
  int pte1 = memory[pte];
  int pte2 = memory[pte + 1];

  // look at appropriate flags
  char flags[8];
  strcpy(flags, "----");
  if (DEFINED(pte1)) {
    flags[0] = 'F';
  }
  if (DIRTY(pte1)) {
    flags[1] = 'D';
  }
  if (REFERENCED(pte1)) {
    flags[2] = 'R';
  }
  if (PINNED(pte1)) {
    flags[3] = 'P';
  }

  // output pte line
  printf("\n%s x%04x = %04x %04x  %s", s, pte, pte1, pte2, flags);
  if (DEFINED(pte1) || DEFINED(pte2)) {
    printf(" Frame=%d", FRAME(pte1));
  }
  if (DEFINED(pte2)) {
    printf(" Page=%d", SWAPPAGE(pte2));
  }
} // end outPTE



// **************************************************************************
// **************************************************************************
// display page table entries
static void display_pt(int pta, int badr, int inc) {
  char buf[32];

  for (int i = 0; i < 32; i++) {
    sprintf(buf, "(x%04x-x%04x) ", badr+ i*inc, badr + ((i+1)*inc)-1);
    outPTE("", (pta + i*2));
  }
} // end display_pt



// **************************************************************************
// **************************************************************************
// look at virtual memory location va
static void look_vm(int va) {
   unsigned short int rpte1, rpte2, upte1, upte2, pa;

   // get root page table entry
  rpte1 = MEMWORD(LC3_RPT + RPTI(va));
  rpte2 = MEMWORD(LC3_RPT + RPTI(va) + 1);
  if (DEFINED(rpte1)) {
    upte1 = MEMWORD((FRAME(rpte1) << 6) + UPTI(va));
    upte2 = MEMWORD((FRAME(rpte1) << 6) + UPTI(va) + 1);
  } else {
    // rpte undefined
    printf("\n  RTB[Undefined]");
    return;
  }
  if (DEFINED(upte1)) {
    pa = (FRAME(upte1) << 6) + FRAMEOFFSET(va);
  } else {
    // upte undefined
    printf("\n  UTB[Undefined]");
    return;
  }
  printf("\n  RPT[0x%04x] = %04x %04x", LC3_RPT + RPTI(va), rpte1, rpte2);
  if (rpte1 & BIT_14_MASK) {
    printf(" D");
  }
  if (rpte1 & BIT_13_MASK) {
    printf(" R");
  }
  if (rpte1 & BIT_12_MASK) {
    printf(" P");
  }
  printf(" Frame=%d", rpte1&0x03ff);
  if (DEFINED(rpte2)) {
    printf(" Page=%d", rpte2&0x0fff);
  }
  printf("\n  UPT[0x%04x] = %04x %04x", (FRAME(rpte1)<<6) + UPTI(va), upte1, upte2);
  if (upte1 & BIT_14_MASK) {
    printf(" D");
  }
  if (upte1 & BIT_13_MASK) {
    printf(" R");
  }
  if (upte1 & BIT_12_MASK) {
    printf(" P");
  }
  printf(" Frame=%d", upte1&0x03ff);
  if (DEFINED(upte2)) {
    printf(" Page=%d", upte2&0x0fff);
  }
  printf("\n  MEM[0x%04x] = %04x", pa, MEMWORD(pa));
} // end look_vm



// **************************************************************************
// **************************************************************************
// pm <#>  Display page frame
static void display_page(int pn) {
  short int *buffer;
  printf("\nPage %d", pn);
  buffer = (short int*)accessPage(pn, pn, 3);
  for (int ma = 0; ma < 64;) {
    printf("\n0x%04x:", ma);
    for (int i = 0; i < 8; i++) {
      printf(" %04x", MASKTO16BITS(buffer[ma + i]));
    }
    ma += 8;
  }
} // end displayPage



// **************************************************************************
// **************************************************************************
// dm <sa> <ea> - dump lc3 memory
static void dump_memory(char *s, int sa, int ea) {
  printf("\n%s", s);
  for (int ma = sa; ma < ea;) {
    printf("\n0x%04x:", ma);
    for (int i = 0; i < 8; i++) {
      printf(" %04x", MEMWORD((ma+i)));
    }
    ma += 8;
  }
} // end dumpMemory



// **************************************************************************
// *****************************************************************************
// dvm <sa> <ea> - dump lc3 virtual memory
static void dump_vmemory(char *s, int sa, int ea) {
  printf("\n%s", s);
  for (int ma = sa; ma < ea;) {
    printf("\n0x%04x:", ma);
    for (int i = 0; i < 8; i++) {
      printf(" %04x", getMemoryData(ma + i));
    }
    ma += 8;
  }
} // end dumpVMemory



// *****************************************************************************
// *****************************************************************************
// crawler and memtest programs
/*
 * @param string - The name of the file containing the program to be loaded
 */
static void load_lc3_file(char* string) {
  char* myArgv[2];
  char buff[32];

  strcpy(buff, string);
  if (strchr(buff, '.')) {
    *(strchr(buff, '.')) = 0;
  }

  myArgv[0] = buff;
  myArgv[1] = string;
  createTask(myArgv[0], lc3Task, MED_PRIORITY, 2, myArgv);
} // end loadFile



// *****************************************************************************
// *****************************************************************************
int P4_crawler(int argc, char* argv[]) {
  load_lc3_file("crawler.hex");
  return 0;
} // end P4_crawler



// *****************************************************************************
// *****************************************************************************
int P4_memtest(int argc, char* argv[]) {
  load_lc3_file("memtest.hex");
  return 0;
} // end crawler and memtest programs

// *****************************************************************************
