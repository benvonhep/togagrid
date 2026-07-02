// Game-style animations
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

// ── Icons: 11 rows, 11 bits each (col0 = MSB / bit 10) ──
void drawIcon(const uint16_t rows[11],CRGB c){
  for(int r=0;r<11;r++)
    for(int col=0;col<11;col++)
      if(rows[r]&(1<<(10-col))) fillCell(r,col,c);
}
const uint16_t ICON_HEART[11]={
  0b00000000000,0b01100011000,0b11110111100,0b11111111110,0b11111111110,
  0b11111111110,0b01111111100,0b00111111000,0b00011110000,0b00001100000,0b00000000000};
const uint16_t ICON_SMILEY[11]={
  0b00011111000,0b00111111100,0b01111111110,0b01101110110,0b01101110110,
  0b01111111110,0b01111111110,0b01100000110,0b01111111110,0b00111111100,0b00011111000};
const uint16_t ICON_STAR[11]={
  0b00000100000,0b00000100000,0b00001110000,0b11111111111,0b01111111110,
  0b00111111100,0b00111111100,0b01110001110,0b01100000110,0b01000000010,0b00000000000};
const uint16_t ICON_ARROW[11]={
  0b00000100000,0b00001110000,0b00011111000,0b00111111100,0b01111111110,
  0b11101110111,0b00001110000,0b00001110000,0b00001110000,0b00001110000,0b00001110000};
const uint16_t ICON_INVADER[11]={
  0b00100000100,0b00010001000,0b00111111100,0b01101110110,0b11111111111,
  0b10111111101,0b10100000101,0b00011011000,0b00000000000,0b00000000000,0b00000000000};
const uint16_t ICON_NOTE[11]={
  0b00000011110,0b00000011110,0b00000010010,0b00000010010,0b00000010010,
  0b00000010010,0b00011010010,0b00111011100,0b00111011100,0b00011000000,0b00000000000};
const uint16_t* const ICONS[6]={ICON_HEART,ICON_SMILEY,ICON_STAR,ICON_ARROW,ICON_INVADER,ICON_NOTE};
const int NUM_ICONS=6;

// ═══════════ 39. TETRIS ═══════════
bool tetInit=false;
static uint8_t tetB[11][11];
static int     tetShape,tetRot,tetR,tetC,tetTargetRot,tetTargetCol;
static uint8_t tetHue;
static uint32_t tetStep;
static bool     tetClearing, tetFull[11];    // line-clear flash state
static uint8_t  tetFlashCnt; static uint32_t tetFlashT;
const int8_t TET_SHAPES[7][4][2]={
  {{0,-1},{0,0},{0,1},{0,2}}, // I
  {{0,0},{0,1},{1,0},{1,1}},  // O
  {{0,-1},{0,0},{0,1},{1,0}}, // T
  {{0,0},{0,1},{1,-1},{1,0}}, // S
  {{0,-1},{0,0},{1,0},{1,1}}, // Z
  {{0,-1},{0,0},{0,1},{1,1}}, // J
  {{0,-1},{0,0},{0,1},{1,-1}} // L
};
// cells of shape s rotated r times → out[4][2]
void tetCells(int s,int r,int8_t out[4][2]){
  for(int i=0;i<4;i++){ out[i][0]=TET_SHAPES[s][i][0]; out[i][1]=TET_SHAPES[s][i][1]; }
  for(int k=0;k<(r&3);k++) for(int i=0;i<4;i++){ int a=out[i][0],b=out[i][1]; out[i][0]=b; out[i][1]=-a; }
}
bool tetFitsCells(int8_t cells[4][2],int row,int col){
  for(int i=0;i<4;i++){ int rr=row+cells[i][0], cc=col+cells[i][1];
    if(cc<0||cc>10||rr>10) return false;
    if(rr>=0 && tetB[rr][cc]) return false; }
  return true;
}
// El-Tetris-style board score: reward line clears, punish height/holes/bumps.
float tetEval(uint8_t bd[11][11],int lines){
  int h[11],agg=0,holes=0,bump=0;
  for(int c=0;c<11;c++){ h[c]=0; for(int r=0;r<11;r++) if(bd[r][c]){ h[c]=11-r; break; } }
  for(int c=0;c<11;c++){ agg+=h[c]; int top=11-h[c]; for(int r=top+1;r<11;r++) if(!bd[r][c]) holes++; }
  for(int c=0;c<10;c++){ int d=h[c]-h[c+1]; bump+=d<0?-d:d; }
  return 0.80f*lines - 0.51f*agg - 0.36f*holes - 0.18f*bump;
}
// Pick the best rotation+column for the current piece (plays to survive/clear).
void tetPlan(){
  float best=-1e30f; int bestR=0,bestC=5; int8_t cells[4][2];
  for(int r=0;r<4;r++){
    tetCells(tetShape,r,cells);
    for(int col=-1;col<=11;col++){
      if(!tetFitsCells(cells,-1,col)) continue;
      int row=-1; while(tetFitsCells(cells,row+1,col)) row++;   // hard drop
      uint8_t tmp[11][11]; bool ok=true;
      for(int a=0;a<11;a++) for(int b=0;b<11;b++) tmp[a][b]=tetB[a][b];
      for(int i=0;i<4;i++){ int rr=row+cells[i][0],cc=col+cells[i][1]; if(rr<0){ok=false;break;} tmp[rr][cc]=1; }
      if(!ok) continue;
      int lines=0;
      for(int rr=10;rr>=0;rr--){ bool full=true; for(int c=0;c<11;c++) if(!tmp[rr][c]) full=false;
        if(full){ lines++; for(int r2=rr;r2>0;r2--) for(int c=0;c<11;c++) tmp[r2][c]=tmp[r2-1][c];
                  for(int c=0;c<11;c++) tmp[0][c]=0; rr++; } }
      float sc=tetEval(tmp,lines);
      if(sc>best){ best=sc; bestR=r; bestC=col; }
    }
  }
  tetTargetRot=bestR; tetTargetCol=bestC;
}
void tetNew(){
  tetShape=random(7); tetRot=0; tetR=-1; tetC=5;
  tetHue=random(256); if(tetHue==0) tetHue=1;
  int8_t cells[4][2]; tetCells(tetShape,0,cells);
  if(!tetFitsCells(cells,tetR,tetC)) for(int a=0;a<11;a++) for(int b=0;b<11;b++) tetB[a][b]=0; // full → restart
  tetPlan();
}
void anim_tetris(uint32_t at,float dt){
  if(!tetInit){ for(int a=0;a<11;a++) for(int b=0;b<11;b++) tetB[a][b]=0; tetStep=at; tetInit=true; tetClearing=false; tetNew(); }
  if(tetClearing){                                     // full rows flash twice, then clear
    if(at-tetFlashT>=170){
      tetFlashT=at;
      if(tetFlashCnt>0) tetFlashCnt--;
      if(tetFlashCnt==0){
        for(int r=10;r>=0;r--){ bool full=true; for(int c=0;c<11;c++) if(!tetB[r][c]) full=false;
          if(full){ for(int rr=r;rr>0;rr--) for(int c=0;c<11;c++) tetB[rr][c]=tetB[rr-1][c];
                    for(int c=0;c<11;c++) tetB[0][c]=0; r++; } }
        tetClearing=false; tetNew();
      }
    }
  } else if(at-tetStep>=300){                           // slow, deliberate play
    tetStep=at;
    int8_t cur[4][2]; tetCells(tetShape,tetRot,cur);
    if(tetRot!=tetTargetRot){                           // 1) turn the piece
      int nr=(tetRot+1)&3; int8_t rc[4][2]; tetCells(tetShape,nr,rc);
      if(tetFitsCells(rc,tetR,tetC)) tetRot=nr;
      else if(tetFitsCells(rc,tetR,tetC-1)){ tetRot=nr; tetC--; }
      else if(tetFitsCells(rc,tetR,tetC+1)){ tetRot=nr; tetC++; }
      else tetTargetRot=tetRot;
    } else if(tetC!=tetTargetCol){                       // 2) slide toward target column
      int dir=tetTargetCol>tetC?1:-1; int8_t rc[4][2]; tetCells(tetShape,tetRot,rc);
      if(tetFitsCells(rc,tetR,tetC+dir)) tetC+=dir; else tetTargetCol=tetC;
    } else if(tetFitsCells(cur,tetR+1,tetC)){            // 3) fall
      tetR++;
    } else {                                            // lock; if rows completed, start the flash
      for(int i=0;i<4;i++){ int rr=tetR+cur[i][0],cc=tetC+cur[i][1]; if(rr>=0&&rr<=10&&cc>=0&&cc<=10) tetB[rr][cc]=tetHue; }
      bool any=false;
      for(int r=0;r<11;r++){ bool full=true; for(int c=0;c<11;c++) if(!tetB[r][c]) full=false; tetFull[r]=full; if(full) any=true; }
      if(any){ tetClearing=true; tetFlashCnt=4; tetFlashT=at; }   // 4 toggles → two white flashes
      else tetNew();
    }
  }
  clearFrame();
  for(int r=0;r<11;r++) for(int c=0;c<11;c++) if(tetB[r][c]) fillCell(r,c,CRGB(CHSV(tetB[r][c],220,160)));
  if(tetClearing){
    if(tetFlashCnt&1) for(int r=0;r<11;r++) if(tetFull[r]) for(int c=0;c<11;c++) fillCell(r,c,CRGB(255,255,255));
  } else {
    int8_t cur[4][2]; tetCells(tetShape,tetRot,cur);
    for(int i=0;i<4;i++){ int rr=tetR+cur[i][0], cc=tetC+cur[i][1]; if(rr>=0) fillCell(rr,cc,CRGB(CHSV(tetHue,255,235))); }
  }
  showGrid();
}

// ═══════════ 40. SNAKE (self-playing) ═══════════
bool snakeInit=false;
static int8_t snR[121],snC[121];
static int    snLen,snDir;
static int8_t sfR,sfC;
static uint32_t snStep;
const int8_t SN_DR[4]={-1,0,1,0}, SN_DC[4]={0,1,0,-1};
bool snOccupied(int r,int c,int upto){
  for(int i=0;i<upto;i++) if(snR[i]==r&&snC[i]==c) return true;
  return false;
}
void snakeFood(){
  for(int tries=0;tries<200;tries++){
    int r=random(11),c=random(11);
    if(!snOccupied(r,c,snLen)){ sfR=r; sfC=c; return; }
  }
}
void snakeReset(){
  snLen=3; snDir=1;
  for(int i=0;i<snLen;i++){ snR[i]=5; snC[i]=5-i; }
  snakeFood();
}
void anim_snake(uint32_t at,float dt){
  if(!snakeInit){ snakeReset(); snStep=at; snakeInit=true; }
  if(at-snStep>=140){
    snStep=at;
    int best=-1,bestDist=999;
    for(int d=0;d<4;d++){
      if(d==(snDir+2)%4) continue;              // no reversing
      int nr=snR[0]+SN_DR[d], nc=snC[0]+SN_DC[d];
      if(nr<0||nr>10||nc<0||nc>10) continue;
      if(snOccupied(nr,nc,snLen-1)) continue;    // tail cell will vacate
      int dist=iabs(nr-sfR)+iabs(nc-sfC);
      if(dist<bestDist){ bestDist=dist; best=d; }
    }
    if(best<0){ snakeReset(); }
    else {
      snDir=best;
      int nr=snR[0]+SN_DR[best], nc=snC[0]+SN_DC[best];
      bool ate=(nr==sfR&&nc==sfC);
      if(ate&&snLen<120) snLen++;
      for(int i=snLen-1;i>0;i--){ snR[i]=snR[i-1]; snC[i]=snC[i-1]; }
      snR[0]=nr; snC[0]=nc;
      if(ate) snakeFood();
    }
  }
  clearFrame();
  fillCell(sfR,sfC,CRGB(CHSV(0,255,220)));                 // food = red
  for(int i=0;i<snLen;i++){
    uint8_t v=(i==0)?255:(uint8_t)(90+120.0f*(1.0f-(float)i/snLen));
    fillCell(snR[i],snC[i],CRGB(CHSV(96,i==0?160:220,v)));  // body = green
  }
  showGrid();
}

// ═══════════ 41. ICON SHOW ═══════════
void anim_iconShow(uint32_t at){
  const uint32_t per=2600;
  int idx=(at/per)%NUM_ICONS;
  uint32_t ph=at%per;
  float env = (ph<400)?ph/400.0f : (ph>per-500)?(per-ph)/500.0f : 1.0f;
  uint8_t val=(uint8_t)constrain(env*230.0f,0.0f,230.0f);
  uint8_t hue=(uint8_t)(at/22);
  clearFrame();
  if(val>4) drawIcon(ICONS[idx],CRGB(CHSV(hue,235,val)));
  showGrid();
}

// ═══════════ 42. BOUNCING LOGO ═══════════
bool logoInit=false;
static float logoX,logoY,logoVX,logoVY;
static uint8_t logoHue;
const uint8_t LOGO[5]={0b01010,0b11111,0b11111,0b01110,0b00100}; // 5x5 heart
void anim_bouncingLogo(uint32_t at,float dt){
  if(!logoInit){ logoX=3;logoY=2;logoVX=0.017f;logoVY=0.012f;logoHue=random(256);logoInit=true; }
  logoX+=logoVX*dt; logoY+=logoVY*dt;
  const int w=5;
  if(logoX<0){ logoX=0; logoVX=-logoVX; logoHue+=43; }
  if(logoX>11-w){ logoX=11-w; logoVX=-logoVX; logoHue+=43; }
  if(logoY<0){ logoY=0; logoVY=-logoVY; logoHue+=43; }
  if(logoY>11-w){ logoY=11-w; logoVY=-logoVY; logoHue+=43; }
  clearFrame();
  int ox=(int)(logoX+0.5f), oy=(int)(logoY+0.5f);
  for(int r=0;r<5;r++) for(int c=0;c<5;c++)
    if(LOGO[r]&(1<<(4-c))) fillCell(oy+r,ox+c,CRGB(CHSV(logoHue,255,215)));
  showGrid();
}

// ═══════════ 43. STAR TUNNEL 3D ═══════════
bool stunInit=false;
static float stA[STUN_N],stRad[STUN_N];
void anim_starTunnel3D(uint32_t at,float dt){
  if(!stunInit){ for(int i=0;i<STUN_N;i++){ stA[i]=frand()*6.2832f; stRad[i]=frand(); } stunInit=true; }
  clearFrame();
  for(int i=0;i<STUN_N;i++){
    stRad[i]+=dt*0.00016f*(stRad[i]*2.5f+0.25f);       // accelerate outward
    if(stRad[i]>1.05f){ stRad[i]=0.04f; stA[i]=frand()*6.2832f; }
    float rr=stRad[i], ca=cosf(stA[i]), sa=sinf(stA[i]);
    int cx=(int)(5.0f+ca*rr*5.6f+0.5f), cy=(int)(5.0f+sa*rr*5.6f+0.5f);
    uint8_t val=(uint8_t)(30+rr*225), hue=(uint8_t)(160+rr*80);
    int tx=(int)(5.0f+ca*(rr-0.13f)*5.6f+0.5f), ty=(int)(5.0f+sa*(rr-0.13f)*5.6f+0.5f);
    fillCell(ty,tx,CRGB(CHSV(hue,150,val/3)));           // dim trail
    fillCell(cy,cx,CRGB(CHSV(hue,120,val)));             // bright head
  }
  showGrid();
}

// ═══════════ 44. RIPPLE RAIN ═══════════
bool rripInit=false;
static int8_t rpR[RRIP_N],rpC[RRIP_N];
static float  rpRad[RRIP_N];
static bool   rpAct[RRIP_N];
static uint8_t rpHue[RRIP_N];
static uint32_t rripSpawn;
void anim_rippleRain(uint32_t at,float dt){
  if(!rripInit){ for(int i=0;i<RRIP_N;i++) rpAct[i]=false; rripSpawn=at; rripInit=true; }
  if(at-rripSpawn>=340){
    rripSpawn=at;
    for(int i=0;i<RRIP_N;i++) if(!rpAct[i]){ rpAct[i]=true; rpR[i]=1+random(9); rpC[i]=1+random(9); rpRad[i]=0; rpHue[i]=random(256); break; }
  }
  clearFrame();
  for(int i=0;i<RRIP_N;i++){
    if(!rpAct[i]) continue;
    rpRad[i]+=dt*0.006f;
    if(rpRad[i]>8.0f){ rpAct[i]=false; continue; }
    int R=(int)(rpRad[i]+0.5f);
    uint8_t val=(uint8_t)constrain(220.0f*(1.0f-rpRad[i]/8.0f),0.0f,220.0f);
    for(int r=0;r<11;r++) for(int c=0;c<11;c++)
      if(max(iabs(r-rpR[i]),iabs(c-rpC[i]))==R) fillCell(r,c,CRGB(CHSV(rpHue[i],200,val)));
  }
  showGrid();
}

// ═══════════ 45. SPIRAL GALAXY ═══════════
void anim_spiralGalaxy(uint32_t t){
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){
    float x=X_POS[xi];
    for(int n=1;n<=121;n++){
      float y=(float)n-CENTER, r=sqrtf(x*x+y*y)/MAX_DIST, ang=atan2f(y,x);
      float arm=sinf(ang*2.0f - r*7.5f + tf*1.6f)*0.5f+0.5f;
      float v=arm*(1.0f-r*0.55f); if(r<0.14f) v+=(0.14f-r)*5.0f; // core glow
      v=constrain(v,0.0f,1.0f); v*=v;
      uint8_t b=(uint8_t)(v*230.0f);
      uint8_t hue=(uint8_t)(165+r*70.0f+tf*9.0f);
      setLED(true,xi+1,n, b>3?CRGB(CHSV(hue,r<0.14f?90:210,b)):CRGB::Black);
    }
  }
  for(int yi=0;yi<12;yi++){
    float yy=Y_POS[yi];
    for(int n=1;n<=121;n++){
      float x=(float)n-CENTER, r=sqrtf(x*x+yy*yy)/MAX_DIST, ang=atan2f(yy,x);
      float arm=sinf(ang*2.0f - r*7.5f + tf*1.6f)*0.5f+0.5f;
      float v=arm*(1.0f-r*0.55f); if(r<0.14f) v+=(0.14f-r)*5.0f;
      v=constrain(v,0.0f,1.0f); v*=v;
      uint8_t b=(uint8_t)(v*230.0f);
      uint8_t hue=(uint8_t)(165+r*70.0f+tf*9.0f);
      setLED(false,yi+1,n, b>3?CRGB(CHSV(hue,r<0.14f?90:210,b)):CRGB::Black);
    }
  }
  showGrid();
}

// ═══════════ 46. PLASMA FIELD ═══════════
void anim_plasmaField(uint32_t t){
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){
    float x=X_POS[xi];
    for(int n=1;n<=121;n++){
      float y=(float)n-CENTER;
      float v=sinf(x*0.09f+tf)+sinf(y*0.09f+tf*1.1f)
             +sinf((x+y)*0.07f+tf*0.8f)+sinf(sqrtf(x*x+y*y)*0.10f-tf*1.3f);
      float nv=(v+4.0f)/8.0f;
      uint8_t hue=(uint8_t)(nv*255.0f+tf*20.0f);
      setLED(true,xi+1,n,CRGB(CHSV(hue,230,200)));
    }
  }
  for(int yi=0;yi<12;yi++){
    float yy=Y_POS[yi];
    for(int n=1;n<=121;n++){
      float x=(float)n-CENTER;
      float v=sinf(x*0.09f+tf)+sinf(yy*0.09f+tf*1.1f)
             +sinf((x+yy)*0.07f+tf*0.8f)+sinf(sqrtf(x*x+yy*yy)*0.10f-tf*1.3f);
      float nv=(v+4.0f)/8.0f;
      uint8_t hue=(uint8_t)(nv*255.0f+tf*20.0f);
      setLED(false,yi+1,n,CRGB(CHSV(hue,230,200)));
    }
  }
  showGrid();
}

// ═══════════ 47. BREATHE GRADIENT (calm) ═══════════
void anim_breatheGradient(uint32_t t){
  float tf=t*0.001f;
  float breath=0.45f+0.55f*(0.5f+0.5f*sinf(tf*0.6f));
  uint8_t base=(uint8_t)(tf*6.0f);
  for(int xi=0;xi<12;xi++){
    float x=X_POS[xi];
    for(int n=1;n<=121;n++){
      float y=(float)n-CENTER;
      uint8_t hue=(uint8_t)(base+(x+y)*0.9f);
      uint8_t b=(uint8_t)(breath*200.0f);
      setLED(true,xi+1,n,CRGB(CHSV(hue,190,b)));
    }
  }
  for(int yi=0;yi<12;yi++){
    float yy=Y_POS[yi];
    for(int n=1;n<=121;n++){
      float x=(float)n-CENTER;
      uint8_t hue=(uint8_t)(base+(x+yy)*0.9f);
      uint8_t b=(uint8_t)(breath*200.0f);
      setLED(false,yi+1,n,CRGB(CHSV(hue,190,b)));
    }
  }
  showGrid();
}

// ═══════════ 48. BOIDS (flocking) ═══════════
bool boidInit=false;
static float bx[BOID_N],by[BOID_N],bvx[BOID_N],bvy[BOID_N];
void anim_boids(uint32_t at,float dt){
  if(!boidInit){ for(int i=0;i<BOID_N;i++){ bx[i]=frand()*100-50; by[i]=frand()*100-50; bvx[i]=frand()*0.04f-0.02f; bvy[i]=frand()*0.04f-0.02f; } boidInit=true; }
  for(int i=0;i<BOID_N;i++){
    float cx=0,cy=0,vx=0,vy=0,sx=0,sy=0; int cnt=0;
    for(int j=0;j<BOID_N;j++){
      if(j==i) continue;
      float ddx=bx[j]-bx[i], ddy=by[j]-by[i], d2=ddx*ddx+ddy*ddy;
      cx+=bx[j]; cy+=by[j]; vx+=bvx[j]; vy+=bvy[j]; cnt++;
      if(d2<90.0f){ sx-=ddx; sy-=ddy; }
    }
    if(cnt){ cx/=cnt; cy/=cnt; vx/=cnt; vy/=cnt;
      bvx[i]+=(cx-bx[i])*0.0006f+(vx-bvx[i])*0.03f+sx*0.0018f;
      bvy[i]+=(cy-by[i])*0.0006f+(vy-bvy[i])*0.03f+sy*0.0018f;
    }
    float sp=sqrtf(bvx[i]*bvx[i]+bvy[i]*bvy[i]);
    if(sp>0.05f){ bvx[i]=bvx[i]/sp*0.05f; bvy[i]=bvy[i]/sp*0.05f; }
    if(sp<0.012f&&sp>0){ bvx[i]=bvx[i]/sp*0.012f; bvy[i]=bvy[i]/sp*0.012f; }
    bx[i]+=bvx[i]*dt; by[i]+=bvy[i]*dt;
    if(bx[i]<-55) bx[i]=55; if(bx[i]>55) bx[i]=-55;
    if(by[i]<-55) by[i]=55; if(by[i]>55) by[i]=-55;
  }
  clearFrame();
  for(int i=0;i<BOID_N;i++){
    int col=(int)((bx[i]+55.0f)/110.0f*10.0f+0.5f);
    int row=(int)((by[i]+55.0f)/110.0f*10.0f+0.5f);
    uint8_t hue=(uint8_t)((atan2f(bvy[i],bvx[i])+3.1416f)*40.0f);
    fillCell(row,col,CRGB(CHSV(hue,220,220)));
  }
  showGrid();
}

// ═══════════ 49. FIREWORKS ═══════════
bool fwInit=false;
static float fpx[FW_N],fpy[FW_N],fpvx[FW_N],fpvy[FW_N],fpl[FW_N];
static uint8_t fphue[FW_N];
static uint32_t fwSpawn;
void anim_fireworks(uint32_t at,float dt){
  if(!fwInit){ for(int i=0;i<FW_N;i++) fpl[i]=0; fwSpawn=at; fwInit=true; }
  if(at-fwSpawn>=800){
    fwSpawn=at;
    float bx0=2+frand()*7, by0=1+frand()*4; uint8_t h=random(256);
    int made=0;
    for(int i=0;i<FW_N&&made<14;i++) if(fpl[i]<=0){
      float ang=frand()*6.2832f, spd=0.006f+frand()*0.014f;
      fpx[i]=bx0; fpy[i]=by0; fpvx[i]=cosf(ang)*spd; fpvy[i]=sinf(ang)*spd;
      fpl[i]=1.0f; fphue[i]=h+random(20); made++;
    }
  }
  clearFrame();
  for(int i=0;i<FW_N;i++){
    if(fpl[i]<=0) continue;
    fpvy[i]+=0.00003f*dt;               // gravity (downward = +row)
    fpx[i]+=fpvx[i]*dt; fpy[i]+=fpvy[i]*dt;
    fpl[i]-=dt*0.0009f;
    int col=(int)(fpx[i]+0.5f), row=(int)(fpy[i]+0.5f);
    uint8_t val=(uint8_t)constrain(fpl[i]*235.0f,0.0f,235.0f);
    fillCell(row,col,CRGB(CHSV(fphue[i],230,val)));
  }
  showGrid();
}

// ═══════════ 50. REACTION-DIFFUSION (Gray-Scott lite) ═══════════
bool rdInit=false;
static float rdA[11][11],rdB[11][11],rdNA[11][11],rdNB[11][11];
void anim_reactionDiff(uint32_t at,float dt){
  if(!rdInit){
    for(int r=0;r<11;r++) for(int c=0;c<11;c++){ rdA[r][c]=1.0f; rdB[r][c]=0.0f; }
    for(int r=4;r<=6;r++) for(int c=4;c<=6;c++){ rdA[r][c]=0.0f; rdB[r][c]=0.9f; }
    rdInit=true;
  }
  const float Da=1.0f,Db=0.5f,f=0.055f,k=0.062f;
  for(int iter=0;iter<2;iter++){
    for(int r=0;r<11;r++) for(int c=0;c<11;c++){
      int rm=(r+10)%11, rp=(r+1)%11, cm=(c+10)%11, cp=(c+1)%11; // toroidal
      float lapA=rdA[rm][c]+rdA[rp][c]+rdA[r][cm]+rdA[r][cp]-4.0f*rdA[r][c];
      float lapB=rdB[rm][c]+rdB[rp][c]+rdB[r][cm]+rdB[r][cp]-4.0f*rdB[r][c];
      float ab=rdA[r][c]*rdB[r][c]*rdB[r][c];
      rdNA[r][c]=constrain(rdA[r][c]+(Da*lapA-ab+f*(1.0f-rdA[r][c])),0.0f,1.0f);
      rdNB[r][c]=constrain(rdB[r][c]+(Db*lapB+ab-(k+f)*rdB[r][c]),0.0f,1.0f);
    }
    for(int r=0;r<11;r++) for(int c=0;c<11;c++){ rdA[r][c]=rdNA[r][c]; rdB[r][c]=rdNB[r][c]; }
  }
  clearFrame();
  for(int r=0;r<11;r++) for(int c=0;c<11;c++){
    uint8_t val=(uint8_t)constrain(rdB[r][c]*640.0f,0.0f,225.0f);
    if(val>4) fillCell(r,c,CRGB(CHSV((uint8_t)(150+rdB[r][c]*260.0f),210,val)));
  }
  showGrid();
}
