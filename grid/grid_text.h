// Segment font, text & mode-number overlay
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

// 12-segment stroke font: each glyph is 2 cells wide x 2 cells tall
// (a 3x3 node grid). The centre column + centre row let us draw proper
// letters (real G, R, K, M, N…) rather than calculator digits.
//   n--TL--n--TR--n
//   |LU     |CU    |RU
//   n--ML--n--MR--n
//   |LL     |CL    |RL
//   n--BL--n--BR--n
enum { sTL=1,sTR=2,sML=4,sMR=8,sBL=16,sBR=32,
       sLU=64,sLL=128,sCU=256,sCL=512,sRU=1024,sRL=2048 };
const uint16_t FONTW[37]={
  0,                                              // ' '
  sTL|sTR|sLU|sLL|sRU|sRL|sML|sMR,                // A
  sTL|sTR|sLU|sLL|sRU|sRL|sML|sMR|sBL|sBR,        // B (~8)
  sTL|sTR|sLU|sLL|sBL|sBR,                        // C
  sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR,                // D (~O)
  sTL|sTR|sLU|sLL|sML|sMR|sBL|sBR,                // E
  sTL|sTR|sLU|sLL|sML|sMR,                        // F
  sTL|sTR|sLU|sLL|sBL|sBR|sRL|sMR,                // G
  sLU|sLL|sRU|sRL|sML|sMR,                        // H
  sTL|sTR|sBL|sBR|sCU|sCL,                        // I
  sRU|sRL|sBL|sBR|sLL,                            // J
  sLU|sLL|sMR|sRU|sRL,                            // K
  sLU|sLL|sBL|sBR,                                // L
  sLU|sLL|sRU|sRL|sTL|sTR|sCU,                    // M
  sLU|sLL|sRU|sRL|sCU|sCL,                        // N
  sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR,                // O
  sTL|sTR|sLU|sLL|sML|sMR|sRU,                    // P
  sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR|sCL,            // Q
  sTL|sTR|sLU|sLL|sML|sMR|sRU|sRL,                // R
  sTL|sTR|sLU|sML|sMR|sRL|sBL|sBR,                // S
  sTL|sTR|sCU|sCL,                                // T
  sLU|sLL|sRU|sRL|sBL|sBR,                        // U
  sLU|sLL|sRU|sRL|sBL|sBR,                        // V (~U)
  sLU|sLL|sRU|sRL|sBL|sBR|sCL,                    // W
  sLU|sRU|sCU|sCL|sLL|sRL,                        // X (approx)
  sLU|sRU|sCU|sCL,                                // Y
  sTL|sTR|sMR|sCL|sBL|sBR,                        // Z (approx)
  sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR,                // 0
  sCU|sCL,                                        // 1
  sTL|sTR|sRU|sML|sMR|sLL|sBL|sBR,                // 2
  sTL|sTR|sRU|sML|sMR|sRL|sBL|sBR,                // 3
  sLU|sML|sMR|sRU|sRL,                            // 4
  sTL|sTR|sLU|sML|sMR|sRL|sBL|sBR,                // 5
  sTL|sTR|sLU|sLL|sML|sMR|sRL|sBL|sBR,            // 6
  sTL|sTR|sRU|sRL,                                // 7
  sTL|sTR|sLU|sLL|sRU|sRL|sML|sMR|sBL|sBR,        // 8
  sTL|sTR|sLU|sRU|sRL|sML|sMR|sBL|sBR             // 9
};
int glyphIndex(char c){
  if(c>='a'&&c<='z') c-=32;
  if(c>='A'&&c<='Z') return c-'A'+1;
  if(c>='0'&&c<='9') return c-'0'+27;
  return 0; // space / unknown
}
// Draw one glyph with top-left node (topR,C). Box spans nodes topR..topR+2, C..C+2.
void drawGlyphW(uint16_t s,int topR,int C,CRGB col){
  if(s&sTL) drawHSeg(topR,   C,   col);
  if(s&sTR) drawHSeg(topR,   C+1, col);
  if(s&sML) drawHSeg(topR+1, C,   col);
  if(s&sMR) drawHSeg(topR+1, C+1, col);
  if(s&sBL) drawHSeg(topR+2, C,   col);
  if(s&sBR) drawHSeg(topR+2, C+1, col);
  if(s&sLU) drawVSeg(topR,   C,   col);
  if(s&sLL) drawVSeg(topR+1, C,   col);
  if(s&sCU) drawVSeg(topR,   C+1, col);
  if(s&sCL) drawVSeg(topR+1, C+1, col);
  if(s&sRU) drawVSeg(topR,   C+2, col);
  if(s&sRL) drawVSeg(topR+1, C+2, col);
}
// Width / draw in node-columns (3 per char: 2 wide glyph + 1 gap).
int textLenNodes(const char* s){ int n=0; while(*s++) n++; return n*3; }
void drawTextCells(const char* s,int leftC,int topR,CRGB col){
  int C=leftC;
  for(const char* p=s;*p;p++){ drawGlyphW(FONTW[glyphIndex(*p)],topR,C,col); C+=3; }
}

// ── Condensed 1-cell-wide 7-segment micro-font (Task 5) ──
// Each glyph is ONE cell wide x TWO cells tall (nodes topR..topR+2, C..C+1), so
// 6 letters fit statically across the 11-column lattice (CASTLE / 2026, no scroll).
// Segments a=top g=mid d=bottom  f=top-left e=bottom-left  b=top-right c=bottom-right.
enum { g7a=1,g7b=2,g7c=4,g7d=8,g7e=16,g7f=32,g7g=64 };
uint8_t seg7(char c){
  if(c>='a'&&c<='z') c-=32;
  switch(c){
    case '0': return g7a|g7b|g7c|g7d|g7e|g7f;
    case '1': return g7b|g7c;
    case '2': return g7a|g7b|g7g|g7e|g7d;
    case '3': return g7a|g7b|g7g|g7c|g7d;
    case '4': return g7f|g7g|g7b|g7c;
    case '5': return g7a|g7f|g7g|g7c|g7d;
    case '6': return g7a|g7f|g7g|g7e|g7c|g7d;
    case '7': return g7a|g7b|g7c;
    case '8': return g7a|g7b|g7c|g7d|g7e|g7f|g7g;
    case '9': return g7a|g7b|g7c|g7d|g7f|g7g;
    case 'A': return g7a|g7b|g7c|g7e|g7f|g7g;
    case 'C': return g7a|g7d|g7e|g7f;
    case 'E': return g7a|g7d|g7e|g7f|g7g;
    case 'F': return g7a|g7e|g7f|g7g;
    case 'H': return g7b|g7c|g7e|g7f|g7g;
    case 'L': return g7d|g7e|g7f;
    case 'O': return g7a|g7b|g7c|g7d|g7e|g7f;
    case 'P': return g7a|g7b|g7e|g7f|g7g;
    case 'S': return g7a|g7c|g7d|g7f|g7g;
    case 'T': return g7d|g7e|g7f|g7g;   // 7-seg 't' (no centre column possible in 1 cell)
    case 'U': return g7b|g7c|g7d|g7e|g7f;
    default:  return 0;                 // space / unsupported
  }
}
void drawGlyph1(uint8_t s,int topR,int C,CRGB col){   // 1 cell wide, 2 cells tall
  if(s&g7a) drawHSeg(topR,   C,   col);
  if(s&g7g) drawHSeg(topR+1, C,   col);
  if(s&g7d) drawHSeg(topR+2, C,   col);
  if(s&g7f) drawVSeg(topR,   C,   col);
  if(s&g7e) drawVSeg(topR+1, C,   col);
  if(s&g7b) drawVSeg(topR,   C+1, col);
  if(s&g7c) drawVSeg(topR+1, C+1, col);
}
// Draw a word centered horizontally at node-row topR (advances 2 node-cols/char).
void drawText1Centered(const char* s,int topR,CRGB col){
  int n=0; for(const char* p=s;*p;p++) n++;
  int leftC=(12-2*n)/2; if(leftC<0) leftC=0;
  int C=leftC; for(const char* p=s;*p;p++){ drawGlyph1(seg7(*p),topR,C,col); C+=2; }
}
#define TEXT_TOP 4          // top node row for banner text (glyphs 2 cells tall → rows 4..6)

// ── Mode-change readout: the mode NUMBER in the top-right corner for ~0.5s,
//    drawn INTO the frame by showGrid() (see the forward-declares in grid.ino).
//    It used to be painted after the animation's show() and pushed with a second
//    FastLED.show(); at 10-15ms per show that left it dark for half of every
//    frame, which read as a hard strobe. It is a readout, not an event — it
//    should sit there quietly and go away. ──
uint32_t overlayUntil=0;
char     overlayNum[4];     // "40" (mode) or "A2" (auto mood)
uint8_t  overlayMood=0;     // 0 = mode readout (white), 1..4 = that mood's colour
void triggerOverlay(uint32_t t,int idx){
  int n=idx+1; char* p=overlayNum;
  if(n>=10) *p++=(char)('0'+n/10);
  *p++=(char)('0'+n%10); *p=0;
  overlayMood=0;
  overlayUntil=t+500;        // number flashes ~0.5s, top-right, over the animation
}
// The auto mood (button 0) reads out as "A1".."A4" in that mood's own colour —
// the same colour button 0 is blinking in, so the wall and the controller can
// never name the same mood differently.
//
// The "A" carries the identity, not the colour: "A2" and mode 2 would otherwise
// differ by hue alone, and the tint walks the mood hues onto each other (mood 2
// at tint 150 lands on mood 4's own green). The colour is the link to the
// button; the letter is what makes the readout unambiguous.
//
// Held twice as long as the mode number: it answers "what did I just press" on
// a button that only ever counts up 1→2→3→4→1 and never shows where it stopped.
#define AUTO_MS 1000
void triggerOverlayMood(uint32_t t,uint8_t mood){
  if(mood<1||mood>TOGA_AUTO_COUNT) return;
  overlayNum[0]='A'; overlayNum[1]=(char)('0'+mood); overlayNum[2]=0;
  overlayMood=mood;
  overlayUntil=t+AUTO_MS;
}
// Draw the mode number small in the top-right (rows 0..2), into the frame.
// Plain white, deliberately dim: it is drawn AFTER the AGC has measured the
// frame, so it never gets dimmed along with a bright mode — at 150 it was the
// brightest thing on the wall on every mode change. Tune NUM_WHITE alone if it
// reads too bright or too faint; it is the only knob for this.
//
// AUTO_V is the same knob for the mood readout, and it is higher for a reason:
// a fully saturated hue at NUM_WHITE lights one channel where white lights
// three, so at 70 it would read as a third of the brightness. It is not a
// second opinion about how bright the corner should be.
#define NUM_WHITE 70
#define AUTO_V   160
void drawNumberTopRight(uint32_t t){
  // Colour resolved HERE, not at trigger time: hueAuto rotates continuously, and
  // button 0 wears the tint live. Latching it would leave the wall on a stale
  // hue for the whole second while the button turned — at the top of the Hue-
  // Speed dial that is three full revolutions of disagreement.
  CRGB col(NUM_WHITE,NUM_WHITE,NUM_WHITE);
  if(overlayMood)
    col=CRGB(CHSV((uint8_t)(TOGA_AUTO_HUE[overlayMood-1]+gHueBase+gHueAuto),255,AUTO_V));
  int w=textLenNodes(overlayNum);           // node-cols (3 per digit incl. gap)
  int leftC=11-w; if(leftC<0) leftC=0;       // right-aligned, ~1 node margin
  drawTextCells(overlayNum,leftC,0,col);
}
