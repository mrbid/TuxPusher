/*
    James William Fletcher (github.com/mrbid)
        July 2022 - January 2023

    https://SeaPusher.com & https://pusha.one

    A highly optimised and efficient coin pusher
    implementation.

    This was not made in a way that is easily extensible,
    the code can actually seem kind of confusing to
    initially understand.

    Unlike the CoinPusher/SeaPusher release;
    https://github.com/mrbid/coinpusher this version does not
    allow custom framerates, just whatever your screen refresh
    rate is set to. I just wanted to be a little simple/regular
    in this release.

    Also in this one I step the collisions 6 times per frame
    rather than just once, which is 6x better quality, this
    prevents noticable overlap at higher input speeds which
    can now be set via the second argv.
*/

#include <SDL2/SDL_mouse.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef BUILD_GLFW
    #include "inc/gl.h"
    #define GLFW_INCLUDE_NONE
    #include "inc/glfw3.h"
    #define GL_DEBUG
#else
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_opengles2.h>
#endif

#ifndef __x86_64__
    #define NOSSE
#endif

// uncomment if you want that little bit more performance
//#define FAST_RAND
#ifdef FAST_RAND
    #define SEIR_RAND
#endif

#include "esAux2.h"
#include "res.h"

#include "assets/scene.h"
#include "assets/coin.h"
#include "assets/coin_silver.h"
#include "assets/tux.h"
#include "assets/evil.h"
#include "assets/king.h"
#include "assets/ninja.h"
#include "assets/surf.h"
#include "assets/trip.h"
#include "assets/gameover.h"
#include "assets/rx.h"
#include "assets/sa.h"
#include "assets/ga.h"

#include "assets/console_menus.h"

#define uint GLushort // it's a short don't forget that
#define sint GLshort  // and this.
#define f32 GLfloat
#define forceinline __attribute__((always_inline)) inline

//*************************************
// globals
//*************************************
char appTitle[] = "TuxPusher.com";
#ifdef BUILD_GLFW
    GLFWwindow* wnd;
#else
    SDL_Window* wnd;
    SDL_GLContext glc;
    SDL_Surface* s_icon = NULL;
    SDL_Cursor* cross_cursor;
    SDL_Cursor* beam_cursor;
#endif
uint cursor_state = 0;
uint winw = 1024, winh = 768;
f32 aspect;
f32 t = 0.f;
f32 dt = 0;   // delta time
f32 fc = 0;   // frame count
f32 lfct = 0; // last frame count time
f32 rww, ww, rwh, wh, ww2, wh2;
f32 uw, uh, uw2, uh2; // normalised pixel dpi
f32 touch_margin = 120.f;
f32 mx=0, my=0;
uint md=0;
f32 rst = 0.f;
uint ortho = 0;

// render state id's
GLint projection_id;
GLint modelview_id;
GLint position_id;
GLint lightpos_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// render state inputs
vec lightpos = {0.f, 10.f, 13.f};
int csp = -1; // current shader program

ESModel mdlPlane;
ESModel mdlGameover;
ESModel mdlScene;
ESModel mdlCoin;
ESModel mdlCoinSilver;
ESModel mdlTux;
ESModel mdlEvil;
ESModel mdlKing;
ESModel mdlSurf;
ESModel mdlNinja;
ESModel mdlTrip;
ESModel mdlRX;
ESModel mdlSA;
ESModel mdlGA;

// game vars
f32 gold_stack = 64.f;  // line 740+ defining these as float32 eliminates the need to cast in mTranslate()
f32 silver_stack = 64.f;// function due to the use of a float32 also in the for(f32 i;) loop.
uint active_coin = 0;
uint inmotion = 0;
f32 gameover = 0.f;
uint isnewcoin = 0;
f32 PUSH_SPEED = 1.6f;

typedef struct
{
    f32 x, y, r;
    signed char color;
} coin; // 4+4+4+1 = 13 bytes, 3 bytes padding to 16 byte cache line
#define MAX_COINS 130
coin coins[MAX_COINS] = {0};

// Bit flag based method for storing trophie states, 
// 0b0000_0000 where the last bit denotes 1, second last denotes 2 etc like so:
// 0b0000_0001 = trophie 1
// 0b0000_0101 = trophie 1 and 3
char trophies_bits = 0; 
#define trophies_set(x) trophies_bits |= (0b1 << x)
#define trophies_clear() trophies_bits = 0
#define trophies_get(x) (trophies_bits >> x) & 0b1
#define trophies_all() trophies_bits


//*************************************
// game functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}

forceinline f32 fRandFloat(const f32 min, const f32 max)
{
#ifdef FAST_RAND
    return min + randf() * (max-min);
#else
    return esRandFloat(min, max);
#endif
}

forceinline int fRand(const f32 min, const f32 max)
{
#ifdef FAST_RAND
    return (int)((min + randf() * (max-min))+0.5f);
#else
    return esRand(min, max);
#endif
}

forceinline f32 f32Time()
{
#ifdef BUILD_GLFW
    return glfwGetTime();
#else
    return ((f32)SDL_GetTicks())*0.001f;
#endif
}

void setActiveCoin(const uint color)
{
    for(int i=0; i < MAX_COINS; i++)
    {
        if(coins[i].color == -1)
        {
            coins[i].color = color;
            active_coin = i;
            return;
        }
    }
}

void takeStack()
{
    if(gameover != 0.f)
        return;
    
    if(silver_stack != 0.f)
    {
        // play a silver coin
        isnewcoin = 1;
        setActiveCoin(0);
        inmotion = 1;
    }
    else if(gold_stack != 0.f)
    {
        // play a gold coin
        isnewcoin = 2;
        setActiveCoin(1);
        inmotion = 1;
    }

    if(inmotion == 1)
    {
        if(mx < touch_margin)
        {
            coins[active_coin].x = -1.90433f;
            coins[active_coin].y = -4.54055f;
            return;
        }
        
        if(mx > ww-touch_margin)
        {
            coins[active_coin].x = 1.90433f;
            coins[active_coin].y = -4.54055f;
            return;
        }
        
        coins[active_coin].x = -1.90433f+(((mx-touch_margin)*rww)*3.80866f);
        coins[active_coin].y = -4.54055f;
    }
}

void injectFigure()
{
    if(inmotion != 0)
        return;
    
    int fcn = -1;
    for(int i=0; i < 3; i++)
    {
        if(coins[i].color == -1)
        {
            active_coin = i;
            fcn = i;
            coins[i].color = fRand(1, 6);
            break;
        }
    }

    if(fcn != -1)
    {
        coins[active_coin].x = fRandFloat(-1.90433f, 1.90433f);
        coins[active_coin].y = -4.54055f;
        inmotion = 1;
    }
}

int insidePitch(const f32 x, const f32 y, const f32 r)
{
    // off bottom
    if(y < -4.03414f+r)
        return 0;
    
    // first left & right
    if(y < -2.22855f)
    {
        if(x < (-2.22482f - (0.77267f*(fabsf(y+4.03414f) * 0.553835588f))) + r)
            return 0;
        else if(x > (2.22482f + (0.77267f*(fabsf(y+4.03414f) * 0.553835588f))) - r)
            return 0;
    }
    else if(y < -0.292027f) // second left & right
    {
        if(x < (-2.99749f - (0.41114f*(fabsf(y+2.22855f) * 0.516389426f))) + r)
            return 0;
        else if(x > (2.99749f + (0.41114f*(fabsf(y+2.22855f) * 0.516389426f))) - r)
            return 0;
    }
    else if(y < 1.45439f) // third left & right
    {
        if(x < -3.40863f + r)
            return 0;
        else if(x > 3.40863f - r)
            return 0;
    }

    return 1;
}

int collision(int ci)
{
    for(int i=0; i < MAX_COINS; i++)
    {
        if(i == ci || coins[i].color == -1){continue;}
        const f32 xm = (coins[i].x - coins[ci].x);
        const f32 ym = (coins[i].y - coins[ci].y);
        const f32 radd = coins[i].r+coins[ci].r;
        if(xm*xm + ym*ym < radd*radd)
            return 1;
    }
    return 0;
}

uint stepCollisions()
{
    uint was_collision = 0;
    for(int i=0; i < MAX_COINS; i++)
    {
        if(coins[i].color == -1){continue;}
        for(int j=0; j < MAX_COINS; j++)
        {
            if(i == j || coins[j].color == -1 || j == active_coin){continue;}
            f32 xm = (coins[i].x - coins[j].x);
            xm += fRandFloat(-0.01f, 0.01f); // add some random offset to our unit vector, very subtle but works so well!
            const f32 ym = (coins[i].y - coins[j].y);
            f32 d = xm*xm + ym*ym;
            const f32 cr = coins[i].r+coins[j].r;
            if(d < cr*cr)
            {
                d = sqrtps(d);
                const f32 len = 1.f/d;
                const f32 uy = (ym * len);
                if(uy > 0.f){continue;} // best hack ever to massively simplify
                const f32 m = d-cr;
                coins[j].x += (xm * len) * m;
                coins[j].y += uy * m;

                // first left & right
                if(coins[j].y < -2.22855f)
                {
                    const f32 fl = (-2.22482f - (0.77267f*(fabsf(coins[j].y+4.03414f) * 0.553835588f))) + coins[j].r;
                    if(coins[j].x < fl)
                    {
                        coins[j].x = fl;
                    }
                    else
                    {
                        const f32 fr = ( 2.22482f + (0.77267f*(fabsf(coins[j].y+4.03414f) * 0.553835588f))) - coins[j].r;
                        if(coins[j].x > fr)
                            coins[j].x = fr;
                    }
                }
                else if(coins[j].y < -0.292027f) // second left & right
                {
                    const f32 fl = (-2.99749f - (0.41114f*(fabsf(coins[j].y+2.22855f) * 0.516389426f))) + coins[j].r;
                    if(coins[j].x < fl)
                    {
                        coins[j].x = fl;
                    }
                    else
                    {
                        const f32 fr = (2.99749f + (0.41114f*(fabsf(coins[j].y+2.22855f) * 0.516389426f))) - coins[j].r;
                        if(coins[j].x > fr)
                            coins[j].x = fr;
                    }
                }
                else if(coins[j].y < 1.64f) // third left & right
                {
                    const f32 fl = -3.40863f + coins[j].r;
                    if(coins[j].x < fl)
                    {
                        coins[j].x = fl;
                    }
                    else
                    {
                        const f32 fr = 3.40863f - coins[j].r;
                        if(coins[j].x > fr)
                            coins[j].x = fr;
                    }
                }
                else if(coins[j].y < 2.58397f) // first house goal
                {
                    const f32 fl = (-3.40863f + (0.41113f*(fabsf(coins[j].y-1.45439f) * 0.885284796f)));
                    if(coins[j].x < fl)
                    {
                        coins[j].color = -1;
                    }
                    else
                    {
                        const f32 fr = (3.40863f - (0.41113f*(fabsf(coins[j].y-1.45439f) * 0.885284796f)));
                        if(coins[j].x > fr)
                            coins[j].color = -1;
                    }
                }
                else if(coins[j].y < 3.70642f) // second house goal
                {
                    const f32 fl = (-2.9975f + (1.34581f*(fabsf(coins[j].y-2.58397f) * 0.890908281f)));
                    if(coins[j].x < fl)
                    {
                        coins[j].color = -1;
                    }
                    else
                    {
                        const f32 fr = (2.9975f - (1.34581f*(fabsf(coins[j].y-2.58397f) * 0.890908281f)));
                        if(coins[j].x > fr)
                            coins[j].color = -1;
                    }
                }
                else if(coins[j].y < 4.10583f) // silver goal
                {
                    const f32 fl = (-1.65169f + (1.067374f*(fabsf(coins[j].y-3.70642f) * 2.503692947f)));
                    if(coins[j].x < fl)
                    {
                        if(j >= 0 && j <= 3)
                        {
                            if(trophies_get(coins[j].color-1)) // already have? then reward coins!
                            {
                                gold_stack += 6.f;
                                silver_stack += 6.f;
                            }
                            else
                                trophies_set(coins[j].color-1);
                        }
                        else
                        {
                            if(coins[j].color == 0)
                                silver_stack += 1.f;
                            else if(coins[j].color == 1)
                                silver_stack += 2.f;
                        }

                        coins[j].color = -1;
                    }
                    else
                    {
                        const f32 fr = (1.65169f - (1.067374f*(fabsf(coins[j].y-3.70642f) * 2.503692947f)));
                        if(coins[j].x > fr)
                        {
                            if(j >= 0 && j <= 3)
                            {
                                if(trophies_get(coins[j].color-1)) // already have? then reward coins!
                                {
                                    gold_stack += 6.f;
                                    silver_stack += 6.f;
                                }
                                else
                                    trophies_set(coins[j].color-1);
                            }
                            else
                            {
                                if(coins[j].color == 0)
                                    silver_stack += 1.f;
                                else if(coins[j].color == 1)
                                    silver_stack += 2.f;
                            }

                            coins[j].color = -1;
                        }
                    }
                }
                else if(coins[j].y >= 4.31457f) // gold goal
                {
                    if(coins[j].x >= -0.584316f && coins[j].x <= 0.584316f)
                    {
                        if(j >= 0 && j <= 3)
                        {
                            if(trophies_get(coins[j].color-1)) // already have? then reward coins!
                            {
                                gold_stack += 6.f;
                                silver_stack += 6.f;
                            }
                            else
                                trophies_set(coins[j].color-1);
                        }
                        else
                        {
                            if(coins[j].color == 0)
                                gold_stack += 1.f;
                            else if(coins[j].color == 1)
                                gold_stack += 2.f;
                        }

                        coins[j].color = -1;
                    }
                    else
                    {
                        if(j >= 0 && j <= 3)
                        {
                            if(trophies_get(coins[j].color-1)) // already have? then reward coins!
                            {
                                gold_stack += 6.f;
                                silver_stack += 6.f;
                            }
                            else
                                trophies_set(coins[j].color-1);
                        }
                        else
                            silver_stack += 1.f;

                        coins[j].color = -1;
                    }
                }
                
                was_collision++;
            }
        }
    }
    return was_collision;
}

void newGame()
{
    // seed randoms
    srand(time(0));

    // defaults
    gold_stack = 64.f;
    silver_stack = 64.f;
    active_coin = 0;
    inmotion = 0;
    gameover = 0.f;
    trophies_clear();
    for(int i=0; i < MAX_COINS; i++)
    {
        coins[i].color = -1;
        coins[i].r = 0.3f;
    }

    // trophies
    for(int i=0; i < 3; i++)
    {
        coins[i].color = fRand(1, 6);
        coins[i].r = 0.36f;

        coins[i].x = fRandFloat(-3.40863f, 3.40863f);
        coins[i].y = fRandFloat(-4.03414f, 1.45439f-coins[i].r);
        while(insidePitch(coins[i].x, coins[i].y, coins[i].r) == 0 || collision(i) == 1)
        {
            coins[i].x = fRandFloat(-3.40863f, 3.40863f);
            coins[i].y = fRandFloat(-4.03414f, 1.45439f-coins[i].r);
        }
    }

    // coins
    const f32 lt = f32Time();
    for(int i=3; i < MAX_COINS; i++)
    {
        coins[i].x = fRandFloat(-3.40863f, 3.40863f);
        coins[i].y = fRandFloat(-4.03414f, 1.45439f-coins[i].r);
        uint tl = 0;
        while(insidePitch(coins[i].x, coins[i].y, coins[i].r) == 0 || collision(i) == 1)
        {
            coins[i].x = fRandFloat(-3.40863f, 3.40863f);
            coins[i].y = fRandFloat(-4.03414f, 1.45439f-coins[i].r);
            if(f32Time()-lt > 0.033){tl=1;break;} // 33ms timeout
        }
        if(tl==1){break;}
        coins[i].color = fRand(0, 4);
        if(coins[i].color > 1){coins[i].color = 0;}
    }

    // const int mc2 = MAX_COINS/2;
    // for(int i=3; i < mc2; i++)
    // {
    //     coins[i].x = fRandFloat(-3.40863f, 3.40863f);
    //     coins[i].y = fRandFloat(-4.03414f, 1.45439f-coins[i].r);
    //     while(insidePitch(coins[i].x, coins[i].y, coins[i].r) == 0 || collision(i) == 1)
    //     {
    //         coins[i].x = fRandFloat(-3.40863f, 3.40863f);
    //         coins[i].y = fRandFloat(-4.03414f, 1.45439f-coins[i].r);
    //     }
    //     coins[i].color = fRand(0, 4);
    //     if(coins[i].color > 1){coins[i].color = 0;}
    // }

    rst = f32Time(); // round start time
}

//*************************************
// render functions
//*************************************
forceinline void modelBind1(const ESModel* mdl)
{
    glBindBuffer(GL_ARRAY_BUFFER, mdl->vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdl->iid);
}

forceinline void modelBind3(const ESModel* mdl)
{
    glBindBuffer(GL_ARRAY_BUFFER, mdl->cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdl->vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdl->nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdl->iid);
}

void doPerspective()
{
    glViewport(0, 0, winw, winh);

    ww = (f32)winw;
    wh = (f32)winh;
    if(ortho == 1){touch_margin = ww*0.3076923192f;}
    else{touch_margin = ww*0.2058590651f;}
    rww = 1.f/(ww-touch_margin*2.f);
    rwh = 1.f/wh;
    ww2 = ww/2.f;
    wh2 = wh/2.f;
    uw = aspect/ww;
    uh = 1.f/wh;
    uw2 = aspect/ww2;
    uh2 = 1.f/wh2;

    mIdent(&projection);

    if(ortho == 1)
        mOrtho(&projection, -5.0f, 5.0f, -3.2f, 3.4f, 2.0f, 30.f);
    else
    {
        if(winw > winh)
            aspect = ww / wh;
        else
            aspect = wh / ww;
        mPerspective(&projection, 30.0f, aspect, 2.0f, 30.f);
    }
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static f32 lt = 0;
    t = f32Time();
    dt = t-lt;
    lt = t;

//*************************************
// input handling
//*************************************
#ifdef BUILD_GLFW
    double tmx, tmy;
    glfwGetCursorPos(wnd, &tmx, &tmy);
    mx = (f32)tmx;
    my = (f32)tmy;
#else
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch (event.type) 
        {
            
            case SDL_MOUSEMOTION:
                mx = (f32)event.motion.x;
                my = (f32)event.motion.y;
                break;
            
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) 
                {
                    case SDL_BUTTON_LEFT:

                        if (inmotion != 0 || event.button.button != SDL_BUTTON_LEFT)
                            break;

                        takeStack();
                        md = 1;

                        if (gameover == 0.f) 
                            break;

                        if(f32Time() <= gameover+3.0f)
                            break;
                        
                        newGame();

                        if(PUSH_SPEED >= 32.f)
                            return;

                        PUSH_SPEED += 1.f;
                        char titlestr[256];
                        sprintf(titlestr, "TuxPusher [%.1f]", PUSH_SPEED);
                        SDL_SetWindowTitle(wnd, titlestr);

                        return;

                    case SDL_BUTTON_RIGHT:
                        static uint cs = 1;
                        cs = 1 - cs;
                        SDL_ShowCursor(cs);
                }
                break;
            
            case SDL_MOUSEBUTTONUP:
                if(event.button.button == SDL_BUTTON_LEFT)
                    md = 0;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) 
                {
                    case SDLK_f:
                        if(t-lfct <= 2.0)
                            break;

                        char strts[16];
                        timestamp(&strts[0]);
                        const f32 nfps = fc/(t-lfct);
                        printf("[%s] FPS: %g\n", strts, nfps);
                        lfct = t;
                        fc = 0;

                        break;

                    case SDLK_c:
                        ortho = 1 - ortho;
                        doPerspective();
                        break;
                }
                break;
            
            case SDL_WINDOWEVENT:
                if(event.window.event != SDL_WINDOWEVENT_RESIZED)
                    break;
                winw = event.window.data1;
                winh = event.window.data2;
                doPerspective();
                break;

            case SDL_QUIT:
                SDL_GL_DeleteContext(glc);
                SDL_FreeSurface(s_icon);
                SDL_FreeCursor(cross_cursor);
                SDL_FreeCursor(beam_cursor);
                SDL_DestroyWindow(wnd);
                SDL_Quit();
                exit(0);
                
        }
        
    }
#endif
    
//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// main render
//*************************************

#ifndef BUILD_GLFW

    // Cursor icon switching based on the mouse position. 
    // Have a cross while over the playing field.
    switch (cursor_state) 
    {
        case 0:
            if (mx <= ww-touch_margin+1.f && mx >= touch_margin-1.f)
                break;
            SDL_SetCursor(beam_cursor);
            cursor_state = 1;
            break;
        case 1:
            if (mx > touch_margin && mx < ww-touch_margin) 
                SDL_SetCursor(cross_cursor);
                cursor_state = 0;
            break;
    }
    
#endif
    
    // camera
    mIdent(&view);
    mTranslate(&view, 0.f, 0.f, -13.f);
    if(ortho == 1)
        mRotY(&view, 50.f*DEG2RAD);
    else
        mRotY(&view, 62.f*DEG2RAD);

    // inject a new figure if time has come
    injectFigure();
    
    // prep scene for rendering
    if(csp != 1)
    {
        shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
        glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
        glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
        csp = 1;
    }
    glUniform1f(opacity_id, 0.148f);

    // render scene
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
    modelBind3(&mdlScene);
    glDrawElements(GL_TRIANGLES, scene_numind, GL_UNSIGNED_SHORT, 0);

    // detect gameover
    if(gold_stack < 0.f){gold_stack = 0.f;}
    if(silver_stack < 0.f){silver_stack = 0.f;}
    if(gameover > 0.f && (gold_stack != 0.f || silver_stack != 0.f))
    {
        gameover = 0.f;
    }
    else if(gold_stack == 0.f && silver_stack == 0.f)
    {
        if(gameover == 0.f)
            gameover = t+3.0f;
    }

    // coin
    glUniform1f(opacity_id, 0.148f);

    // targeting coin
    if(gold_stack > 0.f || silver_stack > 0.f)
    {
        if(coins[active_coin].color == 1)
            modelBind3(&mdlCoinSilver);
        else
            modelBind3(&mdlCoin);
        if(inmotion == 0)
        {
            if(silver_stack > 0.f)
                modelBind3(&mdlCoinSilver);
            else
                modelBind3(&mdlCoin);

            mIdent(&model);
            mScale(&model, 1.f, 1.f, 2.f);

            if(mx < touch_margin)
                mTranslate(&model, -1.90433f, -4.54055f, 0);
            else if(mx > ww-touch_margin)
                mTranslate(&model, 1.90433f, -4.54055f, 0);
            else
                mTranslate(&model, -1.90433f+(((mx-touch_margin)*rww)*3.80866f), -4.54055f, 0);

            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
        }
    }

    // do motion
    if(inmotion == 1)
    {
        if(coins[active_coin].y < -3.73414f)
        {
            coins[active_coin].y += PUSH_SPEED * dt;
            for(int i=0; i < 6; i++) // six seems enough
                stepCollisions();
        }
        else
        {
            inmotion = 0;

            if(isnewcoin > 0)
            {
                if(isnewcoin == 1)
                    silver_stack -= 1.f;
                else
                    gold_stack -= 1.f;

                isnewcoin = 0;
            }
        }
    }

    // gold stack
    modelBind3(&mdlCoin);
    f32 gss = gold_stack;
    if(silver_stack == 0.f){gss -= 1.f;}
    if(gss < 0.f){gss = 0.f;}
    for(f32 i = 0.f; i < gss; i += 1.f)
    {
        mIdent(&model);
        if(ortho == 0)
            mTranslate(&model, -2.62939f, -4.54055f, 0.033f*i);
        else
            mTranslate(&model, -4.62939f, -4.54055f, 0.033f*i);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
    }

    // silver stack
    modelBind3(&mdlCoinSilver);
    f32 sss = silver_stack-1.f;
    if(sss < 0.f){sss = 0.f;}
    for(f32 i = 0.f; i < sss; i += 1.f)
    {
        mIdent(&model);
        if(ortho == 0)
            mTranslate(&model, 2.62939f, -4.54055f, 0.033f*i);
        else
            mTranslate(&model, 4.62939f, -4.54055f, 0.033f*i);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
    }

    // pitch coins
    for(int i=3; i < MAX_COINS; i++)
    {
        if(coins[i].color == -1)
            continue;
        
        if(coins[i].color == 0)
            modelBind3(&mdlCoinSilver);
        else
            modelBind3(&mdlCoin);

        mIdent(&model);
        mScale(&model, 1.f, 1.f, 2.f);
        mTranslate(&model, coins[i].x, coins[i].y, 0.f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
    }

    // tux is fancy
    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 0.148f);

    // trophies
    for(int i=0; i < 3; i++)
    {
        mIdent(&model);
        mTranslate(&model, coins[i].x, coins[i].y, 0.f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        
        // Draw the base Tux model, this will be our base to add apparel onto
        glUniform1f(opacity_id, 0.148f);
        modelBind3(&mdlTux);
        glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

        // Tux Skin Selection.
        switch (coins[i].color) {
            case 2:
                glUniform1f(opacity_id, 0.5f);
                modelBind3(&mdlEvil);
                glDrawElements(GL_TRIANGLES, evil_numind, GL_UNSIGNED_BYTE, 0);
                break;
            case 3:
                glUniform1f(opacity_id, 0.6f);
                modelBind3(&mdlKing);
                glDrawElements(GL_TRIANGLES, king_numind, GL_UNSIGNED_BYTE, 0);
                break;
            case 4:
                modelBind3(&mdlNinja);
                glDrawElements(GL_TRIANGLES, ninja_numind, GL_UNSIGNED_BYTE, 0);
                break;
            case 5:
                glUniform1f(opacity_id, 0.4f);
                modelBind3(&mdlSurf);
                glDrawElements(GL_TRIANGLES, surf_numind, GL_UNSIGNED_SHORT, 0);
                break;
            case 6:
                glUniform1f(opacity_id, 0.5f);
                modelBind3(&mdlTrip);
                glDrawElements(GL_TRIANGLES, trip_numind, GL_UNSIGNED_SHORT, 0);
                break;
        }
    }

    //

    if (trophies_all()) // Are there any trophies that need to be rendered?
    { 
        if(trophies_get(0))
        {
            mIdent(&model);
            mTranslate(&model, 3.92732f, 1.0346f, 0.f);
            mRotZ(&model, t*0.3f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
        }
        if(trophies_get(1))
        {
            mIdent(&model);
            mTranslate(&model, 3.65552f, -1.30202f, 0.f);
            mRotZ(&model, t*0.3f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
            glUniform1f(opacity_id, 0.5f);
            modelBind3(&mdlEvil);
            glDrawElements(GL_TRIANGLES, evil_numind, GL_UNSIGNED_BYTE, 0);
        }
        if(trophies_get(2))
        {
            mIdent(&model);
            mTranslate(&model, 3.01911f, -3.23534f, 0.f);
            mRotZ(&model, t*0.3f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
            glUniform1f(opacity_id, 0.6f);
            modelBind3(&mdlKing);
            glDrawElements(GL_TRIANGLES, king_numind, GL_UNSIGNED_BYTE, 0);
        }
        if(trophies_get(3))
        {
            mIdent(&model);
            mTranslate(&model, -3.92732f, 1.0346f, 0.f);
            mRotZ(&model, t*0.3f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
            modelBind3(&mdlNinja);
            glDrawElements(GL_TRIANGLES, ninja_numind, GL_UNSIGNED_BYTE, 0);
        }
        if(trophies_get(4))
        {
            mIdent(&model);
            mTranslate(&model, -3.65552f, -1.30202f, 0.f);
            mRotZ(&model, t*0.3f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
            glUniform1f(opacity_id, 0.4f);
            modelBind3(&mdlSurf);
            glDrawElements(GL_TRIANGLES, surf_numind, GL_UNSIGNED_SHORT, 0);
        }
        if(trophies_get(5))
        {
            mIdent(&model);
            mTranslate(&model, -3.01911f, -3.23534f, 0.f);
            mRotZ(&model, t*0.3f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
            glUniform1f(opacity_id, 0.5f);
            modelBind3(&mdlTrip);
            glDrawElements(GL_TRIANGLES, trip_numind, GL_UNSIGNED_SHORT, 0);
        }
    }

    // render scene props
    const f32 std = t-rst;
    if((gameover > 0.f && t > gameover) || std < 6.75f)
    {
        shadeFullbright(&position_id, &projection_id, &modelview_id, &color_id, &opacity_id);
        glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
        glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
        csp = 0;

        if(std < 6.75f)
        {
            if((std > 1.5f && std < 2.f) || (std > 2.5f && std < 3.f) || (std > 3.5f && std < 4.f))
            {
                glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
                glUniform3f(color_id, 0.89f, 0.f, 0.157f);
                modelBind1(&mdlRX);
                glDrawElements(GL_TRIANGLES, rx_numind, GL_UNSIGNED_BYTE, 0);
            }

            if((std > 4.5f && std < 4.75f) || (std > 5.f && std < 5.25f))
            {
                modelBind1(&mdlSA);

                mIdent(&model);
                mTranslate(&model, -0.01f, 0.01f, 0.f);
                mMul(&modelview, &model, &view);
                glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
                glUniform3f(color_id, 0.f, 0.f, 0.f);
                glDrawElements(GL_TRIANGLES, sa_numind, GL_UNSIGNED_BYTE, 0);
                
                glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
                glUniform3f(color_id, 0.714f, 0.741f, 0.8f);
                glDrawElements(GL_TRIANGLES, sa_numind, GL_UNSIGNED_BYTE, 0);
            }

            if((std > 5.5f && std < 5.75f) || (std > 6.f && std < 6.75f))
            {
                f32 step = (std-6.25f)*2.f;
                if(step < 0.f){step = 0.f;}
                glUniform3f(color_id, 0.698f - (0.16859f * step), 0.667f + (0.14084f * step), 0.263f + (0.65857f * step));
                glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
                modelBind1(&mdlGA);
                glDrawElements(GL_TRIANGLES, ga_numind, GL_UNSIGNED_BYTE, 0);
            }
        }

        // render game over
        if(gameover > 0.f && t > gameover)
        {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);

            modelBind1(&mdlPlane);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
            glUniform3f(color_id, 0.f, 0.f, 0.f);
            f32 opa = t-gameover;
            if(opa > 0.8f){opa = 0.8f;}
            glUniform1f(opacity_id, opa);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
            
            glUniform1f(opacity_id, 0.5f);
            modelBind1(&mdlGameover);

            mIdent(&model);
            mTranslate(&model, -0.01f, 0.01f, 0.01f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform3f(color_id, 0.f, 0.f, 0.f);
            glDrawElements(GL_TRIANGLES, gameover_numind, GL_UNSIGNED_SHORT, 0);

            mIdent(&model);
            mTranslate(&model, 0.005f, -0.005f, -0.005f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glUniform3f(color_id, 0.2f, 0.2f, 0.2f);
            glDrawElements(GL_TRIANGLES, gameover_numind, GL_UNSIGNED_SHORT, 0);

            const f32 ts = t*0.3f;
            glUniform3f(color_id, fabsf(cosf(ts)), fabsf(sinf(ts)), fabsf(cosf(ts)));
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
            glDrawElements(GL_TRIANGLES, gameover_numind, GL_UNSIGNED_SHORT, 0);
            
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }
    }


//*************************************
// swap buffers / display render
//*************************************
#ifdef BUILD_GLFW
    glfwSwapBuffers(wnd);
#else
    SDL_GL_SwapWindow(wnd);
#endif
}

//*************************************
// GLFW Input Handelling
//*************************************
#ifdef BUILD_GLFW
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(inmotion == 0 && button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if(gameover > 0.f)
            {
                if(glfwGetTime() > gameover+3.0f)
                {
                    newGame();
                    if(PUSH_SPEED < 32.f)
                    {
                        PUSH_SPEED += 1.f;
                        char titlestr[256];
                        sprintf(titlestr, "TuxPusher [%.1f]", PUSH_SPEED);
                        glfwSetWindowTitle(window, titlestr);
                    }
                }
                return;
            }
            takeStack();
            md = 1;
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            static uint cs = 1;
            cs = 1 - cs;
            if(cs == 0)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
            md = 0;
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width;
    winh = height;

    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = (double)winw;
    wh = (double)winh;
    if(ortho == 1){touch_margin = ww*0.3076923192f;}
    else{touch_margin = ww*0.2058590651f;}
    rww = 1.0/(ww-touch_margin*2.0);
    rwh = 1.0/wh;
    ww2 = ww/2.0;
    wh2 = wh/2.0;
    uw = (double)aspect / ww;
    uh = 1.0/wh;
    uw2 = (double)aspect / ww2;
    uh2 = 1.0/wh2;

    mIdent(&projection);

    if(ortho == 1)
        mOrtho(&projection, -5.0f, 5.0f, -3.2f, 3.4f, 0.01f, 320.f);
    else
    {
        if(winw > winh)
            aspect = ww / wh;
        else
            aspect = wh / ww;
        mPerspective(&projection, 30.0f, aspect, 0.1f, 320.f);
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_F)
        {
            if(t-lfct > 2.0)
            {
                char strts[16];
                timestamp(&strts[0]);
                const double nfps = fc/(t-lfct);
                printf("[%s] FPS: %g\n", strts, nfps);
                lfct = t;
                fc = 0;
            }
        }
        else if(key == GLFW_KEY_C)
        {
            ortho = 1 - ortho;
            window_size_callback(window, winw, winh);
        }
    }
}
#endif

//*************************************
// Process Entry Point
//*************************************
#ifndef BUILD_GLFW
SDL_Surface* surfaceFromData(const Uint32* data, Uint32 w, Uint32 h)
{
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    memcpy(s->pixels, data, s->pitch*h);
    return s;
}
void printAttrib(SDL_GLattr attr, char* name)
{
    int i;
    SDL_GL_GetAttribute(attr, &i);
    printf("%s: %i\n", name, i);
}
#endif

#ifdef __linux__ 
// This is for benchmarking a specific function.
// Returns the processing time. 
unsigned int BenchmarkFunction(void (*F)(), int samples)
{
    (*F)(); // Preempt the function to skip initializations
    unsigned int average = 0;
    for (int i = 0; i < samples; i++)
    {
        struct timespec InitalTime;
        struct timespec FinalTime;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &InitalTime);
        (*F)(); // execute function
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &FinalTime);
        average += FinalTime.tv_nsec - InitalTime.tv_nsec;
    }
    if (average % 2 == 1) { // odd number detected!
        average += 1;
    }
    return average/samples;
}
#endif


// A small function to perform djb2 hash algorithm. This is for quick string checking and other hash needs.
// More info here: https://github.com/dim13/djb2/blob/master/docs/hash.md
unsigned int quickHash(const char *string) 
{
    unsigned long hash = 5381;
    int c;
    while (c = *string++) {
        hash = ((hash << 5) + hash) + c;
    }
    return (unsigned int)hash; // cast into an int so it's usable with switches.
}

int main(int argc, char** argv)
{
    // set game push speed (global variable)
    PUSH_SPEED = 1.6f;

    // msaa level variable
    int option_msaa = 16;

    // Vertical sync option. 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync
    int option_vsync = 1;

    // Evaluate hashes for comparing arguments later...
    const int HASHGEN = 285276507; // --generate-hash
    const int TINY_HASHGEN = 193429505; // -gh
    
    const int HELP = 1950366504; // --help
    const int TINY_HELP = 5861498; // -h

    const int MSAALEVEL = 2444437574; // --msaa-level
    const int TINY_MSAALEVEL = 191564340; // -msaa

    const int VSYNC = 596612675; // --vertical-sync
    const int TINY_VSYNC = 193430011; // -vs

    const int PUSHSPEED = 1053346333; // --push-speed
    const int TINY_PUSHSPEED = 193429813; // -ps

    const int ARG_BENCHMARK = 3795426538; // --benchmark
    const int ARG_BENCHMARK_TINY = 193429345; // -bm

    // Loop through console arguments and adjust program accordingly.
    // i starts at one to skip the program name.
    for (int i = 1; i < argc; i++) {
        switch (quickHash(argv[i])) {
            case HASHGEN:
            case TINY_HASHGEN:
                printf("This tool is designed to generate hashes to compare the console arguments to.\n");
                printf("Hashed value: %u\n", quickHash(argv[i+1]));
                printf("Prehashed Value: %s\n", argv[i+1]);
                exit(0);
            case HELP: // Display the help menu and quit
            case TINY_HELP:
                printf(HelpMenu);
                exit(0);
            case ARG_BENCHMARK:
            case ARG_BENCHMARK_TINY: // Benchmark multiple aspects of the game.
                printf("==============================\n\n   -= Benchmark results =-\n\n");
                printf("Collision Function: %i ns\n", BenchmarkFunction((void(*)())stepCollisions, 512));
                printf("Take Stack: %i ns\n", BenchmarkFunction((void(*)())takeStack, 512));
                printf("inject Figures: %i ns\n", BenchmarkFunction((void(*)())injectFigure, 512));
                printf("New Game Function: %i ns\n\n", BenchmarkFunction((void(*)())newGame, 16));
                printf("Inside Pitch: %i ns\n", BenchmarkFunction((void(*)())insidePitch, 512));
                printf("\n==============================\n");
                exit(0);
            case MSAALEVEL: // Change the MSAA level.
            case TINY_MSAALEVEL:
                option_msaa = atoi(argv[i+1]);
                break;
            case VSYNC: // change the Vsync options.
            case TINY_VSYNC:
                switch (atoi(argv[i+1])) { // sanitizes the vsync options
                    case -1:
                        option_vsync = -1;
                        break;
                    case 1:
                        option_vsync = 1;
                        break;
                    default:
                        printf("WARNING: Invalid vsync option, valid options are: -1, 0, 1.");
                }
                break;
            case PUSHSPEED: // Change the push speed of the game.
            case TINY_PUSHSPEED:
                PUSH_SPEED = atof(argv[i+1]);
                if(PUSH_SPEED > 32.f) {
                    PUSH_SPEED = 32.f;
                }
                printf("Successfully set Push speed to %f", PUSH_SPEED);
                break;
        }
    }

#ifdef BUILD_GLFW
        // init glfw
        if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_SAMPLES, option_msaa);
        wnd = glfwCreateWindow(winw, winh, appTitle, NULL, NULL);
        if(!wnd)
        {
            printf("glfwCreateWindow() failed.\n");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowPos(wnd, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
        glfwSetWindowSizeCallback(wnd, window_size_callback);
        glfwSetKeyCallback(wnd, key_callback);
        glfwSetMouseButtonCallback(wnd, mouse_button_callback);
        glfwMakeContextCurrent(wnd);
        gladLoadGL(glfwGetProcAddress);
        glfwSwapInterval(option_vsync); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

        // set icon
        glfwSetWindowIcon(wnd, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});
#else
        // init sdl
        if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) < 0) // SDL_INIT_AUDIO
        {
            printf("ERROR: SDL_Init(): %s\n", SDL_GetError());
            return 1;
        }
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, option_msaa);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        while(wnd == NULL)
        {
            option_msaa--;
            if(option_msaa == 0)
            {
                printf("ERROR: SDL_CreateWindow(): %s\n", SDL_GetError());
                return 1;
            }
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, option_msaa);
            wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        }
        SDL_GL_SetSwapInterval(option_vsync);
        glc = SDL_GL_CreateContext(wnd);
        if(glc == NULL)
        {
            printf("ERROR: SDL_GL_CreateContext(): %s\n", SDL_GetError());
            return 1;
        }

        // set cursors
        cross_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        beam_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
        SDL_SetCursor(cross_cursor);

        // set icon
        s_icon = surfaceFromData((Uint32*)&icon_image.pixel_data[0], 16, 16);
        SDL_SetWindowIcon(wnd, s_icon);
#endif

    // debug (cant do this on ES unless >= ES 3.2)
#if defined(GL_DEBUG) && !defined(__MINGW32__) // no need to debug the windows release
    esDebug(1);
#endif

#ifndef BUILD_GLFW
    // dump some info
    printAttrib(SDL_GL_DOUBLEBUFFER, "GL_DOUBLEBUFFER");
    printAttrib(SDL_GL_DEPTH_SIZE, "GL_DEPTH_SIZE");
    printAttrib(SDL_GL_RED_SIZE, "GL_RED_SIZE");
    printAttrib(SDL_GL_GREEN_SIZE, "GL_GREEN_SIZE");
    printAttrib(SDL_GL_BLUE_SIZE, "GL_BLUE_SIZE");
    printAttrib(SDL_GL_ALPHA_SIZE, "GL_ALPHA_SIZE");
    printAttrib(SDL_GL_BUFFER_SIZE, "GL_BUFFER_SIZE");
    printAttrib(SDL_GL_STENCIL_SIZE, "GL_STENCIL_SIZE");
    printAttrib(SDL_GL_ACCUM_RED_SIZE, "GL_ACCUM_RED_SIZE");
    printAttrib(SDL_GL_ACCUM_GREEN_SIZE, "GL_ACCUM_GREEN_SIZE");
    printAttrib(SDL_GL_ACCUM_BLUE_SIZE, "GL_ACCUM_BLUE_SIZE");
    printAttrib(SDL_GL_ACCUM_ALPHA_SIZE, "GL_ACCUM_ALPHA_SIZE");
    printAttrib(SDL_GL_STEREO, "GL_STEREO");
    printAttrib(SDL_GL_MULTISAMPLEBUFFERS, "GL_MULTISAMPLEBUFFERS");
    printAttrib(SDL_GL_MULTISAMPLESAMPLES, "GL_MULTISAMPLESAMPLES");
    printAttrib(SDL_GL_ACCELERATED_VISUAL, "GL_ACCELERATED_VISUAL");
    printAttrib(SDL_GL_RETAINED_BACKING, "GL_RETAINED_BACKING");
    printAttrib(SDL_GL_CONTEXT_MAJOR_VERSION, "GL_CONTEXT_MAJOR_VERSION");
    printAttrib(SDL_GL_CONTEXT_MINOR_VERSION, "GL_CONTEXT_MINOR_VERSION");
    printAttrib(SDL_GL_CONTEXT_FLAGS, "GL_CONTEXT_FLAGS");
    printAttrib(SDL_GL_CONTEXT_PROFILE_MASK, "GL_CONTEXT_PROFILE_MASK");
    printAttrib(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, "GL_SHARE_WITH_CURRENT_CONTEXT");
    printAttrib(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, "GL_FRAMEBUFFER_SRGB_CAPABLE");
    printAttrib(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, "GL_CONTEXT_RELEASE_BEHAVIOR");
    printAttrib(SDL_GL_CONTEXT_EGL, "GL_CONTEXT_EGL");

    printf("----\n");
    
    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    printf("Compiled against SDL version %u.%u.%u.\n", compiled.major, compiled.minor, compiled.patch);
    printf("Linked against SDL version %u.%u.%u.\n", linked.major, linked.minor, linked.patch);

    printf("----\n");
#else
    printf("%s\n", glfwGetVersionString());
    printf("----\n");
#endif

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** PLANE *****
    const GLfloat plane_vert[] = {13,0,-13, -13,0,13, -13,0,-13, 13,0,13};
    const GLubyte plane_indi[] = {0,1,2,0,3,1};
    esBind(GL_ARRAY_BUFFER, &mdlPlane.vid, plane_vert, sizeof(plane_vert), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPlane.iid, plane_indi, sizeof(plane_indi), GL_STATIC_DRAW);

    // ***** BIND SCENE *****
    esBind(GL_ARRAY_BUFFER, &mdlScene.cid, scene_colors, sizeof(scene_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlScene.vid, scene_vertices, sizeof(scene_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlScene.nid, scene_normals, sizeof(scene_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlScene.iid, scene_indices, sizeof(scene_indices), GL_STATIC_DRAW);

    // ***** BIND GAMEOVER *****
    esBind(GL_ARRAY_BUFFER, &mdlGameover.vid, gameover_vertices, sizeof(gameover_vertices), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlGameover.iid, gameover_indices, sizeof(gameover_indices), GL_STATIC_DRAW);

    // ***** BIND COIN *****
    esBind(GL_ARRAY_BUFFER, &mdlCoin.cid, coin_colors, sizeof(coin_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCoin.vid, coin_vertices, sizeof(coin_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCoin.nid, coin_normals, sizeof(coin_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlCoin.iid, coin_indices, sizeof(coin_indices), GL_STATIC_DRAW);

    // ***** BIND COIN SILVER *****
    esBind(GL_ARRAY_BUFFER, &mdlCoinSilver.cid, coin_silver_colors, sizeof(coin_silver_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCoinSilver.vid, coin_silver_vertices, sizeof(coin_silver_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCoinSilver.nid, coin_silver_normals, sizeof(coin_silver_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlCoinSilver.iid, coin_silver_indices, sizeof(coin_silver_indices), GL_STATIC_DRAW);

    // ***** BIND TUX *****
    esBind(GL_ARRAY_BUFFER, &mdlTux.cid, tux_colors, sizeof(tux_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTux.vid, tux_vertices, sizeof(tux_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTux.nid, tux_normals, sizeof(tux_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlTux.iid, tux_indices, sizeof(tux_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - EVIL *****
    esBind(GL_ARRAY_BUFFER, &mdlEvil.cid, evil_colors, sizeof(evil_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlEvil.vid, evil_vertices, sizeof(evil_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlEvil.nid, evil_normals, sizeof(evil_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlEvil.iid, evil_indices, sizeof(evil_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - KING *****
    esBind(GL_ARRAY_BUFFER, &mdlKing.cid, king_colors, sizeof(king_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlKing.vid, king_vertices, sizeof(king_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlKing.nid, king_normals, sizeof(king_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlKing.iid, king_indices, sizeof(king_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - NINJA *****
    esBind(GL_ARRAY_BUFFER, &mdlNinja.cid, ninja_colors, sizeof(ninja_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlNinja.vid, ninja_vertices, sizeof(ninja_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlNinja.nid, ninja_normals, sizeof(ninja_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlNinja.iid, ninja_indices, sizeof(ninja_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - SURF *****
    esBind(GL_ARRAY_BUFFER, &mdlSurf.cid, surf_colors, sizeof(surf_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlSurf.vid, surf_vertices, sizeof(surf_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlSurf.nid, surf_normals, sizeof(surf_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlSurf.iid, surf_indices, sizeof(surf_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - TRIP *****
    esBind(GL_ARRAY_BUFFER, &mdlTrip.cid, trip_colors, sizeof(trip_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTrip.vid, trip_vertices, sizeof(trip_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTrip.nid, trip_normals, sizeof(trip_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlTrip.iid, trip_indices, sizeof(trip_indices), GL_STATIC_DRAW);

    // ***** BIND SCENE PROP - RED X *****
    esBind(GL_ARRAY_BUFFER, &mdlRX.vid, rx_vertices, sizeof(rx_vertices), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRX.iid, rx_indices, sizeof(rx_indices), GL_STATIC_DRAW);

    // ***** BIND SCENE PROP - SILVER ARROW *****
    esBind(GL_ARRAY_BUFFER, &mdlSA.vid, sa_vertices, sizeof(sa_vertices), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlSA.iid, sa_indices, sizeof(sa_indices), GL_STATIC_DRAW);

    // ***** BIND SCENE PROP - GOLD ARROW *****
    esBind(GL_ARRAY_BUFFER, &mdlGA.vid, ga_vertices, sizeof(ga_vertices), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlGA.iid, ga_indices, sizeof(ga_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader program
//*************************************

    makeFullbright();
    makeLambert3();

//*************************************
// configure render options
//*************************************

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.52941f, 0.80784f, 0.92157f, 0.0f);

//*************************************
// execute update / render loop
//*************************************

    // projection
#ifdef BUILD_GLFW
    window_size_callback(wnd, winw, winh);
#else
    doPerspective();
#endif

    // new game
    newGame();
    
    // init
    t = f32Time();
    lfct = t;

    // event loop
#ifdef BUILD_GLFW
    while(!glfwWindowShouldClose(wnd))
    {
        glfwPollEvents();
        main_loop();
        fc++;
    }
#else

    // Game loop
    for (;;) {
        main_loop();
        fc++;
    }

#endif

    return 0;
}

