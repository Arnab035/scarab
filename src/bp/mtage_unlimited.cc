/**
 * @file mtage_unlimited.cc
 * @brief P. Michaud and A. Seznec
 * @version 0.1
 * @date 2021-01-12
 *
 * Adapted to Scarab by Stephen Pruett
 *
 * Code is derived from P.Michaud and A. Seznec code for the CBP4 winner Sorry
 * two very different code writing styles
 *
 * ////////////// STORAGE BUDGET JUSTIFICATION ////////////////
 *   unlimited size
 */

#include "mtage_unlimited.h"

#include "bp/bp.param.h"

// for my personal statistics
int XX, YY, ZZ, TT;
void PrintStat(double NUMINST) {
  printf("  \nTAGE_MPKI   \t : %10.4f", 1000.0 * (double)(XX) / NUMINST);
  printf("  \nCOLT_MPKI    \t : %10.4f", 1000.0 * (double)(ZZ) / NUMINST);
  printf("  \nNEURAL_MPKI    \t : %10.4f", 1000.0 * (double)(YY) / NUMINST);
  printf("  \nSC_MPKI    \t : %10.4f", 1000.0 * (double)(TT) / NUMINST);

  //    printf("  XX %d YY %d ZZ %d TT %d",XX,YY,ZZ,TT);
}

bool COPRED;
// VARIABLES FOR THE Statistical correctors
int LSUM;
bool predSC;
bool predfinal;
bool pred_inter;

// parameters specific to the global tage
int npred = NPRED;
int P0_NUMG = 25;
int P0_LOGB = 21;
int P0_LOGG = 21;
int P0_MAXHIST = 5000;
int P0_MINHIST = 7;
int P0_HASHPARAM = 3;
int P0_RAMPUP = 100000;

// int LOGCOLT = 20;
int TAGBITS = 15;

#define IMLI  // just to be able to isolate IMLI impact: marginal on CBP5 traces

#define PERCWIDTH 8
#define GPSTEP 6
#define LPSTEP 6
#define BPSTEP 6
#define PPSTEP 6
#define SPSTEP 6
#define YPSTEP 6
#define TPSTEP 6

#define GWIDTH 60
#define LWIDTH 60
#define BWIDTH 42
#define PWIDTH 60
#define SWIDTH 60
#define YWIDTH 60
#define TWIDTH 60

#define LOGTAB 19
#define TABSIZE (1 << LOGTAB)
#define LOGB LOGTAB
#define LOGG LOGTAB
#define LOGSIZE (10)
#define LOGSIZEG (LOGSIZE)
#define LOGSIZEL (LOGSIZE)
#define LOGSIZEB (LOGSIZE)
#define LOGSIZES (LOGSIZE)
#define LOGSIZEP (LOGSIZE)
#define LOGSIZEY (LOGSIZE)
#define LOGSIZET (LOGSIZE)

// The perceptron-inspired components
int8_t PERC[(1 << LOGSIZEP)][10 * (1 << GPSTEP)];
int8_t PERCLOC[(1 << LOGSIZEL)][10 * (1 << LPSTEP)];
int8_t PERCBACK[(1 << LOGSIZEB)][10 * (1 << BPSTEP)];
int8_t PERCYHA[(1 << LOGSIZEY)][10 * (1 << YPSTEP)];
int8_t PERCPATH[(1 << LOGSIZEP)][10 * (1 << PPSTEP)];
int8_t PERCSLOC[(1 << LOGSIZES)][10 * (1 << SPSTEP)];
int8_t PERCTLOC[(1 << LOGSIZET)][10 * (1 << TPSTEP)];

// four  local histories components

#define LOGLOCAL 10
#define NLOCAL (1 << LOGLOCAL)
#define INDLOCAL (PC & (NLOCAL - 1))
long long L_shist[NLOCAL] = {0};

#define LNB 15
int Lm[LNB] = {2, 4, 6, 9, 12, 16, 20, 24, 29, 34, 39, 44, 50, 56, 63};
int8_t LGEHLA[LNB][TABSIZE];
int8_t* LGEHL[LNB] = {0};

// Local history  +IMLI
#define LINB 10
int LIm[LNB] = {18, 20, 24, 29, 34, 39, 44, 50, 56, 63};

int8_t LIGEHLA[LNB][TABSIZE];
int8_t* LIGEHL[LNB] = {0};

#define LOGSECLOCAL 4
#define NSECLOCAL (1 << LOGSECLOCAL)
#define NB 3
#define INDSLOCAL (((PC ^ (PC >> 5)) >> NB) & (NSECLOCAL - 1))
long long S_slhist[NSECLOCAL] = {0};

#define SNB 15
int Sm[SNB] = {2, 4, 6, 9, 12, 16, 20, 24, 29, 34, 39, 44, 50, 56, 63};
int8_t SGEHLA[SNB][TABSIZE];
int8_t* SGEHL[SNB] = {0};

#define LOGTLOCAL 4
#define NTLOCAL (1 << LOGTLOCAL)
#define INDTLOCAL (((PC ^ (PC >> 3) ^ (PC >> 6))) & (NTLOCAL - 1))
long long T_slhist[NTLOCAL] = {0};

#define TNB 15
int Tm[TNB] = {2, 4, 6, 9, 12, 16, 20, 24, 29, 34, 39, 44, 50, 56, 63};
int8_t TGEHLA[TNB][TABSIZE];
int8_t* TGEHL[TNB] = {0};

#define QPSTEP 6
#define QWIDTH 60
#define LOGSIZEQ (LOGSIZE)
int8_t PERCQLOC[(1 << LOGSIZEQ)][10 * (1 << QPSTEP)];

#define LOGQLOCAL 15
#define NQLOCAL (1 << LOGQLOCAL)
#define INDQLOCAL (((PC ^ (PC >> 2) ^ (PC >> 4) ^ (PC >> 8))) & (NQLOCAL - 1))
long long Q_slhist[NQLOCAL] = {0};

#define QNB 15
int Qm[QNB] = {2, 4, 6, 9, 12, 16, 20, 24, 29, 34, 39, 44, 50, 56, 63};
int8_t QGEHLA[QNB][TABSIZE];
int8_t* QGEHL[QNB] = {0};

// correlation at constant local history ? (without PC)
#define QQNB 10
int QQm[QQNB] = {16, 20, 24, 29, 34, 39, 44, 50, 56, 63};
int8_t QQGEHLA[QQNB][TABSIZE];
int8_t* QQGEHL[QQNB] = {0};

// history at IMLI constant

#define LOGTIMLI 12
#define NTIMLI (1 << LOGTIMLI)
#define INDIMLI (IMLIcount & (NTIMLI - 1))
long long IMLIhist[NTIMLI] = {0};

#define IMLINB 15
int IMLIm[IMLINB] = {2, 4, 6, 9, 12, 16, 20, 24, 29, 34, 39, 44, 50, 56, 63};
int8_t IMLIGEHLA[IMLINB][TABSIZE];
int8_t* IMLIGEHL[IMLINB] = {0};

// about the skeleton histories: see CBP4
#define YNB 15
int Ym[SNB] = {2, 4, 6, 9, 12, 16, 20, 24, 29, 34, 39, 44, 50, 56, 63};

int8_t YGEHLA[SNB][TABSIZE];
int8_t* YGEHL[SNB] = {0};

long long YHA = 0;
int LastBR[8] = {0};

// about the IMLI in Micro 2015
#define INB 5
int Im[SNB] = {16, 19, 23, 29, 35};

int8_t IGEHLA[SNB][TABSIZE];
int8_t* IGEHL[SNB] = {0};

long long IMLIcount = 0;

// corresponds to IMLI-OH in Micro 2015
long long futurelocal;

int8_t PAST[64];
#define HISTTABLESIZE 65536
int8_t histtable[HISTTABLESIZE];

#define FNB 5
int Fm[FNB] = {2, 4, 7, 11, 17};

int8_t fGEHLA[FNB][TABSIZE];

int8_t* fGEHL[FNB];

// inherited from CBP4: usefulness ?
#define BNB 10
int Bm[BNB] = {2, 4, 6, 9, 12, 16, 20, 24, 29, 34};

int8_t BGEHLA[BNB][TABSIZE];
int8_t* BGEHL[BNB] = {0};

// close targets
#define CNB 5
int Cm[SNB] = {4, 8, 12, 20, 32};

int8_t CGEHLA[CNB][TABSIZE];
int8_t* CGEHL[CNB] = {0};

long long CHIST = 0;

// more distant targets
#define RNB 5
int Rm[RNB] = {4, 8, 12, 20, 32};

int8_t RGEHLA[RNB][TABSIZE];
int8_t* RGEHL[RNB] = {0};

long long RHIST = 0;

int PERCSUM;
int Pupdatethreshold[(1 << LOGSIZE)] = {0};

#define INDUPD (PC & ((1 << LOGSIZE) - 1))
int updatethreshold;

// the GEHL predictor
#define MAXNHISTGEHL 209  // inherited from CBP4
#ifndef LOGGEHL
// base 2 logarithm of number of entries  on each GEHL  component
#define LOGGEHL (LOGTAB + 1)
#endif
#define MINSTEP 2
#define MINHISTGEHL 1
static int8_t GEHL[1 << LOGGEHL][MAXNHISTGEHL + 1];  // GEHL tables
int mgehl[MAXNHISTGEHL + 1] = {0};                   // GEHL history lengths
int GEHLINDEX[MAXNHISTGEHL + 1] = {0};

// The MACRHSP inspired predictor
#define MAXNRHSP 80  // inherited from CBP4
#define LOGRHSP LOGGEHL
static int8_t RHSP[1 << LOGRHSP][MAXNRHSP + 1];  // RHSP tables
int mrhsp[MAXNRHSP + 1] = {0};                   // RHSP history lengths
int RHSPINDEX[MAXNRHSP + 1] = {0};

int SUMRHSP;

#define PERCWIDTH 8

// local history management

long long BHIST = 0;
uint64_t lastaddr = 0;
long long P_phist = 0;
long long GHIST = 0;

int SUMGEHL;

// Another table

#define LOGBIASFULL LOGTAB
int8_t BiasFull[(1 << LOGBIASFULL)] = {0};

#define INDBIASFULL (((PC << 4) ^ ((TypeFirstSum) + ((COPRED + (PRED << 1)) << 2))) & ((1 << (LOGBIASFULL)) - 1))

#define LOGBIAS LOGTAB
int8_t Bias[(1 << LOGBIAS)] = {0};

#define INDBIAS (((PC << 1) ^ PRED) & ((1 << (LOGBIAS)) - 1))

#define LOGBIASCOLT (LOGTAB)
int8_t BiasColt[TABSIZE] = {0};

#define INDBIASCOLT                                                                                          \
  (((PC << 7) ^ PRED ^                                                                                       \
    ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ (predtaken[4] << 4) ^ \
      (predtaken[5] << 5))                                                                                   \
     << 1)) &                                                                                                \
   ((1 << (LOGBIASCOLT)) - 1))

// variables and tables for computing intermediate predictions
int CTR[NPRED];
int VAL;

// variables for the finalr
int indexfinal;
int LFINAL;
int Cupdatethreshold;
// #define LOGSIZEFINAL LOGTAB
// #define MAXSIZEFINAL  TABSIZE
// #define SIZEFINAL  (1<<(LOGSIZEFINAL))
int8_t GFINAL[TABSIZE] = {0};
int8_t GFINALCOLT[TABSIZE] = {0};

int indexfinalcolt;
// end finalr

int FirstSum;
int FirstThreshold;
int TypeFirstSum;
#define INDFIRST                                                                                   \
  (((PC << 6) ^ ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ \
                  (predtaken[4] << 4) ^ (predtaken[5] << 5)))) &                                   \
   ((1 << (LOGTAB)) - 1))
int8_t FirstBIAS[(1 << LOGTAB)];
int8_t TBias0[((1 << LOGTAB))] = {0};

#define INDBIAS0 (((PC << 7) ^ CTR[0]) & ((1 << LOGTAB) - 1))
int8_t TBias1[((1 << LOGTAB))] = {0};

#define INDBIAS1 (((PC << 7) ^ CTR[1]) & ((1 << LOGTAB) - 1))
int8_t TBias2[((1 << LOGTAB))] = {0};

#define INDBIAS2 (((PC << 7) ^ CTR[2]) & ((1 << LOGTAB) - 1))
int8_t TBias3[((1 << LOGTAB))] = {0};

#define INDBIAS3 (((PC << 7) ^ CTR[3]) & ((1 << LOGTAB) - 1))
int8_t TBias4[((1 << LOGTAB))] = {0};

#define INDBIAS4 (((PC << 7) ^ CTR[4]) & ((1 << LOGTAB) - 1))
int8_t TBias5[((1 << LOGTAB))] = {0};

#define INDBIAS5 (((PC << 3) ^ CTR[5]) & ((1 << LOGTAB) - 1))

int8_t SB0[((1 << LOGTAB))] = {0};

#define INDSB0                                                                                               \
  (((PC << 13) ^                                                                                             \
    ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ (predtaken[4] << 4) ^ \
      (predtaken[5] << 5))                                                                                   \
     << 7) ^                                                                                                 \
    CTR[0]) &                                                                                                \
   ((1 << LOGTAB) - 1))
int8_t SB1[((1 << LOGTAB))] = {0};

#define INDSB1                                                                                               \
  (((PC << 13) ^                                                                                             \
    ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ (predtaken[4] << 4) ^ \
      (predtaken[5] << 5))                                                                                   \
     << 7) ^                                                                                                 \
    CTR[1]) &                                                                                                \
   ((1 << LOGTAB) - 1))
int8_t SB2[((1 << LOGTAB))] = {0};

#define INDSB2                                                                                               \
  (((PC << 13) ^                                                                                             \
    ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ (predtaken[4] << 4) ^ \
      (predtaken[5] << 5))                                                                                   \
     << 7) ^                                                                                                 \
    CTR[2]) &                                                                                                \
   ((1 << LOGTAB) - 1))
int8_t SB3[((1 << LOGTAB))] = {0};

#define INDSB3                                                                                               \
  (((PC << 13) ^                                                                                             \
    ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ (predtaken[4] << 4) ^ \
      (predtaken[5] << 5))                                                                                   \
     << 7) ^                                                                                                 \
    CTR[3]) &                                                                                                \
   ((1 << LOGTAB) - 1))
int8_t SB4[((1 << LOGTAB))] = {0};

#define INDSB4                                                                                               \
  (((PC << 13) ^                                                                                             \
    ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ (predtaken[4] << 4) ^ \
      (predtaken[5] << 5))                                                                                   \
     << 7) ^                                                                                                 \
    CTR[4]) &                                                                                                \
   ((1 << LOGTAB) - 1))
int8_t SB5[((1 << LOGTAB))] = {0};

#define INDSB5                                                                                               \
  (((PC << 13) ^                                                                                             \
    ((predtaken[0] ^ (predtaken[1] << 1) ^ (predtaken[2] << 2) ^ (predtaken[3] << 3) ^ (predtaken[4] << 4) ^ \
      (predtaken[5] << 5))                                                                                   \
     << 7) ^                                                                                                 \
    CTR[5]) &                                                                                                \
   ((1 << LOGTAB) - 1))

// global history
#define HISTBUFFERLENGTH (1 << 18)
uint8_t ghist[HISTBUFFERLENGTH];
int ptghist;

// utilities for computing GEHL indices
folded_history chgehl_i[MAXNHISTGEHL + 1];
folded_history chrhsp_i[MAXNRHSP + 1];

int NGEHL;
int NRHSP;
int MAXHISTGEHL;

#define MTAGE_ASSERT(cond)                                           \
  if (!(cond)) {                                                     \
    fprintf(stderr, "file %s assert line %d\n", __FILE__, __LINE__); \
    abort();                                                         \
  }

#define DECPTR(ptr, size) \
  {                       \
    ptr--;                \
    if (ptr == (-1)) {    \
      ptr = size - 1;     \
    }                     \
  }

#define INCSAT(ctr, max) \
  {                      \
    if (ctr < (max))     \
      ctr++;             \
  }

#define DECSAT(ctr, min) \
  {                      \
    if (ctr > (min))     \
      ctr--;             \
  }

// for updating up-down saturating counters
bool ctrupdate(int8_t& ctr, bool inc, int nbits) {
  MTAGE_ASSERT(nbits <= 8);
  int ctrmin = -(1 << (nbits - 1));
  int ctrmax = -ctrmin - 1;
  bool issat = (ctr == ctrmax) || (ctr == ctrmin);
  if (inc) {
    INCSAT(ctr, ctrmax);
  } else {
    DECSAT(ctr, ctrmin);
  }
  return issat && ((ctr == ctrmax) || (ctr == ctrmin));
}

void path_history::init(int hlen) {
  hlength = hlen;
  h = new unsigned[hlen];
  for (int i = 0; i < hlength; i++) {
    h[i] = 0;
  }
  ptr = 0;
}

void path_history::insert(unsigned val) {
  DECPTR(ptr, hlength);
  h[ptr] = val;
}

unsigned& path_history::operator[](int n) {
  MTAGE_ASSERT((n >= 0) && (n < hlength));
  int k = ptr + n;
  if (k >= hlength) {
    k -= hlength;
  }
  MTAGE_ASSERT((k >= 0) && (k < hlength));
  return h[k];
}

compressed_history::compressed_history() {
  reset();
}

void compressed_history::reset() {
  comp = 0;  // must be consistent with path_history::reset()
}

void compressed_history::init(int original_length, int compressed_length, int injected_bits) {
  olength = original_length;
  clength = compressed_length;
  nbits = injected_bits;
  outpoint = olength % clength;
  MTAGE_ASSERT(clength < 32);
  MTAGE_ASSERT(nbits <= clength);
  mask1 = (1 << clength) - 1;
  mask2 = (1 << nbits) - 1;
  reset();
}

void compressed_history::rotateleft(unsigned& x, int m) {
  MTAGE_ASSERT(m < clength);
  MTAGE_ASSERT((x >> clength) == 0);
  unsigned y = x >> (clength - m);
  x = (x << m) | y;
  x &= mask1;
}

void compressed_history::update(path_history& ph) {
  rotateleft(comp, 1);
  unsigned inbits = ph[0] & mask2;
  unsigned outbits = ph[olength] & mask2;
  rotateleft(outbits, outpoint);
  comp ^= inbits ^ outbits;
}

coltentry::coltentry() {
  for (int i = 0; i < (1 << npred); i++) {
    c[i] = ((i >> (npred - 1)) & 1) ? 1 : -2;
  }
}

int8_t& coltentry::ctr(bool predtaken[NPRED]) {
  int v = 0;
  for (int i = 0; i < npred; i++) {
    v = (v << 1) | ((predtaken[i]) ? 1 : 0);
  }
  return c[v];
}

int8_t& colt::ctr(uint64_t pc, bool predtaken[NPRED]) {
  int i = pc & ((1 << LOGCOLT) - 1);
  return c[i].ctr(predtaken);
}

bool colt::predict(uint64_t pc, bool predtaken[NPRED]) {
  return (ctr(pc, predtaken) >= 0);
}

void colt::update(uint64_t pc, bool predtaken[NPRED], bool taken) {
  ctrupdate(ctr(pc, predtaken), taken, COLTBITS);
}

bftable::bftable() {
  for (int i = 0; i < BFTSIZE; i++) {
    freq[i] = 0;
  }
}

int& bftable::getfreq(uint64_t pc) {
  int i = pc % BFTSIZE;
  MTAGE_ASSERT((i >= 0) && (i < BFTSIZE));
  return freq[i];
}

void subpath::init(int ng, int hist[], int logg, int tagbits, int pathbits, int hp) {
  MTAGE_ASSERT(ng > 0);
  numg = ng;
  ph.init(hist[numg - 1] + 1);
  chg = new compressed_history[numg];
  chgg = new compressed_history[numg];
  cht = new compressed_history[numg];
  chtt = new compressed_history[numg];
  int ghlen = 0;
  for (int i = numg - 1; i >= 0; i--) {
    ghlen = (ghlen < hist[numg - 1 - i]) ? hist[numg - 1 - i] : ghlen + 1;
    chg[i].init(ghlen, logg, pathbits);
    chgg[i].init(ghlen, logg - hp, pathbits);
    cht[i].init(ghlen, tagbits, pathbits);
    chtt[i].init(ghlen, tagbits - 1, pathbits);
  }
}

void subpath::init(int ng, int minhist, int maxhist, int logg, int tagbits, int pathbits, int hp) {
  int* h = new int[ng];
  for (int i = 0; i < ng; i++) {
    h[i] = minhist * pow((double)maxhist / minhist, (double)i / (ng - 1));
  }
  init(ng, h, logg, tagbits, pathbits, hp);
}

void subpath::update(uint64_t targetpc, bool taken) {
  ph.insert((targetpc << 1) | taken);
  for (int i = 0; i < numg; i++) {
    chg[i].update(ph);
    chgg[i].update(ph);
    cht[i].update(ph);
    chtt[i].update(ph);
  }
}

unsigned subpath::cg(int bank) {
  MTAGE_ASSERT((bank >= 0) && (bank < numg));
  return chg[bank].comp;
}

unsigned subpath::cgg(int bank) {
  MTAGE_ASSERT((bank >= 0) && (bank < numg));
  return chgg[bank].comp << (chg[bank].clength - chgg[bank].clength);
}

unsigned subpath::ct(int bank) {
  MTAGE_ASSERT((bank >= 0) && (bank < numg));
  return cht[bank].comp;
}

unsigned subpath::ctt(int bank) {
  MTAGE_ASSERT((bank >= 0) && (bank < numg));
  return chtt[bank].comp << (cht[bank].clength - chtt[bank].clength);
}

spectrum::spectrum() {
  size = 0;
  p = NULL;
}

void spectrum::init(int sz, int ng, int minhist, int maxhist, int logg, int tagbits, int pathbits, int hp) {
  size = sz;
  p = new subpath[size];
  for (int i = 0; i < size; i++) {
    p[i].init(ng, minhist, maxhist, logg, tagbits, pathbits, hp);
  }
}

void freqbins::init(int nb) {
  nbins = nb;
  maxfreq = 0;
}

int freqbins::find(int bfreq) {
  // find in which frequency bin the input branch frequency falls
  MTAGE_ASSERT(bfreq >= 0);
  int b = -1;
  int f = maxfreq;
  for (int i = 0; i < nbins; i++) {
    f = f >> FRATIOBITS;
    if (bfreq >= f) {
      b = i;
      break;
    }
  }
  if (b < 0) {
    b = nbins - 1;
  }
  return b;
}

void freqbins::update(int bfreq) {
  if (bfreq > maxfreq) {
    MTAGE_ASSERT(bfreq == (maxfreq + 1));
    maxfreq = bfreq;
  }
}

gentry::gentry() {
  ctr = 0;
  tag = 0;
  u = 0;
}

tage::tage() {
  b = NULL;
  g = NULL;
  gi = NULL;
  postp = NULL;
  nmisp = 0;
}

void tage::init(const char* nm, int ng, int logb, int logg, int tagb, int ctrb, int ppb, int ru, int caph) {
  // NUMG = number of tagged tables
  // LOGB = log2 of the number of entries of the tagless (bimodal) table
  // LOGG = log2 of the number of entries of each tagged table
  // MAXHIST = maximum path length ("rightmost" tagged table), in branches
  // MINHIST = minimum path length ("leftmost" tagged table), in branches
  // HASHPARAM = parameter used in the hash functions (may need to be changed with
  // predictor size) RAMPUP = ramp-up period in mispredictions (should be kept
  // roughly proportional to predictor size) TAGBITS = tag width in bits CTRBITS =
  // width of the taken/not-taken counters in the tagless (bimodal) and tagged
  // tables PATHBITS = number of per-branch address bits injected in the path
  // hashing POSTPBITS = width of the taken/not-taken counters in the
  // post-predictor POSTPEXTRA = number of secondary hits feeding the
  // post-predictor ALLOCFAILMAX : used for clearing u bits (cf. ISL_TAGE, Andre
  // Seznec, MICRO 2011) MAXALLOC = maximum number of entries stolen upon a
  // misprediction (cf. ISL_TAGE) CAPHIST = path length beyond which aggressive
  // update (ramp-up) is made sligtly less aggressive
  if (MTAGE_REALISTIC_SC_40K) {
    MTAGE_ASSERT(!MTAGE_REALISTIC_SC_100K);
    npred = 1;
    P0_NUMG = 10;
    P0_LOGB = 9;
    P0_LOGG = 9;
    P0_MAXHIST = 359;
    P0_MINHIST = 4;
    P0_HASHPARAM = 3;
    P0_RAMPUP = 10000;
    TAGBITS = 12;
  }
  if (MTAGE_REALISTIC_SC_100K) {
    MTAGE_ASSERT(!MTAGE_REALISTIC_SC_40K);
    npred = 1;
    P0_NUMG = 12;
    P0_LOGB = 10;
    P0_LOGG = 10;
    P0_MAXHIST = 1000;
    P0_MINHIST = 4;
    P0_HASHPARAM = 3;
    P0_RAMPUP = 100000;
    TAGBITS = 12;
  }

  MTAGE_ASSERT(ng > 1);
  MTAGE_ASSERT(logb < 30);
  MTAGE_ASSERT(logg < 30);
  name = nm;
  numg = ng;
  bsize = 1 << logb;
  gsize = 1 << logg;
  tagbits = tagb;
  ctrbits = ctrb;
  postpbits = ppb;
  postpsize = 1 << (2 * ctrbits + 1);
  b = new int8_t[bsize];
  for (int i = 0; i < bsize; i++) {
    b[i] = 0;
  }
  g = new gentry*[numg];
  for (int i = 0; i < numg; i++) {
    g[i] = new gentry[gsize];
  }
  gi = new int[numg];
  postp = new int8_t[postpsize];
  for (int i = 0; i < postpsize; i++) {
    postp[i] = -(((i >> 1) >> (ctrbits - 1)) & 1);
  }
  allocfail = 0;
  rampup = ru;
  caphist = caph;
}

int tage::bindex(uint64_t pc) {
  return pc & (bsize - 1);
}

int tage::gindex(uint64_t pc, subpath& p, int bank) {
  return (pc ^ p.cg(bank) ^ p.cgg(bank)) & (gsize - 1);
}

int tage::gtag(uint64_t pc, subpath& p, int bank) {
  return (pc ^ p.ct(bank) ^ p.ctt(bank)) & ((1 << tagbits) - 1);
}

int tage::postp_index() {
  // post predictor index function
  int ctr[2];
  for (int i = 0; i < 2; i++) {
    ctr[i] = (i < (int)hit.size()) ? getg(hit[i]).ctr : b[bi];
  }
  int v = 0;
  for (int i = 1; i >= 0; i--) {
    v = (v << ctrbits) | (ctr[i] & (((1 << ctrbits) - 1)));
  }

  int u0 = (hit.size() > 0) ? (getg(hit[0]).u > 0) : 1;
  v = (v << 1) | u0;
  v &= postpsize - 1;
  VAL = v;

  return v;
}

gentry& tage::getg(int i) {
  MTAGE_ASSERT((i >= 0) && (i < numg));
  return g[i][gi[i]];
}

bool tage::condbr_predict(uint64_t pc, subpath& p) {
  hit.clear();
  bi = bindex(pc);
  for (int i = 0; i < numg; i++) {
    gi[i] = gindex(pc, p, i);
    if (g[i][gi[i]].tag == gtag(pc, p, i)) {
      hit.push_back(i);
    }
  }

  predtaken = (hit.size() > 0) ? (getg(hit[0]).ctr >= 0) : (b[bi] >= 0);
  altpredtaken = (hit.size() > 1) ? (getg(hit[1]).ctr >= 0) : (b[bi] >= 0);
  ppi = postp_index();
  MTAGE_ASSERT(ppi < postpsize);
  postpredtaken = (postp[ppi] >= 0);
  return postpredtaken;
}

void tage::uclear() {
  for (int i = 0; i < numg; i++) {
    for (int j = 0; j < gsize; j++) {
      if (g[i][j].u)
        g[i][j].u--;
    }
  }
}

void tage::galloc(int i, uint64_t pc, bool taken, subpath& p) {
  getg(i).tag = gtag(pc, p, i);
  getg(i).ctr = (taken) ? 0 : -1;
  getg(i).u = 0;
}

void tage::aggressive_update(uint64_t pc, bool taken, subpath& p) {
  // update policy used during ramp up
  bool allsat = true;

  // AS: slightly improved from CBP4
  if (hit.size() > 0) {
    bool inter = (getg(hit[0]).ctr >= 0);
    allsat &= ctrupdate(getg(hit[0]).ctr, taken, ctrbits);
    int start = 1;
    bool Done = false;
    bool STOP = false;

    if (getg(hit[0]).u == 0) {
      if (hit.size() > 1) {
        if ((getg(hit[1]).ctr >= 0) != inter)
          STOP = true;

        start = 2;
        allsat &= ctrupdate(getg(hit[1]).ctr, taken, ctrbits);
      } else {
        Done = true;
        allsat &= ctrupdate(b[bi], taken, ctrbits);
      }
    }

    if (!STOP)
      for (int i = start; i < (int)hit.size(); i++) {
        if ((getg(hit[i]).ctr >= 0) == inter)
          allsat &= ctrupdate(getg(hit[i]).ctr, taken, ctrbits);
        else {
          Done = true;
          break;
        }
      }
    if (!Done)
      if ((b[bi] >= 0) == inter)
        allsat &= ctrupdate(b[bi], taken, ctrbits);
  } else {
    ctrupdate(b[bi], taken, ctrbits);
  }

  int i = (hit.size() > 0) ? hit[0] : numg;
  while (--i >= 0) {
    if (getg(i).u != 0)
      continue;
    if (!allsat || (p.chg[i].olength <= caphist)) {
      galloc(i, pc, taken, p);
    }
  }
}

void tage::careful_update(uint64_t pc, bool taken, subpath& p) {
  // update policy devised by Andre Seznec for the ISL-TAGE predictor (MICRO
  // 2011)

  if (hit.size() > 0) {
    ctrupdate(getg(hit[0]).ctr, taken, ctrbits);

    if (getg(hit[0]).u == 0) {
      if (hit.size() > 1) {
        ctrupdate(getg(hit[1]).ctr, taken, ctrbits);
      } else {
        ctrupdate(b[bi], taken, ctrbits);
      }
    }
  } else {
    ctrupdate(b[bi], taken, ctrbits);
  }

  if (mispred) {
    int nalloc = 0;
    int i = (hit.size() > 0) ? hit[0] : numg;
    while (--i >= 0) {
      if (getg(i).u == 0) {
        galloc(i, pc, taken, p);
        DECSAT(allocfail, 0);
        i--;
        nalloc++;
        if (nalloc == MAXALLOC)
          break;
      } else {
        INCSAT(allocfail, ALLOCFAILMAX);
        if (allocfail == ALLOCFAILMAX) {
          uclear();
        }
      }
    }
  }
}

bool tage::condbr_update(uint64_t pc, bool taken, subpath& p) {
  mispred = (postpredtaken != taken);

  if (mispred) {
    nmisp++;
  }

  if (nmisp < rampup) {
    aggressive_update(pc, taken, p);
  } else {
    careful_update(pc, taken, p);
  }

  // update u bit (see TAGE, JILP 2006)
  if (predtaken != altpredtaken) {
    if (altpredtaken != predtaken)
      if (predtaken == taken)
        ctrupdate(getg(hit[0]).u, true, 3);
  }

  // update post pred
  ctrupdate(postp[ppi], taken, postpbits);

  return mispred;
}

void tage::printconfig(subpath& p) {
  printf("%s path lengths: ", name.c_str());
  for (int i = numg - 1; i >= 0; i--) {
    printf("%d ", p.chg[i].olength);
  }
  printf("\n");
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void folded_history::init(int original_length, int compressed_length, int N) {
  comp = 0;
  OLENGTH = original_length;
  CLENGTH = compressed_length;
  OUTPOINT = OLENGTH % CLENGTH;
}

void folded_history::update(uint8_t* h, int PT) {
  comp = (comp << 1) ^ h[PT & (HISTBUFFERLENGTH - 1)];
  comp ^= h[(PT + OLENGTH) & (HISTBUFFERLENGTH - 1)] << OUTPOINT;
  comp ^= (comp >> CLENGTH);
  comp = (comp) & ((1 << CLENGTH) - 1);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

MTAGE::MTAGE(void) {
  // NUMG = number of tagged tables
  // LOGB = log2 of the number of entries of the tagless (bimodal) table
  // LOGG = log2 of the number of entries of each tagged table
  // MAXHIST = maximum path length ("rightmost" tagged table), in branches
  // MINHIST = minimum path length ("leftmost" tagged table), in branches
  // HASHPARAM = parameter used in the hash functions (may need to be changed with
  // predictor size) RAMPUP = ramp-up period in mispredictions (should be kept
  // roughly proportional to predictor size) TAGBITS = tag width in bits CTRBITS =
  // width of the taken/not-taken counters in the tagless (bimodal) and tagged
  // tables PATHBITS = number of per-branch address bits injected in the path
  // hashing POSTPBITS = width of the taken/not-taken counters in the
  // post-predictor POSTPEXTRA = number of secondary hits feeding the
  // post-predictor ALLOCFAILMAX : used for clearing u bits (cf. ISL_TAGE, Andre
  // Seznec, MICRO 2011) MAXALLOC = maximum number of entries stolen upon a
  // misprediction (cf. ISL_TAGE) CAPHIST = path length beyond which aggressive
  // update (ramp-up) is made sligtly less aggressive
  if (MTAGE_REALISTIC_SC_40K) {
    MTAGE_ASSERT(!MTAGE_REALISTIC_SC_100K);
    npred = 1;
    P0_NUMG = 10;
    P0_LOGB = 9;
    P0_LOGG = 9;
    P0_MAXHIST = 359;
    P0_MINHIST = 4;
    P0_HASHPARAM = 3;
    P0_RAMPUP = 10000;
    TAGBITS = 12;
  }
  if (MTAGE_REALISTIC_SC_100K) {
    MTAGE_ASSERT(!MTAGE_REALISTIC_SC_40K);
    npred = 1;
    P0_NUMG = 12;
    P0_LOGB = 10;
    P0_LOGG = 10;
    P0_MAXHIST = 1000;
    P0_MINHIST = 4;
    P0_HASHPARAM = 3;
    P0_RAMPUP = 100000;
    TAGBITS = 12;
  }

  sp[0].init(P0_SPSIZE, P0_NUMG, P0_MINHIST, P0_MAXHIST, P0_LOGG, TAGBITS, PATHBITS, P0_HASHPARAM);
  sp[1].init(P1_SPSIZE, P1_NUMG, P1_MINHIST, P1_MAXHIST, P1_LOGG, TAGBITS, PATHBITS, P1_HASHPARAM);
  sp[2].init(P2_SPSIZE, P2_NUMG, P2_MINHIST, P2_MAXHIST, P2_LOGG, TAGBITS, PATHBITS, P2_HASHPARAM);
  sp[3].init(P3_SPSIZE, P3_NUMG, P3_MINHIST, P3_MAXHIST, P3_LOGG, TAGBITS, PATHBITS, P3_HASHPARAM);
  sp[4].init(P4_SPSIZE, P4_NUMG, P4_MINHIST, P4_MAXHIST, P4_LOGG, TAGBITS, PATHBITS, P4_HASHPARAM);
  sp[5].init(P5_SPSIZE, P5_NUMG, P5_MINHIST, P5_MAXHIST, P5_LOGG, TAGBITS, PATHBITS, P5_HASHPARAM);

  pred[0].init("G", P0_NUMG, P0_LOGB, P0_LOGG, TAGBITS, CTRBITS, POSTPBITS, P0_RAMPUP, CAPHIST);
  pred[1].init("A", P1_NUMG, P1_LOGB, P1_LOGG, TAGBITS, CTRBITS, POSTPBITS, P1_RAMPUP, CAPHIST);
  pred[2].init("S", P2_NUMG, P2_LOGB, P2_LOGG, TAGBITS, CTRBITS, POSTPBITS, P2_RAMPUP, CAPHIST);
  pred[3].init("s", P3_NUMG, P3_LOGB, P3_LOGG, TAGBITS, CTRBITS, POSTPBITS, P3_RAMPUP, CAPHIST);
  pred[4].init("F", P4_NUMG, P4_LOGB, P4_LOGG, TAGBITS, CTRBITS, POSTPBITS, P4_RAMPUP, CAPHIST);

  pred[5].init("g", P5_NUMG, P5_LOGB, P5_LOGG, TAGBITS, CTRBITS, POSTPBITS, P5_RAMPUP, CAPHIST);

  bfreq.init(P4_SPSIZE);  // number of frequency bins = P4 spectrum size

  initSC();
}

bool MTAGE::GetPrediction(UINT64 PC, int* bp_confidence) {
  *bp_confidence = 0;
  return GetPrediction(PC);
};

bool MTAGE::GetPrediction(uint64_t PC) {
  subp[0] = &sp[0].p[0];                             // global path
  subp[1] = &sp[1].p[PC % P1_SPSIZE];                // per-address subpath
  subp[2] = &sp[2].p[(PC >> P2_PARAM) % P2_SPSIZE];  // per-set subpath
  subp[3] = &sp[3].p[(PC >> P3_PARAM) % P3_SPSIZE];  // another per-set subpath
  int f = bfreq.find(bft.getfreq(PC));
  MTAGE_ASSERT((f >= 0) && (f < P4_SPSIZE));
  subp[4] = &sp[4].p[f];  // frequency subpath
  subp[5] = &sp[5].p[0];  // global backward path

  for (int i = 0; i < npred; i++) {
    predtaken[i] = pred[i].condbr_predict(PC, *subp[i]);
    CTR[i] = VAL;  // 7 bits of information: the two longest hitting counters +
                   // the u bit
  }

  // The TAGE combiner
  // the neural combination
  int ctr = FirstBIAS[INDFIRST];
  FirstSum = (2 * ctr + 1);
  ctr = TBias0[INDBIAS0];
  FirstSum += (2 * ctr + 1);
  ctr = TBias1[INDBIAS1];
  FirstSum += (2 * ctr + 1);
  ctr = TBias2[INDBIAS2];
  FirstSum += (2 * ctr + 1);
  ctr = TBias3[INDBIAS3];
  FirstSum += (2 * ctr + 1);
  ctr = TBias4[INDBIAS4];
  FirstSum += (2 * ctr + 1);
  ctr = TBias5[INDBIAS5];
  FirstSum += (2 * ctr + 1);
  ctr = SB0[INDSB0];
  FirstSum += (2 * ctr + 1);
  ctr = SB1[INDSB1];
  FirstSum += (2 * ctr + 1);
  ctr = SB2[INDSB2];
  FirstSum += (2 * ctr + 1);
  ctr = SB3[INDSB3];
  FirstSum += (2 * ctr + 1);
  ctr = SB4[INDSB4];
  FirstSum += (2 * ctr + 1);
  ctr = SB5[INDSB5];
  FirstSum += (2 * ctr + 1);

  pred_inter = (FirstSum >= 0);
  // Extracting the confidence level
  if (abs(FirstSum) < FirstThreshold / 4)
    TypeFirstSum = 0;
  else if (abs(FirstSum) < FirstThreshold / 2)
    TypeFirstSum = 1;
  else if (abs(FirstSum) < FirstThreshold)
    TypeFirstSum = 2;
  else
    TypeFirstSum = 3;
  // the COLT predicition
  COPRED = co.predict(PC, predtaken);

  // the statistical correlator
  predSC = SCpredict(PC, pred_inter);
  bool finalpred = predSC;

  finalpred = FinalSCpredict(PC, pred_inter);
  return finalpred;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void MTAGE::UpdatePredictor(uint64_t PC, OpType OPTYPE, bool resolveDir, bool predDir, uint64_t branchTarget) {
  if (PERFECT_BP && !subp[0])
    GetPrediction(PC);

  XX += (predtaken[0] != resolveDir);
  YY += (pred_inter != resolveDir);
  ZZ += (COPRED != resolveDir);
  TT += (predSC != resolveDir);

  // the TAGE stage
  uint64_t ForUpdate = (resolveDir) ? (branchTarget << 1) ^ PC : PC;
  if (npred == 1) {
    pred[0].condbr_update(PC, resolveDir, *subp[0]);
    subp[0]->update(ForUpdate, resolveDir);
  } else {
    for (int i = 0; i < NPRED - 1; i++) {
      pred[i].condbr_update(PC, resolveDir, *subp[i]);
      subp[i]->update(ForUpdate, resolveDir);
    }
    pred[NPRED - 1].condbr_update(PC, resolveDir, *subp[NPRED - 1]);
    if (branchTarget < PC)
      subp[NPRED - 1]->update(ForUpdate, ((branchTarget < PC) & resolveDir));
  }
  bfreq.update(bft.getfreq(PC));
  bft.getfreq(PC)++;

  // update of the TAGE combiner

  if ((abs(FirstSum) < FirstThreshold) || (pred_inter != resolveDir)) {
    if (pred_inter != resolveDir) {
      FirstThreshold += 1;
    } else {
      FirstThreshold -= 1;
    }
    ctrupdate(FirstBIAS[INDFIRST], resolveDir, PERCWIDTH);
    ctrupdate(TBias0[INDBIAS0], resolveDir, PERCWIDTH);
    ctrupdate(TBias1[INDBIAS1], resolveDir, PERCWIDTH);
    ctrupdate(TBias2[INDBIAS1], resolveDir, PERCWIDTH);
    ctrupdate(TBias3[INDBIAS3], resolveDir, PERCWIDTH);
    ctrupdate(TBias4[INDBIAS4], resolveDir, PERCWIDTH);
    ctrupdate(TBias5[INDBIAS5], resolveDir, PERCWIDTH);
    ctrupdate(SB0[INDSB0], resolveDir, PERCWIDTH);
    ctrupdate(SB1[INDSB1], resolveDir, PERCWIDTH);
    ctrupdate(SB2[INDSB1], resolveDir, PERCWIDTH);
    ctrupdate(SB3[INDSB3], resolveDir, PERCWIDTH);
    ctrupdate(SB4[INDSB4], resolveDir, PERCWIDTH);
    ctrupdate(SB5[INDSB5], resolveDir, PERCWIDTH);
  }
  co.update(PC, predtaken, resolveDir);
  // end of the TAGE combiner

  // the statistical corrector

  UpdateSC(PC, resolveDir, pred_inter);

  // The final stage
  UpdateFinalSC(PC, resolveDir);

  HistoryUpdate(PC, 1, resolveDir, branchTarget, ptghist, chgehl_i, chrhsp_i);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
void MTAGE::TrackOtherInst(uint64_t PC, OpType opType, bool taken, uint64_t branchTarget) {
  // also update the global path with unconditional branches
  uint64_t PC0 = (PC ^ (PC >> 2));
  uint64_t branchTarget0 = (branchTarget ^ (branchTarget >> 2));
  uint64_t ForUpdate = (branchTarget0 << 1) ^ PC0;
  sp[0].p[0].update(ForUpdate, true);
  sp[5].p[0].update(ForUpdate, true);
  HistoryUpdate(PC, 0, true, branchTarget, ptghist, chgehl_i, chrhsp_i);
}

uns8 MTAGE::IsFull(void) {
  return 0;
}

void MTAGE::initSC() {
  NRHSP = 80;
  NGEHL = 209;
  MAXHISTGEHL = 1393;

  for (int i = 0; i < HISTBUFFERLENGTH; i++)
    ghist[0] = 0;

  ptghist = 0;

  // GEHL initialization
  mgehl[0] = 0;
  mgehl[1] = MINHISTGEHL;
  mgehl[NGEHL] = MAXHISTGEHL;

  for (int i = 2; i <= NGEHL; i++)
    mgehl[i] = (int)(((double)MINHISTGEHL *
                      pow((double)MAXHISTGEHL / (double)MINHISTGEHL, (double)(i - 1) / (double)(NGEHL - 1))) +
                     0.5);

  // just guarantee that all history lengths are distinct

  for (int i = 1; i <= NGEHL; i++)
    if (mgehl[i] <= mgehl[i - 1] + MINSTEP)
      mgehl[i] = mgehl[i - 1] + MINSTEP;

  for (int i = 1; i <= NGEHL; i++)
    chgehl_i[i].init(mgehl[i], LOGGEHL, ((i & 1)) ? i : 1);

  // initialization of GEHL tables

  for (int j = 0; j < (1 << LOGGEHL); j++)
    for (int i = 0; i <= NGEHL; i++)
      GEHL[j][i] = (i & 1) ? -4 : 3;

  // RHSP initialization

  for (int i = 1; i <= NRHSP; i++)
    mrhsp[i] = 6 * i;

  for (int i = 1; i <= NRHSP; i++)
    chrhsp_i[i].init(mrhsp[i], LOGRHSP, ((i & 1)) ? i : 1);

  // initialization of RHSP tables

  for (int j = 0; j < (1 << LOGRHSP); j++)
    for (int i = 0; i <= NRHSP; i++)
      RHSP[j][i] = (i & 1) ? -4 : 3;

  updatethreshold = 100;
  Cupdatethreshold = 11;

  for (int i = 0; i < (1 << LOGSIZE); i++)
    Pupdatethreshold[i] = 0;

  for (int i = 0; i < LNB; i++)
    LGEHL[i] = &LGEHLA[i][0];
  for (int i = 0; i < LINB; i++)
    LIGEHL[i] = &LIGEHLA[i][0];
  for (int i = 0; i < SNB; i++)
    SGEHL[i] = &SGEHLA[i][0];

  for (int i = 0; i < QNB; i++)
    QGEHL[i] = &QGEHLA[i][0];

  for (int i = 0; i < TNB; i++)
    TGEHL[i] = &TGEHLA[i][0];
  for (int i = 0; i < IMLINB; i++)
    IMLIGEHL[i] = &IMLIGEHLA[i][0];

  for (int i = 0; i < BNB; i++)
    BGEHL[i] = &BGEHLA[i][0];

  for (int i = 0; i < YNB; i++)
    YGEHL[i] = &YGEHLA[i][0];
  for (int i = 0; i < INB; i++)
    IGEHL[i] = &IGEHLA[i][0];

  for (int i = 0; i < FNB; i++)
    fGEHL[i] = &fGEHLA[i][0];

#ifdef LOOPMTAGE
  ltable = new lentry[1 << (LOGL)];
#endif
#ifdef LOOPMTAGE
  LVALID = false;
  WITHLOOP = -1;
#endif
  for (int i = 0; i < LNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        LGEHL[i][j] = -1;
      }
    }

  for (int i = 0; i < SNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        SGEHL[i][j] = -1;
      }
    }

  for (int i = 0; i < QNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        QGEHL[i][j] = -1;
      }
    }
  for (int i = 0; i < LINB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        LIGEHL[i][j] = -1;
      }
    }

  for (int i = 0; i < TNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        TGEHL[i][j] = -1;
      }
    }
  for (int i = 0; i < IMLINB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        IMLIGEHL[i][j] = -1;
      }
    }

  for (int i = 0; i < BNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        BGEHL[i][j] = -1;
      }
    }

  for (int i = 0; i < YNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        YGEHL[i][j] = -1;
      }
    }
  for (int i = 0; i < INB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        IGEHL[i][j] = -1;
      }
    }
  for (int i = 0; i < FNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        fGEHL[i][j] = -1;
      }
    }
  for (int i = 0; i < CNB; i++)
    CGEHL[i] = &CGEHLA[i][0];
  for (int i = 0; i < CNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        CGEHL[i][j] = -1;
      }
    }
  for (int i = 0; i < RNB; i++)
    RGEHL[i] = &RGEHLA[i][0];
  for (int i = 0; i < RNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        RGEHL[i][j] = -1;
      }
    }

  for (int i = 0; i < QQNB; i++)
    QQGEHL[i] = &QQGEHLA[i][0];
  for (int i = 0; i < QQNB; i++)
    for (int j = 0; j < TABSIZE; j++) {
      if (j & 1) {
        QQGEHL[i][j] = -1;
      }
    }

  for (int j = 0; j < (1 << LOGBIAS); j++)
    Bias[j] = (j & 1) ? 15 : -16;

  for (int j = 0; j < (1 << LOGBIASCOLT); j++)
    BiasColt[j] = (j & 1) ? 0 : -1;

  for (int i = 0; i < (1 << LOGSIZES); i++)
    for (int j = 0; j < (SWIDTH / SPSTEP) * (1 << SPSTEP); j++) {
      if (j & 1) {
        PERCSLOC[i][j] = -1;
      }
    }

  for (int i = 0; i < (1 << LOGSIZEQ); i++)
    for (int j = 0; j < (QWIDTH / QPSTEP) * (1 << QPSTEP); j++) {
      if (j & 1) {
        PERCQLOC[i][j] = -1;
      }
    }

  for (int i = 0; i < (1 << LOGSIZEP); i++)
    for (int j = 0; j < (GWIDTH / GPSTEP) * (1 << GPSTEP); j++) {
      if (j & 1) {
        PERC[i][j] = -1;
      }
    }

  for (int i = 0; i < (1 << LOGSIZEL); i++)
    for (int j = 0; j < (LWIDTH / LPSTEP) * (1 << LPSTEP); j++) {
      if (j & 1) {
        PERCLOC[i][j] = -1;
      }
    }

  for (int i = 0; i < (1 << LOGSIZEB); i++)
    for (int j = 0; j < ((BWIDTH / BPSTEP)) * (1 << BPSTEP); j++) {
      if (j & 1) {
        PERCBACK[i][j] = -1;
      }
    }

  for (int i = 0; i < (1 << LOGSIZEY); i++)
    for (int j = 0; j < (YWIDTH / YPSTEP) * (1 << YPSTEP); j++) {
      if (j & 1) {
        PERCYHA[i][j] = -1;
      }
    }

  for (int i = 0; i < (1 << LOGSIZEP); i++)
    for (int j = 0; j < (PWIDTH / PPSTEP) * (1 << PPSTEP); j++) {
      if (j & 1) {
        PERCPATH[i][j] = -1;
      }
    }
}

void MTAGE::HistoryUpdate(uint64_t PC, uint8_t brtype, bool taken, uint64_t target, int& Y, folded_history* K,

                          folded_history* L) {
#define OPTYPE_BRANCH_COND 1
  // History skeleton
  bool V = false;

  for (int i = 0; i <= 7; i++)
    if (LastBR[i] == (int)PC)
      V = true;

  for (int i = 7; i >= 1; i--)
    LastBR[i] = LastBR[i - 1];

  LastBR[0] = PC;

  if (!V)
    YHA = (YHA << 1) ^ (taken ^ ((PC >> 5) & 1));

  // Path history
  P_phist = (P_phist << 1) ^ (taken ^ ((PC >> 5) & 1));
  IMLIhist[INDIMLI] = (IMLIhist[INDIMLI] << 1) ^ (taken ^ ((PC >> 5) & 1));

  if (brtype == OPTYPE_BRANCH_COND) {
    // local history
    L_shist[INDLOCAL] = (L_shist[INDLOCAL] << 1) + (taken);
    Q_slhist[INDQLOCAL] = (Q_slhist[INDQLOCAL] << 1) + (taken);
    S_slhist[INDSLOCAL] = (S_slhist[INDSLOCAL] << 1) + (taken);
    S_slhist[INDSLOCAL] ^= ((PC >> LOGSECLOCAL) & 15);
    T_slhist[INDTLOCAL] = (T_slhist[INDTLOCAL] << 1) + (taken);
    T_slhist[INDTLOCAL] ^= ((PC >> LOGTLOCAL) & 15);
    // global branch history
    GHIST = (GHIST << 1) + taken;

    if ((target > PC + 64) || (target < PC - 64))
      RHIST = (RHIST << 1) + taken;
    if (taken)
      if ((target > PC + 64) || (target < PC + 64))
        CHIST = (CHIST << 1) ^ (PC & 63);
  }

  // is it really useful ?
  if ((PC + 16 < lastaddr) || (PC > lastaddr + 128)) {
    BHIST = (BHIST << 1) ^ (PC & 15);
  }
  lastaddr = PC;
  // IMLI related
  if (brtype == OPTYPE_BRANCH_COND)
    if (target < PC) {
      // This branch corresponds to a loop
      if (!taken) {
        // exit of the "loop"
        IMLIcount = 0;
      }
      if (taken) {
        if (IMLIcount < ((1 << Im[0]) - 1))
          IMLIcount++;
      }
    }

#define SHIFTFUTURE 9
  // IMLI OH history, see IMLI paper at Micro 2015
  if (brtype == OPTYPE_BRANCH_COND) {
    if (target >= PC) {
      PAST[PC & 63] = histtable[(((PC ^ (PC >> 2)) << SHIFTFUTURE) + IMLIcount) & (HISTTABLESIZE - 1)];
      histtable[(((PC ^ (PC >> 2)) << SHIFTFUTURE) + IMLIcount) & (HISTTABLESIZE - 1)] = taken;
    }
  }

  int T = ((target ^ (target >> 3) ^ PC) << 1) + taken;

  int8_t DIR = (T & 127);

  // update  history
  Y--;
  ghist[Y & (HISTBUFFERLENGTH - 1)] = DIR;

  // prepare next index and tag computations
  for (int i = 1; i <= NGEHL; i++) {
    K[i].update(ghist, Y);
  }

  for (int i = 1; i <= NRHSP; i++) {
    L[i].update(ghist, Y);
  }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void MTAGE::UpdateFinalSC(uint64_t PC, bool taken) {
  bool CRES = (taken);
  {
    ctrupdate(GFINALCOLT[indexfinalcolt], CRES, 8);
    ctrupdate(GFINAL[indexfinal], CRES, 8);
    // using only the GFINAL table would result in 0.004 MPKI more
  }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool MTAGE::FinalSCpredict(uint64_t PC, bool Tpred) {
  int TypeSecondSum;
  int X = abs(LSUM);
  int Y = updatethreshold + Pupdatethreshold[INDUPD];
  if (X < Y / 4)
    TypeSecondSum = 0;
  else if (X < Y / 2)
    TypeSecondSum = 1;
  else if (X < Y)
    TypeSecondSum = 2;
  else
    TypeSecondSum = 3;
  int CLASS = (TypeSecondSum << 2) + (TypeFirstSum) + ((COPRED + (Tpred << 1) + (predSC << 2)) << 4);
  indexfinal = CLASS;
  indexfinalcolt = ((PC << 7) + CLASS) & (TABSIZE - 1);
  LFINAL = 2 * GFINAL[indexfinal] + 1;
  if (abs(2 * GFINALCOLT[indexfinalcolt] + 1) > 15)
    LFINAL = 2 * GFINALCOLT[indexfinalcolt] + 1;

  return (LFINAL >= 0);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void MTAGE::UpdateSC(uint64_t PC, bool taken, bool PRED) {
  if ((predSC != taken) || ((abs(LSUM) < updatethreshold + Pupdatethreshold[INDUPD]))) {
    if (predSC != taken) {
      updatethreshold += 1;
    } else {
      updatethreshold -= 1;
    }

    if (predSC != taken) {
      Pupdatethreshold[INDUPD] += 1;
    } else {
      Pupdatethreshold[INDUPD] -= 1;
    }

    gehlupdate(PC, taken);
    rhspupdate(PC, taken);
    ctrupdate(BiasFull[INDBIASFULL], taken, PERCWIDTH);
    ctrupdate(Bias[INDBIAS], taken, PERCWIDTH);
    ctrupdate(BiasColt[INDBIASCOLT], taken, PERCWIDTH);

    updateperc(taken, PERC[PC & ((1 << LOGSIZEG) - 1)], GHIST, GPSTEP, GWIDTH);
    updateperc(taken, PERCLOC[PC & ((1 << LOGSIZEL) - 1)], L_shist[INDLOCAL], LPSTEP, LWIDTH);
    updateperc(taken, PERCBACK[PC & ((1 << LOGSIZEB) - 1)], BHIST, BPSTEP, BWIDTH);
    updateperc(taken, PERCYHA[PC & ((1 << LOGSIZEB) - 1)], YHA, YPSTEP, YWIDTH);
    updateperc(taken, PERCPATH[PC & ((1 << LOGSIZEP) - 1)], P_phist, PPSTEP, PWIDTH);
    updateperc(taken, PERCSLOC[PC & ((1 << LOGSIZES) - 1)], S_slhist[INDSLOCAL], SPSTEP, SWIDTH);
    updateperc(taken, PERCTLOC[PC & ((1 << LOGSIZES) - 1)], T_slhist[INDTLOCAL], SPSTEP, SWIDTH);
    updateperc(taken, PERCQLOC[PC & ((1 << LOGSIZEQ) - 1)], Q_slhist[INDQLOCAL], QPSTEP, QWIDTH);

    Gupdate(PC, taken, L_shist[INDLOCAL], Lm, LGEHL, LNB, PERCWIDTH);
    // for IMLI
    Gupdate(PC, taken, (L_shist[INDLOCAL] << 16) ^ IMLIcount, LIm, LIGEHL, LINB, PERCWIDTH);
    Gupdate(PC, taken, S_slhist[INDSLOCAL], Sm, SGEHL, SNB, PERCWIDTH);
    Gupdate(PC, taken, T_slhist[INDTLOCAL], Tm, TGEHL, TNB, PERCWIDTH);
    Gupdate(PC, taken, IMLIhist[INDIMLI], IMLIm, IMLIGEHL, IMLINB, PERCWIDTH);
    Gupdate(PC, taken, Q_slhist[INDQLOCAL], Qm, QGEHL, QNB, PERCWIDTH);

    Gupdate(PC, taken, BHIST, Bm, BGEHL, BNB, PERCWIDTH);
    Gupdate(PC, taken, YHA, Ym, YGEHL, YNB, PERCWIDTH);
    Gupdate(PC, taken, (IMLIcount + (GHIST << 16)), Im, IGEHL, INB, PERCWIDTH);
    Gupdate((PC << 8), taken, futurelocal, Fm, fGEHL, FNB, PERCWIDTH);

    Gupdate(0, taken, Q_slhist[INDQLOCAL], QQm, QQGEHL, QQNB, PERCWIDTH);
    Gupdate(PC, taken, CHIST, Cm, CGEHL, CNB, PERCWIDTH);
    Gupdate(PC, taken, RHIST, Rm, RGEHL, RNB, PERCWIDTH);
  }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool MTAGE::SCpredict(uint64_t PC, bool PRED) {
  LSUM = 0;
  predict_gehl(PC);
  predict_rhsp(PC);
  LSUM += SUMGEHL;
  LSUM += SUMRHSP;

  int8_t ctr = Bias[INDBIAS];
  LSUM += 2 * (2 * ctr + 1);
  ctr = BiasFull[INDBIASFULL];
  LSUM += 2 * (2 * ctr + 1);
  ctr = BiasColt[INDBIASCOLT];
  LSUM += 2 * (2 * ctr + 1);

  LSUM += percpredict(PC, GHIST, PERC[PC & ((1 << LOGSIZEG) - 1)], GPSTEP, GWIDTH);
  LSUM += percpredict(PC, L_shist[INDLOCAL], PERCLOC[PC & ((1 << LOGSIZEL) - 1)], LPSTEP, LWIDTH);
  LSUM += percpredict(PC, BHIST, PERCBACK[PC & ((1 << LOGSIZEB) - 1)], BPSTEP, BWIDTH);
  LSUM += percpredict(PC, YHA, PERCYHA[PC & ((1 << LOGSIZEY) - 1)], YPSTEP, YWIDTH);
  LSUM += percpredict(PC, P_phist, PERCPATH[PC & ((1 << LOGSIZEP) - 1)], PPSTEP, PWIDTH);
  LSUM += percpredict(PC, S_slhist[INDSLOCAL], PERCSLOC[PC & ((1 << LOGSIZES) - 1)], SPSTEP, SWIDTH);
  LSUM += percpredict(PC, T_slhist[INDTLOCAL], PERCTLOC[PC & ((1 << LOGSIZET) - 1)], TPSTEP, TWIDTH);
  LSUM += percpredict(PC, Q_slhist[INDQLOCAL], PERCQLOC[PC & ((1 << LOGSIZEQ) - 1)], QPSTEP, QWIDTH);

  LSUM += Gpredict(PC, L_shist[INDLOCAL], Lm, LGEHL, LNB);
  LSUM += Gpredict(PC, T_slhist[INDTLOCAL], Tm, TGEHL, TNB);

  LSUM += Gpredict(PC, Q_slhist[INDQLOCAL], Qm, QGEHL, QNB);

  LSUM += Gpredict(PC, S_slhist[INDSLOCAL], Sm, SGEHL, SNB);

  LSUM += Gpredict(PC, BHIST, Bm, BGEHL, BNB);
  LSUM += Gpredict(PC, YHA, Ym, YGEHL, BNB);
#ifdef IMLI
  LSUM += Gpredict(PC, IMLIhist[INDIMLI], IMLIm, IMLIGEHL, IMLINB);
  LSUM += Gpredict(PC, (L_shist[INDLOCAL] << 16) ^ IMLIcount, LIm, LIGEHL, LINB);
  futurelocal = -1;

  for (int i = Fm[FNB - 1]; i >= 0; i--) {
    futurelocal =
        histtable[(((PC ^ (PC >> 2)) << SHIFTFUTURE) + IMLIcount + i) & (HISTTABLESIZE - 1)] + (futurelocal << 1);
  }
  futurelocal = PAST[PC & 63] + (futurelocal << 1);
  LSUM += Gpredict((PC << 8), futurelocal, Fm, fGEHL, FNB);
#else
  IMLIcount = 0;

#endif

  LSUM += Gpredict(PC, (IMLIcount + (GHIST << 16)), Im, IGEHL, INB);

  LSUM += Gpredict(0, Q_slhist[INDQLOCAL], QQm, QQGEHL, QQNB);
  LSUM += Gpredict(PC, CHIST, Cm, CGEHL, CNB);
  LSUM += Gpredict(PC, RHIST, Rm, RGEHL, RNB);

  return (LSUM >= 0);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
// Functions  for the statiscal corrector

void MTAGE::predict_gehl(uint64_t PC) {
  // index computation
  for (int i = 1; i <= NGEHL; i++) {
    GEHLINDEX[i] = gehlindex(PC, i);
  }
  GEHLINDEX[0] = PC & ((1 << LOGGEHL) - 1);

  // SUMGEHL is centered
  SUMGEHL = 0;
  for (int i = 0; i <= NGEHL; i++) {
    SUMGEHL += 2 * GEHL[GEHLINDEX[i]][i] + 1;
  }
}

void MTAGE::gehlupdate(uint64_t PC, bool taken) {
  // update the GEHL  predictor tables
  for (int i = NGEHL; i >= 0; i--)
    ctrupdate(GEHL[GEHLINDEX[i]][i], taken, PERCWIDTH);
}

void MTAGE::predict_rhsp(uint64_t PC) {
  // index computation
  for (int i = 1; i <= NRHSP; i++) {
    RHSPINDEX[i] = rhspindex(PC, i);
  }
  RHSPINDEX[0] = PC & ((1 << LOGRHSP) - 1);

  // SUMRHSP is centered
  SUMRHSP = 0;
  for (int i = 1; i <= NRHSP; i++)
    SUMRHSP += 2 * RHSP[RHSPINDEX[i]][i] + 1;
}

void MTAGE::rhspupdate(uint64_t PC, bool taken) {
  for (int i = NRHSP; i >= 1; i--)
    ctrupdate(RHSP[RHSPINDEX[i]][i], taken, PERCWIDTH);
}

int MTAGE::percpredict(int PC, long long BHIST, int8_t* line, int PSTEP, int WIDTH) {
  PERCSUM = 0;
  long long bhist = BHIST;
  int PT = 0;

  for (int i = 0; i < WIDTH; i += PSTEP) {
    int index = bhist & ((1 << PSTEP) - 1);
    int8_t ctr = line[PT + index];

    PERCSUM += 2 * ctr + 1;

    bhist >>= PSTEP;
    PT += (1 << PSTEP);
  }

  return PERCSUM;
}

void MTAGE::updateperc(bool taken, int8_t* line, long long BHIST, int PSTEP, int WIDTH) {
  int PT = 0;
  long long bhist = BHIST;

  for (int i = 0; i < WIDTH; i += PSTEP) {
    int index = bhist & ((1 << PSTEP) - 1);
    ctrupdate(line[PT + index], taken, PERCWIDTH);
    bhist >>= PSTEP;
    PT += (1 << PSTEP);
  }
}

int MTAGE::Gpredict(uint64_t PC, long long BHIST, int* length, int8_t** tab, int NBR) {
  PERCSUM = 0;

  for (int i = 0; i < NBR; i++) {
    long long bhist = BHIST & ((long long)((1 << length[i]) - 1));

    int index =
        (((long long)PC) ^ bhist ^ (bhist >> (LOGTAB - i)) ^ (bhist >> (40 - 2 * i)) ^ (bhist >> (60 - 3 * i))) &
        (TABSIZE - 1);

    int8_t ctr = tab[i][index];
    PERCSUM += 2 * ctr + 1;
  }
  return PERCSUM;
}

void MTAGE::Gupdate(uint64_t PC, bool taken, long long BHIST, int* length, int8_t** tab, int NBR, int WIDTH) {
  for (int i = 0; i < NBR; i++) {
    long long bhist = BHIST & ((long long)((1 << length[i]) - 1));

    int index =
        (((long long)PC) ^ bhist ^ (bhist >> (LOGTAB - i)) ^ (bhist >> (40 - 2 * i)) ^ (bhist >> (60 - 3 * i))) &
        (TABSIZE - 1);

    ctrupdate(tab[i][index], taken, WIDTH);
  }
}

int MTAGE::gehlindex(uint64_t PC, int bank) {
  int index = PC ^ (PC >> ((mgehl[bank] % LOGGEHL) + 1)) ^ chgehl_i[bank].comp;
  return index & ((1 << LOGGEHL) - 1);
}

int MTAGE::rhspindex(uint64_t PC, int bank) {
  int index = PC ^ (PC >> ((mrhsp[bank] % LOGRHSP) + 1)) ^ chrhsp_i[bank].comp;
  if (bank > 1) {
    index ^= chrhsp_i[bank - 1].comp;
  }
  if (bank > 3) {
    index ^= chrhsp_i[bank / 3].comp;
  }
  return index & ((1 << LOGRHSP) - 1);
}
