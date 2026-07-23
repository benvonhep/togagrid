// Geometric / boxy animations
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

// 35. SQUARE TUNNEL
// Concentric square frames zoom toward viewer like flying through a tunnel.
// Perspective: outer frames = far, inner = near. Smooth continuous motion.
void anim_squareTunnel(uint32_t t) {
  float tf=t*0.001f;
  float offset=fmod(tf*0.8f,1.0f);
  // Tunnel slowly rotates/shifts off-center for parallax
  float shiftX=sinf(tf*0.13f)*1.5f;
  float shiftY=cosf(tf*0.09f)*1.5f;

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  for(int frame=0;frame<8;frame++){
    float depth=(float)frame+offset;
    float fade=1.0f-depth/8.0f;if(fade<=0.0f)continue;
    float size=(7.0f-depth)/7.0f;
    int half=(int)(size*5.5f);if(half<1)continue;
    // Shift center slightly for parallax
    int cx=6+(int)(shiftX*(1.0f-size));
    int cy=6+(int)(shiftY*(1.0f-size));
    int r1=constrain(cy-half,1,12);int r2=constrain(cy+half+1,1,12);
    int c1=constrain(cx-half,1,12);int c2=constrain(cx+half+1,1,12);
    if(r1>=r2||c1>=c2)continue;
    uint8_t b=(uint8_t)(fade*fade*230.0f);
    // Each frame gets a hue that rotates — makes tunnel feel alive
    uint8_t hue=(uint8_t)(depth*32.0f+tf*25.0f);
    uint8_t sat=(uint8_t)(180.0f+fade*75.0f);
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,sat,b)));
  }
  showGrid();
}

// Light the four corners of a grid square. drawGridSquare() runs each border
// strictly BETWEEN the intersection LEDs, so the corner LEDs (at INTS[]) are
// left dark — which reads as a square with gaps at the corners. Fill both the
// horizontal- and vertical-strip LED at each of the four crossings so the
// outline closes.
void drawSquareCorners(int r1,int c1,int r2,int c2,CRGB col){
  setLED(false,r1,INTS[c1-1],col); setLED(false,r1,INTS[c2-1],col);   // top strip: left/right corner
  setLED(false,r2,INTS[c1-1],col); setLED(false,r2,INTS[c2-1],col);   // bottom strip
  setLED(true, c1,INTS[r1-1],col); setLED(true, c1,INTS[r2-1],col);   // left strip: top/bottom corner
  setLED(true, c2,INTS[r1-1],col); setLED(true, c2,INTS[r2-1],col);   // right strip
}

// SPOT TUNNEL — controller phys key 5 (TOGA_F_SPOT) override, NOT a mode.
// While the moving-spot key is held the modules chase bright dots down their
// strips; the wall drives one square that is born at the centre and grows out
// to full grid size, then dissolves at the rim and starts again — a single
// frame rushing toward you, so it reads as driving "through" something. Real-
// time clock (millis), so the drive speed is the same at any Speed-knob
// setting. Colour still answers the Color knob: showGrid() rotates every hue.
void anim_spotTunnel(uint32_t t) {
  float tf    = t*0.001f;
  float period= 1.5f;                        // seconds per expansion (matches the floor-spot period)
  float phase = fmodf(tf/period, 1.0f);      // 0..1: one square, centre → rim
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  int half = (int)(phase*6.0f); half = constrain(half,0,5);   // 0..5 → centre cell .. full wall
  int r1=6-half, r2=7+half, c1=6-half, c2=7+half;
  // Brightens as it grows (dim little square at birth → bright as it fills the
  // wall), then a quick fade over the last stretch so it melts at the rim.
  float fade = phase<0.85f ? (0.30f+0.70f*(phase/0.85f)) : (1.0f-(phase-0.85f)/0.15f);
  uint8_t b   = (uint8_t)(constrain(fade,0.0f,1.0f)*235.0f); if(b<3){ showGrid(); return; }
  uint8_t hue = (uint8_t)(tf*30.0f);
  uint8_t sat = (uint8_t)constrain(220-(int)(phase*140.0f),80,255);   // whiter as it nears (headlight glare)
  CRGB col = CRGB(CHSV(hue,sat,b));
  drawGridSquare(r1,c1,r2,c2,col);
  drawSquareCorners(r1,c1,r2,c2,col);
  showGrid();
}

// 36. CONCENTRIC SQUARE PULSE
// Square frames expand outward from center like sonar pings.
// Multiple overlapping pulses, smooth brightness wave.
void anim_concentricSquares(uint32_t t) {
  float tf=t*0.001f;float pulseRate=0.5f;

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // For each possible square size (1..5 half-width = sizes 2..11)
  for(int half=1;half<=5;half++){
    int r1=6-half; int r2=7+half;
    int c1=6-half; int c2=7+half;
    // Pulse phase for this size: outer squares = earlier phase
    float phase=fmod(tf*pulseRate-(float)half*0.15f,1.0f);if(phase<0.0f)phase+=1.0f;
    // Smooth sine brightness
    float bri=powf(sinf(phase*M_PI),2.0f);
    uint8_t b=(uint8_t)(bri*220.0f);if(b<3)continue;
    uint8_t hue=(uint8_t)(tf*30.0f+half*20.0f);
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,220,b)));
  }
  showGrid();
}
static int sqR[SQRAIN_COUNT],sqC[SQRAIN_COUNT],sqSize[SQRAIN_COUNT];
static uint32_t sqBirth[SQRAIN_COUNT];static uint8_t sqHue[SQRAIN_COUNT];
static bool sqRainInit=false;
void anim_squareRain(uint32_t t) {
  if(!sqRainInit){for(int i=0;i<SQRAIN_COUNT;i++){sqBirth[i]=t-random(2000);sqR[i]=1+random(11);sqC[i]=1+random(11);sqSize[i]=1+random(4);sqHue[i]=random(255);}sqRainInit=true;}
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  uint32_t dropLife=600;
  for(int i=0;i<SQRAIN_COUNT;i++){
    uint32_t age=t-sqBirth[i];
    if(age>dropLife){sqBirth[i]=t;sqR[i]=1+random(11);sqC[i]=1+random(11);sqSize[i]=1+random(4);sqHue[i]=random(255);age=0;}
    float phase=(float)age/dropLife;
    // Fast rise, slow fade
    float bri=phase<0.1f?phase/0.1f:1.0f-(phase-0.1f)/0.9f;
    uint8_t b=(uint8_t)(bri*230.0f);if(b<3)continue;
    int r1=sqR[i]; int c1=sqC[i];
    int r2=constrain(r1+sqSize[i],1,12);
    int c2=constrain(c1+sqSize[i],1,12);
    if(r1>=r2||c1>=c2)continue;
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(sqHue[i],220,b)));
  }
  showGrid();
}

// 38. FOURFOLD SYMMETRY
// One quadrant generates a random organic animation.
// The result is mirrored into all 4 quadrants simultaneously.
// Geometric and hypnotic. Uses plasma-style source.
void anim_fourfoldSymmetry(uint32_t t) {
  float tf=t*0.001f;
  // Generate source quadrant (top-left: X1-6, Y1-6)
  // Then mirror to all 4 quadrants
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Fold into top-left quadrant
    float fx=fabsf(x); float fy=fabsf(y);
    // Source pattern on folded coords
    float v=sinf(fx*0.1f+tf*0.7f)*sinf(fy*0.09f-tf*0.5f)
           +sinf((fx+fy)*0.06f+tf*0.4f)*0.5f
           +sinf(sqrtf(fx*fx+fy*fy)*0.08f-tf*0.6f)*0.5f;
    v=(v+2.0f)/4.0f;
    uint8_t hue=(uint8_t)(v*180.0f+tf*20.0f);
    uint8_t bri=(uint8_t)(sinf(v*M_PI)*220.0f);
    setLED(true,xi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float fx=fabsf(x);float fy=fabsf(y);
    float v=sinf(fx*0.1f+tf*0.7f)*sinf(fy*0.09f-tf*0.5f)+sinf((fx+fy)*0.06f+tf*0.4f)*0.5f+sinf(sqrtf(fx*fx+fy*fy)*0.08f-tf*0.6f)*0.5f;
    v=(v+2.0f)/4.0f;uint8_t hue=(uint8_t)(v*180.0f+tf*20.0f);uint8_t bri=(uint8_t)(sinf(v*M_PI)*220.0f);
    setLED(false,yi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);
  }}
  showGrid();
}

// 39. DIAMOND PULSE
// Concentric diamonds (45° squares) expand from center.
// Distance measured as Manhattan distance = perfect diamond shape.
void anim_diamondPulse(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Manhattan distance from center = diamond metric
    float manhattan=(fabsf(x)+fabsf(y))/MAX_DIST;
    // Multiple expanding rings
    float ring=fmod(manhattan-tf*0.4f,0.25f)/0.25f;
    float bri=powf(sinf(ring*M_PI),3.0f);
    // Rotation: slowly rotate the diamond
    float rotAngle=tf*0.1f;float rx=x*cosf(rotAngle)-y*sinf(rotAngle);float ry=x*sinf(rotAngle)+y*cosf(rotAngle);
    float rotManhattan=(fabsf(rx)+fabsf(ry))/MAX_DIST;
    float ring2=fmod(rotManhattan-tf*0.3f+0.5f,0.25f)/0.25f;
    float bri2=powf(sinf(ring2*M_PI),3.0f)*0.5f;
    float total=constrain(bri+bri2,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*230.0f);
    uint8_t hue=(uint8_t)(manhattan*200.0f+tf*25.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,230,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float manhattan=(fabsf(x)+fabsf(y))/MAX_DIST;float ring=fmod(manhattan-tf*0.4f,0.25f)/0.25f;float bri=powf(sinf(ring*M_PI),3.0f);
    float rotAngle=tf*0.1f;float rx=x*cosf(rotAngle)-y*sinf(rotAngle);float ry=x*sinf(rotAngle)+y*cosf(rotAngle);
    float ring2=fmod((fabsf(rx)+fabsf(ry))/MAX_DIST-tf*0.3f+0.5f,0.25f)/0.25f;
    float total=constrain(bri+powf(sinf(ring2*M_PI),3.0f)*0.5f,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*230.0f);uint8_t hue=(uint8_t)(manhattan*200.0f+tf*25.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,230,b)):CRGB::Black);
  }}
  showGrid();
}

// 40. CHECKERBOARD MELT
// 121 squares in checkerboard pattern. Colours slowly melt and shift
// between squares. Neighbouring squares bleed into each other.
void anim_checkerboardMelt(uint32_t t) {
  float tf=t*0.001f;
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  for(int row=0;row<11;row++){for(int col=0;col<11;col++){
    bool checker=((row+col)%2==0);
    // Slowly shifting hue per square, neighbours offset
    float hueBase=(float)(row*11+col)*2.0f;
    float hueShift=sinf(tf*0.3f+hueBase*0.05f)*40.0f;
    uint8_t hue=(uint8_t)(hueBase+hueShift+tf*15.0f);
    float bri=0.6f+0.4f*sinf(tf*0.5f+hueBase*0.1f);
    uint8_t b=(uint8_t)(bri*(checker?200.0f:80.0f));
    // Draw filled square: all 4 borders
    int r1=row+1;int c1=col+1;int r2=row+2;int c2=col+2;
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,230,b)));
  }}
  showGrid();
}

// 41. GAME OF LIFE
// Conway's Game of Life on the 11x11 grid of squares.
// Each square = 1 cell. Evolves every 400ms. Full colour by age.
static bool golGrid[11][11]; static bool golNext[11][11];
static uint8_t golAge[11][11]; // age of living cells
static uint32_t golLastStep=0; static bool golInit=false;
void anim_gameOfLife(uint32_t t) {
  if(!golInit){
    for(int r=0;r<11;r++)for(int c=0;c<11;c++){golGrid[r][c]=(random(3)==0);golAge[r][c]=0;}
    golInit=true;golLastStep=t;
  }
  if(t-golLastStep>350){
    // Step
    for(int r=0;r<11;r++)for(int c=0;c<11;c++){
      int n=0;
      for(int dr=-1;dr<=1;dr++)for(int dc=-1;dc<=1;dc++){if(dr==0&&dc==0)continue;int nr=(r+dr+11)%11;int nc=(c+dc+11)%11;if(golGrid[nr][nc])n++;}
      golNext[r][c]=(golGrid[r][c]?(n==2||n==3):(n==3));
    }
    // Check if static (copy and count changes)
    int changes=0;
    for(int r=0;r<11;r++)for(int c=0;c<11;c++){if(golNext[r][c]!=golGrid[r][c])changes++;golGrid[r][c]=golNext[r][c];if(golGrid[r][c])golAge[r][c]++;else golAge[r][c]=0;}
    // If stuck, randomize
    if(changes==0){for(int r=0;r<11;r++)for(int c=0;c<11;c++){golGrid[r][c]=(random(3)==0);golAge[r][c]=0;}}
    golLastStep=t;
  }
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  for(int row=0;row<11;row++)for(int col=0;col<11;col++){
    if(!golGrid[row][col])continue;
    // Colour by age: young=white, old=deep colour
    uint8_t age=min((int)golAge[row][col],60);
    uint8_t hue=(uint8_t)(age*4);
    uint8_t sat=(uint8_t)(min((int)age*4,255));
    uint8_t bri=200;
    drawGridSquare(row+1,col+1,row+2,col+2,CRGB(CHSV(hue,sat,bri)));
  }
  showGrid();
}

// 42. ROTATING CUBE
// Isometric projection of a rotating cube. Cube faces drawn using
// grid squares. 3 visible faces with different brightness/colour.
void anim_rotatingCube(uint32_t t) {
  float tf=t*0.001f;
  float rotX=tf*0.4f; float rotY=tf*0.6f;

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // Cube vertices in 3D (centered, size=4 strips)
  float S=4.0f;
  float verts[8][3]={{-S,-S,-S},{S,-S,-S},{S,S,-S},{-S,S,-S},{-S,-S,S},{S,-S,S},{S,S,S},{-S,S,S}};

  // Rotate all vertices
  float proj[8][2];
  for(int v=0;v<8;v++){
    float x=verts[v][0],y=verts[v][1],z=verts[v][2];
    // Rotate around Y
    float x2=x*cosf(rotY)-z*sinf(rotY); float z2=x*sinf(rotY)+z*cosf(rotY);
    // Rotate around X
    float y3=y*cosf(rotX)-z2*sinf(rotX); float z3=y*sinf(rotX)+z2*cosf(rotX);
    // Isometric projection → grid coords (6=center)
    float iso_x=x2*0.866f-y3*0.866f; float iso_y=x2*0.5f+y3*0.5f-z3;
    proj[v][0]=iso_x+6.5f; proj[v][1]=iso_y+6.5f; // offset to grid center
  }

  // Draw 3 visible faces as square outlines
  // Face indices: front(4,5,6,7), top(3,2,6,7), right(1,5,6,2)
  int faces[3][4]={{4,5,6,7},{3,2,6,7},{1,5,6,2}};
  uint8_t faceHue[3]={(uint8_t)(tf*20),(uint8_t)(tf*20+60),(uint8_t)(tf*20+120)};
  uint8_t faceBri[3]={220,160,100};

  for(int f=0;f<3;f++){
    // Project each face vertex to nearest grid square corner
    for(int e=0;e<4;e++){
      int v1=faces[f][e]; int v2=faces[f][(e+1)%4];
      int r1=constrain((int)proj[v1][1],1,12); int c1=constrain((int)proj[v1][0],1,12);
      int r2=constrain((int)proj[v2][1],1,12); int c2=constrain((int)proj[v2][0],1,12);
      if(r1==r2&&c1==c2)continue;
      // Draw line between projected points on strips
      int rMin=min(r1,r2);int rMax=max(r1,r2);int cMin=min(c1,c2);int cMax=max(c1,c2);
      if(rMin==rMax){// horizontal edge → Y strip
        for(int led=INTS[cMin-1];led<=INTS[cMax-1];led++)setLED(false,rMin,led,CRGB(CHSV(faceHue[f],220,faceBri[f])));
      }else if(cMin==cMax){// vertical edge → X strip
        for(int led=INTS[rMin-1];led<=INTS[rMax-1];led++)setLED(true,cMin,led,CRGB(CHSV(faceHue[f],220,faceBri[f])));
      }
    }
  }
  showGrid();
}

// 43. SQUARE BREATHING
// 11x11 outermost square contracts to 1x1 center, then expands back.
// Smooth, meditative. Multiple nested squares all breathing together.
void anim_squareBreathing(uint32_t t) {
  float tf=t*0.001f;
  float breath=0.5f+0.5f*sinf(tf*0.5f); // 0..1 expand/contract

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // Draw multiple nested squares all breathing in sync with phase offset
  for(int layer=0;layer<5;layer++){
    float layerPhase=fmod(breath+(float)layer*0.15f,1.0f);
    float size=layerPhase; // 0=1x1 center, 1=11x11 outer
    int half=(int)(size*5.0f+0.5f);half=constrain(half,1,5);
    int r1=6-half;int r2=7+half;int c1=6-half;int c2=7+half;
    float bri=sinf(layerPhase*M_PI);
    uint8_t b=(uint8_t)(bri*bri*230.0f);if(b<3)continue;
    uint8_t hue=(uint8_t)(tf*20.0f+layer*35.0f);
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,220,b)));
  }
  showGrid();
}

// 44. FRACTAL SQUARES
// Squares subdivide recursively. Zooms into center subdivision endlessly.
// Mandelbrot-set-zoom vibe using only the square grid geometry.
void anim_fractalSquares(uint32_t t) {
  float tf=t*0.001f;
  float zoom=fmod(tf*0.3f,1.0f); // 0..1 one zoom cycle
  float zoomFactor=powf(2.0f,zoom); // 1..2 then resets

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // Draw grid squares at multiple scales that zoom inward
  for(int level=0;level<4;level++){
    float scale=powf(0.5f,level)/zoomFactor;
    float fade=(float)level/4.0f;
    float bri=1.0f-fade;bri*=(1.0f-zoom*0.3f);
    uint8_t b=(uint8_t)(bri*200.0f);if(b<5)continue;
    uint8_t hue=(uint8_t)(level*60.0f+tf*30.0f);

    // Draw subdivided squares at this scale
    int divisions=(int)(1.0f/scale);
    if(divisions<1)divisions=1;if(divisions>5)divisions=5;
    int step=11/divisions;if(step<1)step=1;
    for(int r=0;r<11;r+=step)for(int c=0;c<11;c+=step){
      int r1=r+1;int c1=c+1;int r2=min(r+step+1,12);int c2=min(c+step+1,12);
      if(r1>=r2||c1>=c2)continue;
      drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,220,b)));
    }
  }
  showGrid();
}

// ══════════════════════════════════════════════
// 10 NEW ANIMATIONS (45-54) — GRADIENT, BRIGHTNESS, SMOOTH, SLOW
// ══════════════════════════════════════════════

// 45. SILK
// Soft flowing waves of colour drift across the grid like silk in wind.
// Very smooth, multi-layered, slow.
void anim_silk(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST; float nY=y/MAX_DIST;
    // Multiple very slow sine layers
    float v = sinf(nX*1.2f + nY*0.8f + tf*0.18f)
            + sinf(nX*0.7f - nY*1.1f + tf*0.13f + 1.5f)*0.7f
            + sinf((nX+nY)*0.9f + tf*0.09f + 3.0f)*0.5f;
    v=(v+2.2f)/4.4f; // 0..1
    // Smooth brightness: never fully dark, never fully bright
    float bri=0.2f+0.7f*powf(sinf(v*M_PI),1.5f);
    uint8_t b=(uint8_t)(bri*210.0f);
    // Slowly drifting hue across the surface
    uint8_t hue=(uint8_t)(v*120.0f + tf*8.0f + nX*20.0f);
    uint8_t sat=(uint8_t)(160.0f+v*80.0f);
    setLED(true,xi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float v=sinf(nX*1.2f+nY*0.8f+tf*0.18f)+sinf(nX*0.7f-nY*1.1f+tf*0.13f+1.5f)*0.7f+sinf((nX+nY)*0.9f+tf*0.09f+3.0f)*0.5f;
    v=(v+2.2f)/4.4f;float bri=0.2f+0.7f*powf(sinf(v*M_PI),1.5f);
    uint8_t b=(uint8_t)(bri*210.0f);uint8_t hue=(uint8_t)(v*120.0f+tf*8.0f+nX*20.0f);uint8_t sat=(uint8_t)(160.0f+v*80.0f);
    setLED(false,yi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  showGrid();
}

// 46. DEEP BREATH
// The entire grid breathes as one organism. Brightness pulses from
// near-zero to full on a long slow cycle. Colour shifts simultaneously.
void anim_deepBreath(uint32_t t) {
  float tf=t*0.001f;
  // Master breath: one full cycle every ~8 seconds
  float breath=0.5f+0.5f*sinf(tf*0.25f*M_PI*0.5f);
  // Breath is not linear — ease in/out (smoothstep)
  breath=breath*breath*(3.0f-2.0f*breath);
  // Colour slowly cycles through warm→cool→warm
  uint8_t hue=(uint8_t)(tf*6.0f);
  // Distance from center adds subtle gradient
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y)/MAX_DIST;
    // Outer LEDs slightly delayed in phase
    float localBreath=0.5f+0.5f*sinf((tf-dist*0.8f)*0.25f*M_PI*0.5f);
    localBreath=localBreath*localBreath*(3.0f-2.0f*localBreath);
    float bri=0.05f+0.9f*localBreath;
    uint8_t b=(uint8_t)(bri*220.0f);
    uint8_t localHue=(uint8_t)(hue+dist*20.0f);
    uint8_t sat=(uint8_t)(180.0f+localBreath*75.0f);
    setLED(true,xi+1,n,CRGB(CHSV(localHue,sat,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y)/MAX_DIST;
    float localBreath=0.5f+0.5f*sinf((tf-dist*0.8f)*0.25f*M_PI*0.5f);localBreath=localBreath*localBreath*(3.0f-2.0f*localBreath);
    float bri=0.05f+0.9f*localBreath;uint8_t b=(uint8_t)(bri*220.0f);
    uint8_t localHue=(uint8_t)(hue+dist*20.0f);uint8_t sat=(uint8_t)(180.0f+localBreath*75.0f);
    setLED(false,yi+1,n,CRGB(CHSV(localHue,sat,b)));
  }}
  showGrid();
}

// 47. SUNSET HORIZON
// Colour gradient flows from top to bottom. Slowly rotates through
// all sunset colours — deep blue → indigo → purple → red → orange → gold.
// The horizon line drifts slowly.
void anim_sunsetHorizon(uint32_t t) {
  float tf=t*0.001f;
  // Horizon position drifts slowly
  float horizon=sinf(tf*0.07f)*0.3f; // -0.3..+0.3 normalised
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nY=y/MAX_DIST; // -1..1
    // Position relative to horizon
    float pos=nY-horizon; // -1.3..1.3
    float posN=(pos+1.3f)/2.6f; // 0..1
    // Colour gradient: sky (top, posN=0) → horizon (posN=0.5) → ground (posN=1)
    // Sky: deep blue-purple
    // Horizon: orange-red glow
    // Add a soft atmospheric glow at the horizon line
    float horizonGlow=expf(-fabsf(pos)*6.0f)*0.6f;
    // Main gradient hue: 180(blue) at top → 0(red) at horizon → 25(orange) below
    uint8_t hue;
    if(posN<0.5f) hue=(uint8_t)(200.0f-posN*2.0f*200.0f+(tf*5.0f));
    else          hue=(uint8_t)(0.0f+(posN-0.5f)*2.0f*25.0f+(tf*5.0f));
    float bri=0.3f+0.6f*(1.0f-fabsf(posN-0.5f)*1.5f)+horizonGlow;
    bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*220.0f);
    uint8_t sat=(uint8_t)(180.0f+horizonGlow*75.0f);
    setLED(true,xi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nY=y/MAX_DIST;float pos=nY-horizon;float posN=(pos+1.3f)/2.6f;
    float horizonGlow=expf(-fabsf(pos)*6.0f)*0.6f;
    uint8_t hue=posN<0.5f?(uint8_t)(200.0f-posN*2.0f*200.0f+(tf*5.0f)):(uint8_t)((posN-0.5f)*2.0f*25.0f+(tf*5.0f));
    float bri=constrain(0.3f+0.6f*(1.0f-fabsf(posN-0.5f)*1.5f)+horizonGlow,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*220.0f);uint8_t sat=(uint8_t)(180.0f+horizonGlow*75.0f);
    setLED(false,yi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  showGrid();
}

// 48. OIL SLICK
// Iridescent interference colours like oil on water.
// Thin-film colour shifting, slow flowing motion.
void anim_oilSlick(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Thin film interference: path length difference creates colour
    // Multiple slow flowing waves of "film thickness"
    float thickness = sinf(nX*2.1f+nY*1.3f+tf*0.12f)*0.5f
                    + sinf(nX*0.8f-nY*2.4f+tf*0.09f+2.0f)*0.3f
                    + sinf((nX*nX+nY*nY)*3.0f-tf*0.07f)*0.2f;
    thickness=(thickness+1.0f)*0.5f; // 0..1
    // Interference hue cycles rapidly with thickness
    uint8_t hue=(uint8_t)(thickness*255.0f+tf*4.0f);
    // High saturation, moderate brightness — iridescent look
    float bri=0.4f+0.5f*powf(sinf(thickness*M_PI*2.0f+tf*0.3f)*0.5f+0.5f,1.5f);
    uint8_t b=(uint8_t)(bri*200.0f);
    setLED(true,xi+1,n,CRGB(CHSV(hue,255,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float thickness=sinf(nX*2.1f+nY*1.3f+tf*0.12f)*0.5f+sinf(nX*0.8f-nY*2.4f+tf*0.09f+2.0f)*0.3f+sinf((nX*nX+nY*nY)*3.0f-tf*0.07f)*0.2f;
    thickness=(thickness+1.0f)*0.5f;uint8_t hue=(uint8_t)(thickness*255.0f+tf*4.0f);
    float bri=0.4f+0.5f*powf(sinf(thickness*M_PI*2.0f+tf*0.3f)*0.5f+0.5f,1.5f);
    setLED(false,yi+1,n,CRGB(CHSV(hue,255,(uint8_t)(bri*200.0f))));
  }}
  showGrid();
}

// 49. EMBER GLOW
// Glowing coal aesthetic. Brightness pulses organically like a fire
// breathing. Deep reds and oranges, dark areas very dark.
void anim_emberGlow(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Slow organic turbulence
    float heat = sinf(nX*2.3f+nY*1.7f+tf*0.22f)
               * sinf(nX*1.1f-nY*2.1f+tf*0.17f+1.0f);
    heat=(heat+1.0f)*0.5f; // 0..1
    // Secondary slow pulse
    float pulse=0.5f+0.5f*sinf(tf*0.4f+nX*1.5f+nY*1.2f);
    float total=heat*0.7f+pulse*0.3f;
    // Power curve: dark areas very dark, bright areas very bright
    total=powf(total,2.2f);
    float bri=constrain(total,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*240.0f);
    // Colour: deep red → orange → yellow-white at peak
    uint8_t hue=(uint8_t)(bri<0.3f?0:bri<0.7f?(bri-0.3f)/0.4f*25.0f:25.0f+(bri-0.7f)/0.3f*15.0f);
    uint8_t sat=(uint8_t)(255.0f-bri*120.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float heat=sinf(nX*2.3f+nY*1.7f+tf*0.22f)*sinf(nX*1.1f-nY*2.1f+tf*0.17f+1.0f);heat=(heat+1.0f)*0.5f;
    float pulse=0.5f+0.5f*sinf(tf*0.4f+nX*1.5f+nY*1.2f);
    float total=powf(heat*0.7f+pulse*0.3f,2.2f);float bri=constrain(total,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*240.0f);uint8_t hue=(uint8_t)(bri<0.3f?0:bri<0.7f?(bri-0.3f)/0.4f*25.0f:25.0f+(bri-0.7f)/0.3f*15.0f);
    uint8_t sat=(uint8_t)(255.0f-bri*120.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  showGrid();
}

// 50. DEPTH MAP
// Radial gradient: centre bright and warm, edges dark and cool.
// Slowly rotates and breathes. Smooth concentric colour bands.
void anim_depthMap(uint32_t t) {
  float tf=t*0.001f;
  float breath=0.7f+0.3f*sinf(tf*0.35f);
  float rotSpeed=tf*0.06f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Slowly rotating coordinate
    float rx=x*cosf(rotSpeed)-y*sinf(rotSpeed);
    float ry=x*sinf(rotSpeed)+y*cosf(rotSpeed);
    float dist=sqrtf(rx*rx+ry*ry)/MAX_DIST; // 0..1
    // Smooth radial gradient with soft banding
    float banding=0.8f+0.2f*sinf(dist*M_PI*5.0f-tf*0.3f);
    float bri=(1.0f-powf(dist,1.4f))*banding*breath;
    bri=constrain(bri,0.0f,1.0f);
    // Colour: warm (low hue) at centre, cool (high hue) at edge
    uint8_t hue=(uint8_t)(dist*160.0f+tf*4.0f);
    uint8_t sat=(uint8_t)(150.0f+dist*105.0f);
    uint8_t b=(uint8_t)(bri*230.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float rx=x*cosf(rotSpeed)-y*sinf(rotSpeed);float ry=x*sinf(rotSpeed)+y*cosf(rotSpeed);
    float dist=sqrtf(rx*rx+ry*ry)/MAX_DIST;float banding=0.8f+0.2f*sinf(dist*M_PI*5.0f-tf*0.3f);
    float bri=constrain((1.0f-powf(dist,1.4f))*banding*breath,0.0f,1.0f);
    uint8_t hue=(uint8_t)(dist*160.0f+tf*4.0f);uint8_t sat=(uint8_t)(150.0f+dist*105.0f);uint8_t b=(uint8_t)(bri*230.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  showGrid();
}

// 51. NORTHERN MIST
// Extremely delicate wisps of colour float across the grid.
// Very low brightness. Blue/green/white. Ghostly and minimal.
void anim_northernMist(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Very slow multi-layer mist
    float mist = sinf(nX*1.5f+nY*0.6f+tf*0.08f)*0.5f+0.5f;
    float mist2= sinf(nX*0.4f-nY*1.8f+tf*0.06f+2.5f)*0.5f+0.5f;
    float mist3= sinf((nX+nY)*1.1f+tf*0.05f+5.0f)*0.5f+0.5f;
    // Multiply layers for wispy look (most of grid near-zero)
    float total=mist*mist2*mist3;
    total=powf(total,1.5f); // darken midtones
    float bri=total*0.55f; // max brightness very low — mist is subtle
    uint8_t b=(uint8_t)(bri*180.0f);
    // Colour: shifts between icy blue, mint green, and near-white
    uint8_t hue=(uint8_t)(140.0f+mist*40.0f+tf*3.0f);
    uint8_t sat=(uint8_t)(100.0f+mist2*80.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float mist=sinf(nX*1.5f+nY*0.6f+tf*0.08f)*0.5f+0.5f;float mist2=sinf(nX*0.4f-nY*1.8f+tf*0.06f+2.5f)*0.5f+0.5f;float mist3=sinf((nX+nY)*1.1f+tf*0.05f+5.0f)*0.5f+0.5f;
    float total=powf(mist*mist2*mist3,1.5f);float bri=total*0.55f;uint8_t b=(uint8_t)(bri*180.0f);
    uint8_t hue=(uint8_t)(140.0f+mist*40.0f+tf*3.0f);uint8_t sat=(uint8_t)(100.0f+mist2*80.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  showGrid();
}

// 52. MOLTEN GOLD
// Heavy slow viscous waves of gold and amber. High brightness at peaks,
// very dark in troughs. Like liquid metal cooling and flowing.
void anim_moltenGold(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Slow heavy waves — low frequency, high amplitude
    float flow = sinf(nX*1.3f+nY*0.9f+tf*0.14f)
               + sinf(nX*0.6f-nY*1.5f+tf*0.11f+2.0f)*0.6f
               + sinf(sqrtf(nX*nX+nY*nY)*2.0f-tf*0.08f)*0.4f;
    flow=(flow+2.0f)/4.0f; // 0..1
    // Viscous: power curve makes bright peaks sharp, dark troughs wide
    float bri=powf(flow,1.8f);
    uint8_t b=(uint8_t)(bri*245.0f);
    // Gold palette: dark amber→gold→bright warm white at peaks
    uint8_t hue=(uint8_t)(25.0f-bri*10.0f); // 25(orange)→15(warm gold)
    uint8_t sat=(uint8_t)(255.0f-bri*160.0f); // desaturates to near-white at peaks
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float flow=sinf(nX*1.3f+nY*0.9f+tf*0.14f)+sinf(nX*0.6f-nY*1.5f+tf*0.11f+2.0f)*0.6f+sinf(sqrtf(nX*nX+nY*nY)*2.0f-tf*0.08f)*0.4f;
    flow=(flow+2.0f)/4.0f;float bri=powf(flow,1.8f);uint8_t b=(uint8_t)(bri*245.0f);
    uint8_t hue=(uint8_t)(25.0f-bri*10.0f);uint8_t sat=(uint8_t)(255.0f-bri*160.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  showGrid();
}

// 53. COSMIC DUST
// Nebula-like colour clouds drifting very slowly. Purple/blue/pink.
// Soft and dreamlike. No hard edges anywhere.
void anim_cosmicDust(uint32_t t) {
  float tf=t*0.001f;
  // 3 slowly moving colour clouds
  float c1x=sinf(tf*0.07f)*40.0f;float c1y=cosf(tf*0.05f)*40.0f;
  float c2x=sinf(tf*0.06f+2.0f)*45.0f;float c2y=cosf(tf*0.08f+1.0f)*35.0f;
  float c3x=sinf(tf*0.09f+4.0f)*38.0f;float c3y=cosf(tf*0.06f+3.0f)*42.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float d1=sqrtf((x-c1x)*(x-c1x)+(y-c1y)*(y-c1y));
    float d2=sqrtf((x-c2x)*(x-c2x)+(y-c2y)*(y-c2y));
    float d3=sqrtf((x-c3x)*(x-c3x)+(y-c3y)*(y-c3y));
    // Soft Gaussian clouds
    float v1=expf(-d1*d1*0.0008f)*0.8f;
    float v2=expf(-d2*d2*0.0007f)*0.7f;
    float v3=expf(-d3*d3*0.0009f)*0.6f;
    // Each cloud has its own hue
    CRGB col=CRGB::Black;
    if(v1>0.01f){CRGB c=CRGB(CHSV(200,(uint8_t)(220),(uint8_t)(v1*200.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v2>0.01f){CRGB c=CRGB(CHSV(240,(uint8_t)(200),(uint8_t)(v2*180.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v3>0.01f){CRGB c=CRGB(CHSV(170,(uint8_t)(210),(uint8_t)(v3*160.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    setLED(true,xi+1,n,col);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float d1=sqrtf((x-c1x)*(x-c1x)+(y-c1y)*(y-c1y));float d2=sqrtf((x-c2x)*(x-c2x)+(y-c2y)*(y-c2y));float d3=sqrtf((x-c3x)*(x-c3x)+(y-c3y)*(y-c3y));
    float v1=expf(-d1*d1*0.0008f)*0.8f;float v2=expf(-d2*d2*0.0007f)*0.7f;float v3=expf(-d3*d3*0.0009f)*0.6f;
    CRGB col=CRGB::Black;
    if(v1>0.01f){CRGB c=CRGB(CHSV(200,220,(uint8_t)(v1*200.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v2>0.01f){CRGB c=CRGB(CHSV(240,200,(uint8_t)(v2*180.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v3>0.01f){CRGB c=CRGB(CHSV(170,210,(uint8_t)(v3*160.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    setLED(false,yi+1,n,col);
  }}
  showGrid();
}

// 54. HEARTGLOW
// One single very slow warm pulse radiates from the center.
// Long pause between beats. Deeply warm and intimate.
// The brightest possible version of "one thing done perfectly".
void anim_heartglow(uint32_t t) {
  float tf=t*0.001f;
  // Heartbeat timing: beat every ~4s, very slow expansion
  float cycle=fmod(tf*0.25f,1.0f); // one cycle per 4s
  // Shape: fast rise (0→0.15), slow glow (0.15→0.6), long pause (0.6→1.0)
  float pulse;
  if(cycle<0.15f)      pulse=cycle/0.15f;                          // rise
  else if(cycle<0.6f)  pulse=1.0f-(cycle-0.15f)/0.45f*0.7f;       // glow decay
  else                 pulse=0.3f*(1.0f-(cycle-0.6f)/0.4f);        // tail to silence
  pulse=pulse*pulse*(3.0f-2.0f*pulse); // smoothstep

  // Pulse radius expands outward
  float radius=cycle*MAX_DIST*1.3f;

  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);
    // Warm ambient glow always present (very dim)
    float ambient=expf(-dist*0.03f)*0.08f;
    // Expanding pulse ring
    float ring=max(0.0f,1.0f-fabsf(dist-radius)/12.0f)*pulse;
    // Core glow at center during peak
    float core=expf(-dist*0.07f)*pulse*0.8f;
    float total=constrain(ambient+ring+core,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*235.0f);
    // Warm: deep red→warm orange→soft white at peak
    uint8_t hue=(uint8_t)(5.0f+pulse*15.0f);
    uint8_t sat=(uint8_t)(255.0f-total*180.0f);
    setLED(true,xi+1,n,b>1?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);
    float ambient=expf(-dist*0.03f)*0.08f;float ring=max(0.0f,1.0f-fabsf(dist-radius)/12.0f)*pulse;float core=expf(-dist*0.07f)*pulse*0.8f;
    float total=constrain(ambient+ring+core,0.0f,1.0f);uint8_t b=(uint8_t)(total*235.0f);
    uint8_t hue=(uint8_t)(5.0f+pulse*15.0f);uint8_t sat=(uint8_t)(255.0f-total*180.0f);
    setLED(false,yi+1,n,b>1?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  showGrid();
}

// ══════════════════════════════════════════════
// ANIMATION 55: EVOLUTION
// A self-developing generative system. Never the same twice.
// ══════════════════════════════════════════════

struct Gene {
  float value;
  float target;
  float rate;
  float baseRate;
  float memory;
};
