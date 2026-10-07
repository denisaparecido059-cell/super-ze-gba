// Super Zé Advance - ROM real de GBA (C puro, sem bibliotecas)
typedef unsigned char u8; typedef unsigned short u16;
#define R16(a) (*(volatile u16*)(a))
#define DISPCNT R16(0x4000000)
#define VCOUNT  R16(0x4000006)
#define BG0CNT  R16(0x4000008)
#define BG0HOFS R16(0x4000010)
#define KEYS    R16(0x4000130)
#define PAL  ((volatile u16*)0x5000000)
#define OPAL ((volatile u16*)0x5000200)
#define BGT  ((volatile u16*)0x6000000)
#define MAP  ((volatile u16*)0x600E000)
#define OBJT ((volatile u16*)0x6010000)
#define OAM  ((volatile u16*)0x7000000)
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
#define SKY RGB(13,23,30)
#define W 120

static u8 g[10][W];
static int cam, loaded, wint;
static struct { int x, y, vx, vy, gr, f; } p;
static struct { int x, y, vx, vy, dead; } e[10];

static void pix(volatile u16 *b, int t, int x, int y, int c) {
  volatile u16 *a = b + t * 16 + y * 2 + (x >> 2); int s = (x & 3) * 4;
  *a = (*a & ~(15 << s)) | (c << s);
}
static int sol(int x, int y) {
  int tx = x >> 4, ty = y >> 4;
  if (x < 0 || tx >= W) return 1;
  if (y < 0 || ty >= 10) return 0;
  return g[ty][tx] >= 1 && g[ty][tx] <= 7;
}
static void putcol(int c) {
  for (int sx = 0; sx < 2; sx++) {
    int m = (c * 2 + sx) & 63, b = m < 32 ? 0 : 0x400;
    for (int ty = 0; ty < 20; ty++) {
      int id = g[ty >> 1][c];
      MAP[b + ty * 32 + (m & 31)] = id ? id * 4 + (ty & 1) * 2 + sx : 0;
    }
  }
}
static int mc(int id, int x, int y) {
  int edge = (x == 0 || y == 0 || x == 15 || y == 15), c = 0;
  switch (id) {
    case 1: c = (x == 0 || y == 0) ? 2 : 1; break;
    case 2: c = y < 4 ? 3 : (x == 0 ? 2 : 1); break;
    case 3: c = (y == 0 || y == 7 || y == 8 || y == 15 || x == 0 || x == (y < 8 ? 8 : 2)) ? 11 : 4; break;
    case 4: c = edge ? 2 : 5;
      if ((x >= 5 && x <= 10 && (y == 3 || y == 4)) || (x >= 9 && x <= 10 && y >= 5 && y <= 7) ||
          (x >= 7 && x <= 9 && y == 8) || (x >= 7 && x <= 8 && (y == 9 || y == 10 || y == 12 || y == 13))) c = 2;
      break;
    case 5: c = edge ? 11 : 2; break;
    case 6: c = (x == 0 || x == 15) ? 11 : (x < 4 ? 7 : 6); break;
    case 7: c = edge ? 11 : 8; break;
    case 8: c = (x >= 7 && x <= 8) ? 9 : 0; break;
    case 9: c = (x >= 7 && x <= 8) ? 9 : ((x < 7 && y >= 2 && y <= 10) ? 10 : 0); break;
    case 10: c = ((x - 8) * (x - 8) * 4 + (y - 8) * (y - 8) <= 64) ? 5 : 0; break;
  }
  return c;
}
static const char *PL[16] = {
 "................","................","....rrrrrr......","...rrrrrrrrr....",
 "...sssskss......","..ssssskssss....","...sssssssss....","....rrbrrr......",
 "...rrrbrrbrr....","..rrrbbbbbbrr...","..ssbbbbbbss....","...bbbbbbbb.....",
 "...bbb..bbb.....","..nnn....nnn....","..nnnn..nnnn....","................"};
static const char *EN[16] = {
 "................","................","................","................",
 "......nnnn......","....nnnnnnnn....","...nnnnnnnnnn...","..nnwwnnnnwwnn..",
 "..nnwknnnnkwnn..","..nnnnnnnnnnnn..","...nnnnnnnnnn...","....ssssssss....",
 "....kkk..kkk....","...kkkk..kkkk...","................","................"};
static void gfx(void) {
  static const u16 bp[12] = {SKY, RGB(22,12,3), RGB(14,8,2), RGB(8,22,8), RGB(26,13,4), RGB(31,25,0),
    RGB(3,19,7), RGB(14,28,17), RGB(19,19,21), RGB(28,28,28), RGB(31,8,8), 0};
  static const u16 op[7] = {0, RGB(29,4,4), RGB(30,24,16), RGB(5,10,27), 0, RGB(17,9,2), RGB(31,31,31)};
  static const char cm[] = ".rsbknw";
  for (int i = 0; i < 12; i++) PAL[i] = bp[i];
  for (int i = 0; i < 7; i++) OPAL[i] = op[i];
  for (int i = 0; i < 48 * 16; i++) BGT[i] = 0;
  for (int i = 0; i < 8 * 16; i++) OBJT[i] = 0;
  for (int id = 1; id <= 10; id++)
    for (int sy = 0; sy < 2; sy++) for (int sx = 0; sx < 2; sx++)
      for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++)
        pix(BGT, id * 4 + sy * 2 + sx, x, y, mc(id, sx * 8 + x, sy * 8 + y));
  for (int s = 0; s < 2; s++) for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
    char ch = (s ? EN : PL)[y][x]; int c = 0;
    for (int k = 0; k < 7; k++) if (cm[k] == ch) c = k;
    pix(OBJT, s * 4 + (y >> 3) * 2 + (x >> 3), x & 7, y & 7, c);
  }
}
static void init(void) {
  static const u8 gp[4][2] = {{22,24},{45,48},{70,72},{88,91}};
  static const u8 pp[3][2] = {{30,2},{54,3},{76,2}};
  static const u8 cc[10][2] = {{13,3},{15,3},{26,5},{40,3},{66,3},{82,2},{92,5},{60,6},{34,6},{44,6}};
  static const u8 ex[10] = {18,26,34,42,50,62,68,80,86,96};
  for (int i = 0; i < 2048; i++) MAP[i] = 0;
  for (int y = 0; y < 10; y++) for (int x = 0; x < W; x++) g[y][x] = 0;
  for (int x = 0; x < W; x++) {
    int hole = 0; for (int i = 0; i < 4; i++) if (x >= gp[i][0] && x <= gp[i][1]) hole = 1;
    if (!hole) { g[8][x] = 2; g[9][x] = 1; }
  }
  for (int x = 12; x <= 16; x++) g[5][x] = 3;
  g[5][13] = g[5][15] = 4;
  for (int x = 38; x <= 42; x++) g[5][x] = 3;
  g[5][40] = 4;
  for (int x = 64; x <= 68; x++) g[5][x] = 3;
  g[5][65] = g[5][67] = 4;
  for (int x = 80; x <= 84; x++) g[4][x] = 3;
  g[4][82] = 4;
  for (int i = 0; i < 3; i++) for (int j = 0; j < pp[i][1]; j++) g[7 - j][pp[i][0]] = g[7 - j][pp[i][0] + 1] = 6;
  for (int i = 0; i < 5; i++) for (int j = 0; j <= i; j++) g[7 - j][100 + i] = 7;
  for (int y = 3; y < 8; y++) g[y][112] = 8;
  g[2][112] = 9;
  for (int i = 0; i < 10; i++) g[cc[i][1]][cc[i][0]] = 10;
  for (int i = 0; i < 10; i++) { e[i].x = ex[i] * 16 * 256; e[i].y = 112 * 256; e[i].vx = -102; e[i].vy = 0; e[i].dead = 0; }
  p.x = 32 * 256; p.y = 100 * 256; p.vx = p.vy = p.gr = p.f = 0;
  cam = 0; loaded = -1; wint = 0;
  for (int i = 0; i < 17; i++) putcol(++loaded);
}
static void step(void) {
  int k = ~KEYS, mx = (k & 2) ? 538 : 358;
  if (wint) { if (--wint == 0) { PAL[0] = SKY; init(); } else PAL[0] = (wint & 8) ? RGB(31,31,31) : SKY; return; }
  if (k & 32) { p.vx -= 41; p.f = 1; }
  if (k & 16) { p.vx += 41; p.f = 0; }
  if (!(k & 48)) p.vx = p.vx * 210 >> 8;
  if (p.vx > mx) p.vx = mx;
  if (p.vx < -mx) p.vx = -mx;
  if ((k & 1) && p.gr) { p.vy = -1254; p.gr = 0; }
  if (!(k & 1) && p.vy < -512) p.vy = -512;
  p.vy += 56; if (p.vy > 1280) p.vy = 1280;
  p.x += p.vx;
  int px = p.x >> 8, py = p.y >> 8;
  for (int i = 0; i < 2; i++) {
    int y = py + 1 + i * 12;
    if (p.vx > 0 && sol(px + 10, y)) { px = (((px + 10) >> 4) << 4) - 10; p.x = px << 8; p.vx = 0; }
    else if (p.vx < 0 && sol(px, y)) { px = ((px >> 4) + 1) << 4; p.x = px << 8; p.vx = 0; }
  }
  if (px < cam) { px = cam; p.x = px << 8; p.vx = 0; }
  p.y += p.vy; py = p.y >> 8; p.gr = 0;
  if (p.vy > 0) {
    if (sol(px + 1, py + 14) || sol(px + 9, py + 14)) { py = (((py + 14) >> 4) << 4) - 14; p.y = py << 8; p.vy = 0; p.gr = 1; }
  } else if (p.vy < 0 && (sol(px + 1, py) || sol(px + 9, py))) {
    int tx = (px + 5) >> 4, ty = py >> 4;
    py = (ty + 1) << 4; p.y = py << 8; p.vy = 0;
    if (ty >= 0 && ty < 10 && g[ty][tx] == 4) { g[ty][tx] = 5; putcol(tx); }
  }
  { int tx = (px + 5) >> 4, ty = (py + 7) >> 4;
    if (ty >= 0 && ty < 10 && tx < W) {
      int t = g[ty][tx];
      if (t == 10) { g[ty][tx] = 0; putcol(tx); }
      if (t == 8 || t == 9) wint = 120;
    } }
  if (py > 170) { init(); return; }
  for (int i = 0; i < 10; i++) {
    if (e[i].dead) continue;
    int ex = e[i].x >> 8, d = ex - cam;
    if (d < -40 || d > 280) continue;
    e[i].x += e[i].vx; ex = e[i].x >> 8;
    if (sol(ex + (e[i].vx > 0 ? 12 : 0), (e[i].y >> 8) + 10)) e[i].vx = -e[i].vx;
    e[i].vy += 51; e[i].y += e[i].vy; int ey = e[i].y >> 8;
    if (sol(ex + 6, ey + 12)) { ey = (((ey + 12) >> 4) << 4) - 12; e[i].y = ey << 8; e[i].vy = 0; }
    if (ey > 170) { e[i].dead = 1; continue; }
    if (px < ex + 12 && px + 10 > ex && py < ey + 12 && py + 14 > ey) {
      if (p.vy > 0 && py + 14 - ey < 10) { e[i].dead = 1; p.vy = -820; }
      else { init(); return; }
    }
  }
  if (px - 100 > cam) cam = px - 100;
  if (cam > W * 16 - 240) cam = W * 16 - 240;
  while (loaded < W - 1 && loaded * 16 < cam + 256) putcol(++loaded);
}
static void render(void) {
  BG0HOFS = cam & 511;
  int sx = (p.x >> 8) - cam - 3, sy = (p.y >> 8) - 1;
  OAM[0] = sy & 255; OAM[1] = (sx & 511) | (1 << 14) | (p.f ? 1 << 12 : 0); OAM[2] = 0;
  for (int i = 0; i < 10; i++) {
    sx = (e[i].x >> 8) - cam - 2; sy = (e[i].y >> 8) - 2;
    if (e[i].dead || sx < -16 || sx > 240 || sy > 160) OAM[(i + 1) * 4] = 0x200;
    else { OAM[(i + 1) * 4] = sy & 255; OAM[(i + 1) * 4 + 1] = (sx & 511) | (1 << 14); OAM[(i + 1) * 4 + 2] = 4; }
  }
}
int main(void) {
  for (int i = 0; i < 128; i++) { OAM[i * 4] = 0x200; OAM[i * 4 + 1] = 0; OAM[i * 4 + 2] = 0; }
  gfx();
  BG0CNT = (28 << 8) | (1 << 14);
  DISPCNT = 0x1140;
  init();
  for (;;) {
    while (VCOUNT >= 160); while (VCOUNT < 160);
    render(); step();
  }
}
